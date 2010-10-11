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


#ifndef ION_AUDIO_BACKEND_FAAD_DECODER_HPP
#define ION_AUDIO_BACKEND_FAAD_DECODER_HPP

#include <stdint.h>
#include <vector>

#include <ion_config.h>

#ifdef HAVE_NEAACDEC_H
#include <neaacdec.h>
#elif HAVE_FAAD_H
#include <faad.h>
#else
#error No FAAD header found during build configuration
#endif

#include <boost/optional.hpp>
#include <boost/thread/mutex.hpp>

#include "decoder.hpp"
#include "decoder_creator.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


class faad_decoder:
	public decoder
{
public:
	explicit faad_decoder(send_event_callback_t const send_event_callback, source_ptr_t source_);
	~faad_decoder();


	virtual bool is_initialized() const;
	virtual bool can_playback() const;

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

	virtual decoder_properties get_decoder_properties() const;

	virtual unsigned int update(void *dest, unsigned int const num_samples_to_write);


protected:
	typedef boost::optional < faacDecHandle > faad_handle_optional_t;
	typedef std::vector < uint8_t > buffer_t;

	bool initialize(unsigned int const frequency);
	void close();


	mutable boost::mutex mutex_;
	source_ptr_t source_;
	faad_handle_optional_t faad_handle;
	bool initialized;
	playback_properties playback_properties_;
	unsigned int decoder_sample_rate;
	buffer_t in_buffer, out_buffer;
	long current_position;

	unsigned long sample_rate;
	unsigned char channels;
};




class faad_decoder_creator:
	public decoder_creator
{
public:
	explicit faad_decoder_creator();

	virtual decoder_ptr_t create(source_ptr_t source_, metadata_t const &metadata, send_event_callback_t const &send_event_callback);
	virtual std::string get_type() const { return "faad"; }
};


}
}


#endif

