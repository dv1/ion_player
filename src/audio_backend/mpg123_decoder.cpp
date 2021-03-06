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
#include "mpg123_decoder.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


mpg123_decoder::mpg123_decoder(send_event_callback_t const send_event_callback, source_ptr_t source_):
	decoder(send_event_callback),
	source_(source_),
	mpg123_handle_(0),
	id3v1_data(0),
	id3v2_data(0),
	in_buffer_size(16384),
	out_buffer_pad_size(32768),
	has_song_length(false)
{
	// Misc checks & initializations

	if (!source_)
		return; // no source available? Nothing can be done in this decoder.

	long source_size = source_->get_size();
	if (source_size == 0)
		return; // 0 bytes? we cannot use this.

	in_buffer.resize(in_buffer_size);
	out_buffer.resize(0); // This explicitely states that the out_buffer's size is expected to be zero in the beginning, which is important for playback and on-demand buffer filling


	// mpg123 handle initialization

	mpg123_handle_ = mpg123_new(0, &last_read_return_value);

	if (mpg123_handle_ == 0)
	{
		std::cerr << "Unable to create mpg123 handle: " << mpg123_plain_strerror(last_read_return_value) << std::endl;
		return;
	}


	// Setting some flags
	{
		mpg123_param(mpg123_handle_, MPG123_ADD_FLAGS, MPG123_GAPLESS, 0);
	}


	// setting the file size for determining the song length (if said size is available)

	{
		if (source_size > 0) // song length <0 means: size not available (the == 0 case is checked above already)
		{
			mpg123_open_feed(mpg123_handle_);
			if (MPG123_ERR != mpg123_set_filesize(mpg123_handle_, source_size))
				has_song_length = true;
		}
	}


	// Do an initial read of mp3 data, to retrieve data about the format

	last_read_return_value = MPG123_NEED_MORE;
	bool new_format_found = false;

	/* Let it read until one of these things happen:
	1. mpg123 needs more data, but the source has none (an error case)
	2. mpg123 reports an error
	3. mpg123 found a new format
	*/
	while (!new_format_found)
	{
		// First, expand the out buffer by a predefined amount to allow for new samples
		size_t decoded_size;
		unsigned int out_buffer_offset = out_buffer.size();
		out_buffer.resize(out_buffer_offset + out_buffer_pad_size);

		// mp3 decoding happens in this scope
		{
			if (last_read_return_value == MPG123_NEED_MORE)
			{
				// mpg123 needs more input data, lets deliver some
				unsigned long read_size = 0;
				if (source_->can_read())
					read_size = source_->read(&in_buffer[0], in_buffer_size);			
				if (read_size == 0) // the source could not deliver data; this is the case (1) mentioned above
				{
					std::cerr << "Could not read from MP3 source" << std::endl;
					out_buffer.resize(out_buffer_offset);
					mpg123_delete(mpg123_handle_);
					mpg123_handle_ = 0;
					return;
				}

				last_read_return_value = mpg123_decode(mpg123_handle_, &in_buffer[0], read_size, &out_buffer[out_buffer_offset], out_buffer_pad_size, &decoded_size);
			}
			else
			{
				// Currently, no more input data is needed - just decode
				last_read_return_value = mpg123_decode(mpg123_handle_, 0, 0, &out_buffer[out_buffer_offset], out_buffer_pad_size, &decoded_size);
			}
		}

		// evaluate the return value
		switch (last_read_return_value)
		{
			case MPG123_ERR:
			{
				// mpg123 reported an error - delete the handle, the decoder cannot do anything
				std::cerr << "Error while preloading mp3 data: " << mpg123_plain_strerror(last_read_return_value) << std::endl;
				mpg123_delete(mpg123_handle_);
				mpg123_handle_ = 0;
				return;
			}

			case MPG123_NEW_FORMAT:
			{
				// mpg123 found a format specification - stop preloading, it is done
				long rate;
				int channels, enc;
				mpg123_getformat(mpg123_handle_, &rate, &channels, &enc);
				src_num_channels = channels;
				src_frequency = rate;
				std::cerr << "mpg123: " << rate << " Hz " << channels << " channels, encoding value " << enc << std::endl;
				new_format_found = true;
				break;
			}

			default:
				break;
		}

		// correct the out buffer's data to match the actual amount of decoded bytes
		out_buffer.resize(out_buffer_offset + decoded_size);
	}

	if (!new_format_found)
	{
		// no format specifier found -> cannot play
		mpg123_delete(mpg123_handle_);
		mpg123_handle_ = 0;
		return;
	}


	// Set the pointers for ID3 data
	{
		last_read_return_value = mpg123_id3(mpg123_handle_, &id3v1_data, &id3v2_data);
		if (last_read_return_value == MPG123_ERR)
		{
			mpg123_delete(mpg123_handle_);
			mpg123_handle_ = 0;
			return;
		}
	}
}


