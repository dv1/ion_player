/**************************************************************************

    Copyright (C) 2010  Carlos Rafael Giani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**************************************************************************/


#include <assert.h>
#include <iostream>
#include <stdint.h>

#include "gme_decoder.hpp"
#include <gme/gme.h>
#include <gme/Data_Reader.h>
#include <gme/Music_Emu.h>

#include <boost/thread/locks.hpp>



namespace ion
{
namespace audio_backend
{


using namespace audio_common;


namespace
{


class source_reader:
	public Data_Reader
{
public:
	explicit source_reader(source &source_):
		source_(source_)
	{
	}


	virtual long read_avail(void *dest, long n)
	{
		if (source_.can_read())
			return source_.read(dest, n);
		else
			return -1;
	}

	
	virtual long remain() const
	{
		long ret = long(source_.get_size()) - long(source_.get_position());
		assert(ret >= 0);
		return ret;
	}


	virtual blargg_err_t skip(long count)
	{
		if (source_.can_seek(source::seek_relative))
		{
			source_.seek(count, source::seek_relative);
			return 0;
		}
		else
			return Data_Reader::skip(count);
	}


protected:
	source &source_;
};


}


struct gme_decoder::internal_data
{
	Music_Emu *emu;
	track_info_t track_info_;


	explicit internal_data():
		emu(0)
	{
	}

	~internal_data()
	{
		if (emu != 0)
			delete emu;
	}
};


gme_decoder::gme_decoder(send_event_callback_t const send_event_callback, source_ptr_t source_):
	decoder(send_event_callback),
	source_(source_),
	track_nr(0),
	internal_data_(0)
{
	internal_data_ = new internal_data;
	if (internal_data_ == 0)
		return;

	// Misc checks & initializations

	if (!source_)
		return; // no source available? Nothing can be done in this decoder.

	long source_size = source_->get_size();
	if (source_size <= 0)
		return; // 0 bytes? or no length? we cannot use this.

	try
	{
		// For some reason, making options_ a reference causes segfaults in the find() call below TODO: check why this happens
		uri::options_t const options_ = source_->get_uri().get_options();
		uri::options_t::const_iterator iter = options_.find("sub_resource_index");
		if (iter != options_.end())
			track_nr = boost::lexical_cast < int > (iter->second);
	}
	catch (boost::bad_lexical_cast const &)
	{
	}

	reset_emu(48000);
}


gme_decoder::~gme_decoder()
{
	delete internal_data_;
}


bool gme_decoder::is_initialized() const
{
	return internal_data_->emu;
}


bool gme_decoder::can_playback() const
{
	return internal_data_->emu;
}


long gme_decoder::set_current_position(long const new_position)
{
	if (!is_initialized())
		return 0;

	boost::lock_guard < boost::mutex > lock(mutex_);
	internal_data_->emu->seek(new_position);
	set_fade(); // this is necessary, since GME resets the fade time when seeking backwards

	return internal_data_->emu->tell();
}


long gme_decoder::get_current_position() const
{
	if (!is_initialized())
		return 0;

	boost::lock_guard < boost::mutex > lock(mutex_);

	return internal_data_->emu->tell();
}


metadata_t gme_decoder::get_metadata() const
{
	metadata_t metadata_ = empty_metadata();
	if (!is_initialized())
		return metadata_;

	{
		std::string str(internal_data_->track_info_.song);
		if (!str.empty())
		set_metadata_value(metadata_, "title", str);
	}

	{
		std::string str(internal_data_->track_info_.author);
		if (!str.empty())
		set_metadata_value(metadata_, "artist", str);
	}

	{
		std::string str(internal_data_->track_info_.game);
		if (!str.empty())
		set_metadata_value(metadata_, "album", str);
	}

	if (internal_data_->track_info_.track_count > 1)
		set_metadata_value(metadata_, "num_sub_resources", int(internal_data_->track_info_.track_count));

	return metadata_;
}


std::string gme_decoder::get_type() const
{
	return "gme";
}


uri gme_decoder::get_uri() const
{
	return (source_) ? source_->get_uri() : uri();
}


long gme_decoder::get_num_ticks() const
{
	if (!is_initialized())
		return 0;

	return std::max(long(internal_data_->track_info_.length), long(0));
}


long gme_decoder::get_num_ticks_per_second() const
{
	return 1000;
}


void gme_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	if (!is_initialized())
		return;

	boost::lock_guard < boost::mutex > lock(mutex_);

	playback_properties_ = new_playback_properties;
	reset_emu(playback_properties_.frequency);
}


bool gme_decoder::reset_emu(unsigned int const sample_rate)
{
	blargg_err_t error;

	source_->reset();
	source_reader reader(*source_);

	if (internal_data_->emu != 0)
	{
		delete internal_data_->emu;
		internal_data_->emu = 0;
	}

	uint8_t header[4];

	if (reader.read(header, sizeof(header)) != 0)
		return false;

	gme_type_t file_type = gme_identify_extension(gme_identify_header(header));
	if (!file_type)
		return false;

	Music_Emu *new_emu = file_type->new_emu();
	if (new_emu == 0)
		return false;

	error = new_emu->set_sample_rate(sample_rate);
	if (error) { delete new_emu; return false; }

	Remaining_Reader remaining_reader(header, sizeof(header), &reader);
	error = new_emu->load(remaining_reader);
	if (error) { delete new_emu; return false; }

	error = new_emu->start_track(track_nr);
	if (error) { delete new_emu; return false; }

	error = new_emu->track_info(&internal_data_->track_info_);
	if (error) { delete new_emu; return false; }

	/*std::cerr
		<< internal_data_->track_info_.length << " "
		<< internal_data_->track_info_.intro_length << " "
		<< internal_data_->track_info_.loop_length << std::endl;*/

	if (internal_data_->track_info_.length <= 0)
		internal_data_->track_info_.length = internal_data_->track_info_.intro_length + internal_data_->track_info_.loop_length * 2;
	if (internal_data_->track_info_.length <= 0)
		internal_data_->track_info_.length = 0;

	internal_data_->emu = new_emu;
	set_fade();

	return true;
}


void gme_decoder::set_fade()
{
	internal_data_->emu->set_fade(
		(internal_data_->track_info_.length != 0)
			? long(internal_data_->track_info_.length)
			: long(internal_data_->emu->tell() + 2.5 * 60 * 1000) // when no length is known, set the fade point to current point + 150 seconds (to have some tolerance)
		,
		0
	);
}


decoder_properties gme_decoder::get_decoder_properties() const
{
	return decoder_properties(0, 2, audio_common::sample_s16);
}


unsigned int gme_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	if (internal_data_->track_info_.length == 0)
		internal_data_->emu->set_fade(internal_data_->emu->tell() + 2.5 * 60 * 1000);// when no length is known, set the fade point to current point + 150 seconds (to have some tolerance)

	boost::lock_guard < boost::mutex > lock(mutex_);

	internal_data_->emu->play(num_samples_to_write * 2, reinterpret_cast < Music_Emu::sample_t* > (dest));

	if (internal_data_->emu->track_ended())
		return 0;
	else
		return num_samples_to_write;
}




gme_decoder_creator::gme_decoder_creator()
{
}


decoder_ptr_t gme_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_event_callback_t const &send_event_callback)
{
	gme_decoder *gme_decoder_ = new gme_decoder(send_event_callback, source_);
	if (!gme_decoder_->is_initialized())
	{
		delete gme_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(gme_decoder_);
}


}
}

