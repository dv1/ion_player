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


#ifndef ION_AUDIO_BACKEND_BACKEND_FILE_SOURCE_HPP
#define ION_AUDIO_BACKEND_BACKEND_FILE_SOURCE_HPP

#include <fstream>
#include <ion/uri.hpp>
#include "source.hpp"
#include "source_creator.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


class file_source:
	public source
{
public:
	explicit file_source(uri const &uri_);


	virtual void reset();

	virtual long read(void *dest, long const num_bytes);
	virtual bool can_read() const;
	virtual bool end_of_data_reached() const;
	virtual bool is_ok() const;

	virtual void seek(long const num_bytes, seek_type const type);
	virtual bool can_seek(seek_type const type) const;

	virtual long get_position() const;
	virtual long get_size() const;

	virtual uri get_uri() const { return uri_; }

protected:
	mutable std::ifstream file;
	uri uri_;
};




class file_source_creator:
	public source_creator
{
public:
	virtual source_ptr_t create(ion::uri const &uri_);
	virtual std::string get_type() const { return "file"; }
};


}
}


#endif

