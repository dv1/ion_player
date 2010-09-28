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

