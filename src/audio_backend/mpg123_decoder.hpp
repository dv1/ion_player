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


#ifndef ION_AUDIO_BACKEND_MPG123_DECODER_HPP
#define ION_AUDIO_BACKEND_MPG123_DECODER_HPP

#include <stdint.h>
#include <vector>
#include <mpg123.h>
#include <boost/thread/mutex.hpp>

#include "source.hpp"
#include "decoder.hpp"
#include "decoder_creator.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


class mpg123_decoder:
	public decoder
{
public:
	explicit mpg123_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_);
	~mpg123_decoder();


	virtual bool is_initialized() const;
	virtual bool can_playback() const;

	virtual void pause();
	virtual void resume();

	virtual long set_current_position(long const new_position);
	virtual long get_current_position() const;

	virtual long set_current_volume(long const new_volume);
	virtual long get_current_volume() const;

	virtual metadata_t get_metadata() const;

	virtual std::string get_type() const;

	virtual uri get_uri() const;

	virtual long get_num_ticks() const;
	virtual long get_num_ticks_per_second() const;

	virtual void set_loop_mode(int const new_loop_mode);

	virtual void set_playback_properties(playback_properties const &new_playback_properties);

	virtual unsigned int get_decoder_samplerate() const;

	virtual unsigned int update(void *dest, unsigned int const num_samples_to_write);


protected:
	typedef std::vector < uint8_t > buffer_t;

	mutable boost::mutex mutex_;
	playback_properties playback_properties_;
	unsigned int src_frequency, src_num_channels;
	source_ptr_t source_;
	mpg123_handle *mpg123_handle_;
	mpg123_id3v1 *id3v1_data;
	mpg123_id3v2 *id3v2_data;
	buffer_t in_buffer, out_buffer;
	long current_volume;
	unsigned long const in_buffer_size, out_buffer_pad_size;
	int last_read_return_value;
	bool has_song_length;
};




class mpg123_decoder_creator:
	public decoder_creator
{
public:
	explicit mpg123_decoder_creator();
	~mpg123_decoder_creator();

	virtual decoder_ptr_t create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback, std::string const &mime_type);
	virtual std::string get_type() const { return "mpg123"; }
};


}
}


#endif

