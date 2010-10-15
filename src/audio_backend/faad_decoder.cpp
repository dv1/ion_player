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


#include <cstring>
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
	initialized(false),
	current_position(0)
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

	source_->reset();
	in_buffer.resize(32768);
	long num_read_bytes = source_->read(&in_buffer[0], 32768);
	if (num_read_bytes <= 0)
		return false;
	in_buffer.resize(num_read_bytes);

	ret = faacDecInit(*faad_handle, &in_buffer[0], in_buffer.size(), &sample_rate, &channels);
	if (ret < 0)
	{
		std::cerr << "faacDecInit failed" << std::endl;
		close();
		return false;
	}
	else
	{
		decoder_sample_rate = sample_rate;
		if (ret > 0)
		{
			std::memmove(&in_buffer[0], &in_buffer[ret], in_buffer.size() - ret);
			in_buffer.resize(in_buffer.size() - ret);
		}
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


long faad_decoder::set_current_position(long const)
{
	// It generally does not seem to be possible to seek in AAC songs with FAAD
	return current_position;
}


long faad_decoder::get_current_position() const
{
	// It generally does not seem to be possible to retrieve the current position with FAAD
	return current_position;
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


decoder_properties faad_decoder::get_decoder_properties() const
{
	return decoder_properties(decoder_sample_rate, playback_properties_.num_channels, playback_properties_.sample_type_);
}


unsigned int faad_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	unsigned int multiplier = playback_properties_.num_channels * get_sample_size(playback_properties_.sample_type_);

	faacDecFrameInfo info;
	while (out_buffer.size() < (num_samples_to_write * multiplier))
	{
		size_t actual_in_size = in_buffer.size();

		if (actual_in_size < 32768)
		{
			size_t old_in_size = actual_in_size;
			in_buffer.resize(old_in_size + 32768);
			long num_read_bytes = source_->read(&in_buffer[old_in_size], 32768);
			if (num_read_bytes <= 0)
				break;
			actual_in_size = old_in_size + num_read_bytes;
		}

		void *output_samples = faacDecDecode(*faad_handle, &info, reinterpret_cast < unsigned char* > (&in_buffer[0]), actual_in_size);
		unsigned int num_remaining_bytes = actual_in_size;

		num_remaining_bytes -= info.bytesconsumed;
		std::memmove(&in_buffer[0], &in_buffer[info.bytesconsumed], num_remaining_bytes);
		in_buffer.resize(num_remaining_bytes);

		if ((output_samples == 0) || (info.error != 0) || (info.channels == 0))
		{
			initialized = false;
			break;
		}

		if (info.samples > 0)
		{
			size_t old_out_size = out_buffer.size();
			out_buffer.resize(old_out_size + info.samples * playback_properties_.num_channels);
			std::memcpy(&out_buffer[old_out_size], output_samples, info.samples * playback_properties_.num_channels);
		}
	}

	size_t num_bytes_to_copy = std::min(size_t(num_samples_to_write * multiplier), out_buffer.size());
	std::memcpy(dest, &out_buffer[0], num_bytes_to_copy);
	if (out_buffer.size() > num_bytes_to_copy)
		std::memmove(&out_buffer[0], &out_buffer[num_bytes_to_copy], out_buffer.size() - num_bytes_to_copy);
	out_buffer.resize(out_buffer.size() - num_bytes_to_copy);

	current_position += num_bytes_to_copy / multiplier;
	return num_bytes_to_copy / multiplier;
}




faad_decoder_creator::faad_decoder_creator()
{
}


decoder_ptr_t faad_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_event_callback_t const &send_event_callback)
{
	// Test found in libmagic's animation file
	{
		uint8_t bytes[4];
		source_->reset();
		source_->read(bytes, 4);
		source_->reset();

		// check if this is an AAC file in ADIF format
		if ((bytes[0] != 'A') || (bytes[1] != 'D') || (bytes[2] != 'I') || (bytes[3] != 'F'))
		{
			// check if this is an AAC file in ADTS format
			if ((bytes[0] != 0xff) || ((bytes[1] & 0xf6) != 0xf0))
				return decoder_ptr_t();
		}
	}

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