mpg123_decoder::~mpg123_decoder()
{
	boost::lock_guard < boost::mutex > lock(mutex_);
	
	if (mpg123_handle_ != 0)
		mpg123_delete(mpg123_handle_);
}


bool mpg123_decoder::is_initialized() const
{
	return (mpg123_handle_ != 0) && source_;
}


bool mpg123_decoder::can_playback() const
{
	return is_initialized();
}


long mpg123_decoder::set_current_position(long const new_position)
{
	if (!has_song_length)
		return -1;

	if (!can_playback())
		return -1;

	if ((new_position < 0) || (new_position >= get_num_ticks()))
		return -1;

	if (!source_->can_seek(source::seek_absolute))
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);

	off_t input_offset;
	off_t ret = mpg123_feedseek(mpg123_handle_, new_position, SEEK_SET, &input_offset);
	if (ret == MPG123_ERR)
	{
		return -1;
	}
	else
	{
		source_->seek(input_offset, source::seek_absolute);
		return ret;
	}
}


long mpg123_decoder::get_current_position() const
{
	if (!has_song_length)
		return -1;

	if (!can_playback())
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);

	return mpg123_tell(mpg123_handle_);
}


namespace
{

void set_metadata_from_mpg123_string(metadata_t &metadata_, char const *field_name, mpg123_string *string_)
{
	if (string_ != 0)
	{
		if ((string_->p != 0) && (string_->fill >= 1))
			set_metadata_value(metadata_, field_name, std::string(string_->p, string_->fill - 1));
	}
}

template < typename T >
void set_metadata_from_id3v1_string(metadata_t &metadata_, char const *field_name, T const &t)
{
	// not using strlen() here, since it does not allow for a hard limit; strnlen() is not standardized
	unsigned int length = 0;
	for (char const *c = &t[0]; length < sizeof(T); ++c, ++length)
	{
		if (c == 0)
			break;
	}

	if (length > 0)
		set_metadata_value(metadata_, field_name, std::string(&t[0], length));
	else
		set_metadata_value(metadata_, field_name, "");
}

}


metadata_t mpg123_decoder::get_metadata() const
{
	metadata_t metadata_ = empty_metadata();

	// NOTE: not using can_playback() here, since it is valid to call get_songinfo() without any playback going on
	if (is_initialized())
	{
		if (id3v2_data != 0)
		{
			set_metadata_from_mpg123_string(metadata_, "title", id3v2_data->title);
			set_metadata_from_mpg123_string(metadata_, "artist", id3v2_data->artist);
			set_metadata_from_mpg123_string(metadata_, "album", id3v2_data->album);
			set_metadata_from_mpg123_string(metadata_, "year", id3v2_data->year);
			set_metadata_from_mpg123_string(metadata_, "genre", id3v2_data->genre);
		}
		else if (id3v1_data != 0)
		{
			set_metadata_from_id3v1_string(metadata_, "title", id3v1_data->title);
			set_metadata_from_id3v1_string(metadata_, "artist", id3v1_data->artist);
			set_metadata_from_id3v1_string(metadata_, "album", id3v1_data->album);
			set_metadata_from_id3v1_string(metadata_, "year", id3v1_data->year);
		}
	}

	return metadata_;
}


std::string mpg123_decoder::get_type() const
{
	return "mpg123";
}


uri mpg123_decoder::get_uri() const
{
	return (source_) ? source_->get_uri() : uri();
}


long mpg123_decoder::get_num_ticks() const
{
	off_t ret = mpg123_length(mpg123_handle_);
	return (ret == MPG123_ERR) ? 0 : long(ret);
}


long mpg123_decoder::get_num_ticks_per_second() const
{
	return src_frequency;
}


void mpg123_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	if (!is_initialized())
		return;

	boost::lock_guard < boost::mutex > lock(mutex_);

	playback_properties_ = new_playback_properties;
}


decoder_properties mpg123_decoder::get_decoder_properties() const
{
	return decoder_properties(src_frequency, src_num_channels, playback_properties_.sample_type_);
}


