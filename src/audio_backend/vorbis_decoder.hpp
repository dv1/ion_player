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


#ifndef ION_AUDIO_BACKEND_VORBIS_DECODER_HPP
#define ION_AUDIO_BACKEND_VORBIS_DECODER_HPP


#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>

#include <boost/thread/mutex.hpp>

#include "decoder.hpp"
#include "decoder_creator.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


class vorbis_decoder:
	public decoder
{
public:
	explicit vorbis_decoder(send_event_callback_t const send_event_callback, source_ptr_t source_);
	~vorbis_decoder();


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


protected:
	mutable boost::mutex mutex_;
	source_ptr_t source_;
	mutable OggVorbis_File vorbis_file;
	vorbis_info *info;
	int current_section;
	bool initialized;
	playback_properties playback_properties_;
};




class vorbis_decoder_creator:
	public decoder_creator
{
public:
	explicit vorbis_decoder_creator();

	virtual decoder_ptr_t create(source_ptr_t source_, metadata_t const &metadata, send_event_callback_t const &send_event_callback);
	virtual std::string get_type() const { return "vorbis"; }
};


}
}


#endif

