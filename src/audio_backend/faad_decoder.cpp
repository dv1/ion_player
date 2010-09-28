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


#if 0
#include <iostream>
#include <boost/thread/locks.hpp>
#include "faad_decoder.hpp"


namespace ion
{
namespace audio_backend
{


faad_decoder::faad_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_)
	decoder(send_command_callback),
	source_(source_),
	initialized(false)
{
	// Misc checks & initializations

	if (!source_)
		return; // no source available? Nothing can be done in this decoder.

	long source_size = source_->get_size();
	if (source_size == 0)
		return; // 0 bytes? we cannot use this.

	long ret;

	faacDecConfigurationPtr conf = faacDecGetCurrentConfiguration(faad_handle);
	faacDecSetConfiguration(faad_handle, conf);

	faad_handle = faacDecOpen();
	unsigned long sample_rate;
	unsigned char channels;
	ret = faacDecInit(faad_handle, &read_buffer[0], read_buffer.size(), &sample_rate, &channels);
	if (ret < 0)
		return;

	initialized = true;
}


void faad_decoder::initialize(unsigned int const frequency)
{
	close();


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
}


long faad_decoder::get_current_position() const
{
	if (!can_playback())
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);
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
}


long faad_decoder::get_num_ticks_per_second() const
{
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
}


unsigned int faad_decoder::get_decoder_samplerate() const
{
}


unsigned int faad_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

}




faad_decoder_creator::faad_decoder_creator()
{
}


decoder_ptr_t faad_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback, std::string const &mime_type)
{
	if ((mime_type != "audio/x-hx-aac-adts") && (mime_type != "audio/x-hx-aac-adif"))
		return decoder_ptr_t();


	faad_decoder *faad_decoder_ = new faad_decoder(send_command_callback, source_);
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


#endif

