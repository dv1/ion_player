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


#include <iostream>
#include <boost/thread/locks.hpp>
#include "faad_decoder.hpp"


namespace ion
{
namespace audio_backend
{


faad_decoder::faad_decoder(send_event_callback_t const send_event_callback, source_ptr_t source_):
	decoder(send_event_callback),
	source_(source_),
	initialized(false)
{
	// Misc checks & initializations

	if (!source_)
		return; // no source available? Nothing can be done in this decoder.

	long source_size = source_->get_size();
	if (source_size == 0)
		return; // 0 bytes? we cannot use this.

	initialized = initialize(48000);
}


bool faad_decoder::initialize(unsigned int const frequency)
{
	close();

	long ret;

	faad_handle = faacDecOpen();

	faacDecConfigurationPtr conf = faacDecGetCurrentConfiguration(*faad_handle);
	conf->defSampleRate = frequency;
	conf->outputFormat = FAAD_FMT_16BIT;
	faacDecSetConfiguration(*faad_handle, conf);

	unsigned long sample_rate;
	unsigned char channels;

	std::vector < uint8_t > read_buffer(512);
	long actual_read = source_->read(&read_buffer[0], read_buffer.size());
	source_->reset();
	read_buffer.resize(actual_read);

	ret = faacDecInit(*faad_handle, &read_buffer[0], read_buffer.size(), &sample_rate, &channels);
	if (ret < 0)
	{
		close();
		return false;
	}
	else
	{
		decoder_sample_rate = sample_rate;
		return true;
	}
}


void faad_decoder::close()
{
	if (faad_handle)
	{
		faacDecClose(*faad_handle);
		faad_handle = boost::none;
	}
}


faad_decoder::~faad_decoder()
{
	close();
}


bool faad_decoder::is_initialized() const
{
	return initialized && source_;
}


bool faad_decoder::can_playback() const
{
	return is_initialized();
}


void faad_decoder::pause()
{
}


void faad_decoder::resume()
{
}


long faad_decoder::set_current_position(long const new_position)
{
	if (!can_playback())
		return -1;

	if ((new_position < 0) || (new_position >= get_num_ticks()))
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);
	return 0;
}


long faad_decoder::get_current_position() const
{
	if (!can_playback())
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);
	return 0;
}


long faad_decoder::set_current_volume(long const new_volume)
{
	return max_volume(); // TODO:
}


long faad_decoder::get_current_volume() const
{
	return max_volume(); // TODO:
}


metadata_t faad_decoder::get_metadata() const
{
	metadata_t metadata_ = empty_metadata();
	return metadata_;
}


std::string faad_decoder::get_type() const
{
	return "faad";
}


uri faad_decoder::get_uri() const
{
	return (source_) ? source_->get_uri() : uri();
}


long faad_decoder::get_num_ticks() const
{
	return 0;
}


long faad_decoder::get_num_ticks_per_second() const
{
	return decoder_sample_rate;
}


void faad_decoder::set_loop_mode(int const new_loop_mode)
{
}


void faad_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	if (!is_initialized())
		return;

	boost::lock_guard < boost::mutex > lock(mutex_);

	playback_properties_ = new_playback_properties;
	initialize(playback_properties_.frequency);
}


unsigned int faad_decoder::get_decoder_samplerate() const
{
	return decoder_sample_rate;
}


unsigned int faad_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	faacDecFrameInfo info;
	faacDecDecode(*faad_handle, &info, reinterpret_cast < unsigned char* > (dest), num_samples_to_write * 4);

	return ((info.error == 0) && (info.channels != 0)) ? (info.samples / info.channels) : 0;
}




faad_decoder_creator::faad_decoder_creator()
{
}


decoder_ptr_t faad_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_event_callback_t const &send_event_callback)
{
	return decoder_ptr_t(); // TODO: remove this later

	faad_decoder *faad_decoder_ = new faad_decoder(send_event_callback, source_);
	if (!faad_decoder_->is_initialized())
	{
		delete faad_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(faad_decoder_);
}


}
}

