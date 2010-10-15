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


#ifndef ION_AUDIO_BACKEND_DUMB_DECODER_HPP
#define ION_AUDIO_BACKEND_DUMB_DECODER_HPP

#include <boost/thread/mutex.hpp>
#include <dumb.h>
#include "source.hpp"
#include "decoder.hpp"
#include "decoder_creator.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


class dumb_decoder:
	public decoder
{
public:
	enum module_type
	{
		module_type_unknown,

		module_type_xm,
		module_type_it,
		module_type_s3m,
		module_type_mod
	};


	explicit dumb_decoder(send_event_callback_t const send_event_callback, source_ptr_t source_, long const filesize, module_type const module_type_);
	~dumb_decoder();


	virtual bool is_initialized() const;
	virtual bool can_playback() const;

	virtual long set_current_position(long const new_position);
	virtual long get_current_position() const;

	virtual metadata_t get_metadata() const;

	virtual std::string get_type() const;

	virtual uri get_uri() const;

	virtual long get_num_ticks() const;
	virtual long get_num_ticks_per_second() const;

	virtual void set_loop_mode(int const new_loop_mode);

	virtual void set_playback_properties(playback_properties const &new_playback_properties);

	virtual decoder_properties get_decoder_properties() const;

	virtual unsigned int update(void *dest, unsigned int const num_samples_to_write);


	struct loop_data
	{
		long loop_mode, cur_num_loops;
	};


protected:
	void reinitialize_sigrenderer(unsigned const int new_num_channels, long const new_position);
	void set_loop_mode_impl(int const new_loop_mode);


	mutable boost::mutex mutex_;
	DUH *duh;
	DUH_SIGRENDERER *duh_sigrenderer;
	playback_properties playback_properties_;
	module_type module_type_;
	loop_data loop_data_;
	source_ptr_t source_;
};




class dumb_decoder_creator:
	public decoder_creator
{
public:
	explicit dumb_decoder_creator();

	virtual decoder_ptr_t create(source_ptr_t source_, metadata_t const &metadata, send_event_callback_t const &send_event_callback);
	virtual std::string get_type() const { return "dumb"; }

protected:
	dumb_decoder::module_type test_if_module_file(source_ptr_t source_);


	DUMBFILE_SYSTEM fs;
};


}
}


#endif

