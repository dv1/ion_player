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


#ifndef ION_AUDIO_BACKEND_UADE_DECODER_HPP
#define ION_AUDIO_BACKEND_UADE_DECODER_HPP

#include "decoder.hpp"
#include <boost/thread/mutex.hpp>
#include "decoder_creator.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


class uade_decoder:
	public decoder
{
public:
	explicit uade_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_, metadata_t const &initial_metadata);
	~uade_decoder();


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
	mutable boost::mutex mutex_;
	source_ptr_t source_;
	long subsong_nr;
	playback_properties playback_properties_;

	struct internal_data;
	internal_data *internal_data_;
};




class uade_decoder_creator:
	public decoder_creator
{
public:
	explicit uade_decoder_creator();

	virtual decoder_ptr_t create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback);
	virtual std::string get_type() const { return "uade"; }
};


}
}


#endif