unsigned int mpg123_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	boost::lock_guard < boost::mutex > lock(mutex_);

	unsigned int size_to_write = num_samples_to_write * src_num_channels * get_sample_size(playback_properties_.sample_type_);


	// If not enough data is stored in the out buffer, fill it until it is full, an error occurs, or reading fails
	while ((out_buffer.size() < size_to_write) && (last_read_return_value != MPG123_ERR))
	{
		size_t decoded_size;
		unsigned int out_buffer_offset = out_buffer.size();
		out_buffer.resize(out_buffer_offset + out_buffer_pad_size); // make room for new output data

		// Check if the last decode call reported a need for more input
		if (last_read_return_value == MPG123_NEED_MORE)
		{
			// mpg123 needs more input data, lets deliver some
			unsigned long read_size = 0;
			if (source_->can_read())
				read_size = source_->read(&in_buffer[0], in_buffer_size);
			if (read_size == 0) // the source could not deliver data, no further input possible
			{
				out_buffer.resize(out_buffer_offset); // revert to old size
				break;
			}

			last_read_return_value = mpg123_decode(mpg123_handle_, &in_buffer[0], read_size, &out_buffer[out_buffer_offset], out_buffer_pad_size, &decoded_size);
		}
		else
		{
			// Currently, no more input data is needed - just decode
			last_read_return_value = mpg123_decode(mpg123_handle_, 0, 0, &out_buffer[out_buffer_offset], out_buffer_pad_size, &decoded_size);
		}

		out_buffer.resize(out_buffer_offset + decoded_size); // most likely not all of the room we've made for new output data is actually used - contract the buffer again
	}


	// If the buffer is empty at this point, then there is no more input data as well - just return zero
	if (out_buffer.size() == 0)
		return 0;


	// Get the size of the decoded data that is actually available for the caller
	unsigned int available_size = std::min(static_cast < unsigned int > (out_buffer.size()), size_to_write);
	// Copy the available data to the caller's buffer, and keep any remaining data in it in case there is more available than the caller requested
	unsigned int remaining_size = out_buffer.size() - available_size;

	std::memcpy(dest, &out_buffer[0], available_size); // Copy to the dest buffer

	if (remaining_size > 0)
	{
		std::memmove(&out_buffer[0], &out_buffer[available_size], remaining_size); // Copy any remaining data to the start of the out buffer
		out_buffer.resize(remaining_size); // Resize the out buffer to the size of the remaining data
	}
	else
		out_buffer.clear(); // no more data remaining - clear the out buffer


	return available_size / (src_num_channels * get_sample_size(playback_properties_.sample_type_)); // Return the actual amount of samples written to dest
}




mpg123_decoder_creator::mpg123_decoder_creator()
{
	mpg123_init();
}


mpg123_decoder_creator::~mpg123_decoder_creator()
{
	mpg123_exit();
}


decoder_ptr_t mpg123_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_event_callback_t const &send_event_callback)
{
	// Testing out whether or not this is an MP3

	bool recognized = false;

	// Try if it starts with an ID3 tag
	if (!recognized)
	{
		source_->reset();

		char buf[3];
		source_->read(buf, 3);
		recognized = (buf[0] == 'I') && (buf[1] == 'D') && (buf[2] == '3');
	}

	// Try if this is a RIFF (AVI) file with MP3 embedded
	{
		source_->reset();

		char buf[4];
		source_->read(buf, 4);
		if ((buf[0] == 'R') && (buf[1] == 'I') && (buf[2] == 'F') && (buf[3] == 'F'))
		{
			source_->seek(4, source::seek_relative);
			source_->read(buf, 4);

			// Either it is MP3 data directly embedded in the RIFF container
			if ((buf[0] == 'R') && (buf[1] == 'M') && (buf[2] == 'P') && (buf[3] == '3'))
				recognized = true;
			else if ((buf[0] == 'W') && (buf[1] == 'A') && (buf[2] == 'V') && (buf[3] == 'E')) // or it is a WAVE stream, which itself is a container
			{
				source_->seek(8, source::seek_relative);
				uint8_t bytes[2];
				source_->read(bytes, 2);
				// the two bytes form up a little-endian signed 16bit value - this value identifies the wave contents; 85 means "MP3"
				recognized = (bytes[0] == 85) && (bytes[1] == 0);
			}
		}
	}

	// Try a different test found in libmagic's animation file for ADTS files
	if (!recognized)
	{
		source_->reset();

		uint8_t bytes[2];
		source_->read(bytes, 2);
		if (bytes[0] == 0xff)
		{
			int second_byte = (bytes[1] & 0xfe);
			switch (second_byte)
			{
				case 0xe2:
				case 0xf2:
				case 0xf4:
				case 0xf6:
				case 0xfa:
				case 0xfc:
					source_->read(&bytes[0], 1);
					bytes[0] >>= 4;
					recognized = (bytes[0] >= 1) && (bytes[0] <= 0xE);
					break;
				default:
					break;
			}
		}
	}
	
	if (!recognized) // All tests failed - it seems this is not an MP3
		return decoder_ptr_t();


	source_->reset();


	mpg123_decoder *mpg123_decoder_ = new mpg123_decoder(send_event_callback, source_);
	if (!mpg123_decoder_->is_initialized())
	{
		delete mpg123_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(mpg123_decoder_);
}


}
}

