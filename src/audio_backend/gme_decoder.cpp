#include <assert.h>
#include <iostream>
#include <boost/thread/locks.hpp>
#include <gme/gme.h>
#include <gme/Data_Reader.h>
#include "gme_decoder.hpp"


namespace ion
{
namespace audio_backend
{


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


	virtual long read_avail( void *dest, long n)
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


	virtual blargg_err_t skip( long count )
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


bool gme_open_reader( Data_Reader *reader, Music_Emu** out, long sample_rate )
{
	assert( reader && out );
	*out = 0;
	
	char header [4];
	int header_size = 0;
	
	header_size = sizeof header;
	if (reader->read( header, sizeof header ) != 0)
		return false;

	gme_type_t file_type = gme_identify_extension( gme_identify_header( header ) );
	if ( !file_type )
		return false;
	
	Music_Emu* emu = gme_new_emu( file_type, sample_rate );
	if (!emu)
		return false;
	
	// optimization: avoids seeking/re-reading header
	Remaining_Reader rem( header, header_size, reader );
	gme_err_t err = emu->load( rem );
	
	if ( err )
	{
		std::cerr << err << std::endl;
		delete emu;
	}
	else
		*out = emu;
	
	return true;
}


}



gme_decoder::gme_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_):
	decoder(send_command_callback),
	source_(source_),
	initialized(false),
	emu(0)
{
	// Misc checks & initializations

	if (!source_)
		return; // no source available? Nothing can be done in this decoder.

	long source_size = source_->get_size();
	if (source_size <= 0)
		return; // 0 bytes? or no length? we cannot use this.

	{
		source_reader reader(*source_);
		bool ret = gme_open_reader(&reader, &emu, 48000);
		if (!ret)
			return;
	}

	initialized = true;
}


gme_decoder::~gme_decoder()
{
	if (emu != 0)
		delete emu;
}


bool gme_decoder::is_initialized() const
{
	return initialized;
}


bool gme_decoder::can_playback() const
{
	return is_initialized() && (emu != 0);
}


void gme_decoder::pause()
{
}


void gme_decoder::resume()
{
}


long gme_decoder::set_current_position(long const new_position)
{
	return 0;
}


long gme_decoder::get_current_position() const
{
	return 0;
}


long gme_decoder::set_current_volume(long const new_volume)
{
	return max_volume(); // TODO:
}


long gme_decoder::get_current_volume() const
{
	return max_volume(); // TODO:
}


metadata_t gme_decoder::get_metadata() const
{
	return empty_metadata();
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
	return -1;
}


long gme_decoder::get_num_ticks_per_second() const
{
	return -1;
}


void gme_decoder::set_loop_mode(int const new_loop_mode)
{
}


void gme_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	if (!is_initialized())
		return;

	boost::lock_guard < boost::mutex > lock(mutex_);

	playback_properties_ = new_playback_properties;

	if (emu != 0)
	{
		delete emu;
		emu = 0;
	}

	source_->reset();
	source_reader reader(*source_);
	bool ret = gme_open_reader(&reader, &emu, playback_properties_.frequency);
	if (!ret)
	{
		std::cerr << "E 2\n";
		if (emu != 0) delete emu;
		emu = 0;
	}
	else
	{
		emu->start_track(0);

		track_info_t track_info_;
		emu->track_info( &track_info_ );
		{
			if ( track_info_.length <= 0 )
				track_info_.length = track_info_.intro_length +
					track_info_.loop_length * 2;
		}
		if ( track_info_.length <= 0 )
			track_info_.length = (long) (2.5 * 60 * 1000);

		emu->set_fade(track_info_.length);
	}
}


unsigned int gme_decoder::get_decoder_samplerate() const
{
	return 0;
}


unsigned int gme_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	boost::lock_guard < boost::mutex > lock(mutex_);

	if (emu == 0)
		return 0;

	emu->play(num_samples_to_write * 2, reinterpret_cast < Music_Emu::sample_t* > (dest));

	if (emu->track_ended())
		return 0;
	else
		return num_samples_to_write;
}




gme_decoder_creator::gme_decoder_creator()
{
}


decoder_ptr_t gme_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback, std::string const &mime_type)
{
	gme_decoder *gme_decoder_ = new gme_decoder(send_command_callback, source_);
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

