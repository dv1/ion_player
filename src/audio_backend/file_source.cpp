#include "file_source.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


file_source::file_source(uri const &uri_):
	uri_(uri_)
{
	file.open(uri_.get_path().c_str(), std::ios::binary);
}


void file_source::reset()
{
	file.close();
	file.open(uri_.get_path().c_str(), std::ios::binary);
}


long file_source::read(void *dest, long const num_bytes)
{
	if (file.good())
	{
		file.read(reinterpret_cast < char* > (dest), num_bytes);
		return file.gcount();
	}
	else
		return 0;
}


bool file_source::can_read() const
{
	return file.good();
}


bool file_source::end_of_data_reached() const
{
	return file.eof();
}


bool file_source::is_ok() const
{
	return !file.fail();
}


void file_source::seek(long const num_bytes, seek_type const type)
{
	std::ios_base::seekdir dir = std::ios::beg;

	switch (type)
	{
		case seek_absolute: dir = std::ios::beg; break;
		case seek_relative: dir = std::ios::cur; break;
		case seek_from_end: dir = std::ios::end; break;
		default: break;
	}

	file.seekg(num_bytes, dir);
}


bool file_source::can_seek(seek_type const) const
{
	return true;
}


long file_source::get_position() const
{
	return file.tellg();
}


long file_source::get_size() const
{
	std::streampos old_pos = file.tellg();
	file.seekg(0, std::ios::end);
	std::streampos new_pos = file.tellg();
	file.seekg(old_pos, std::ios::beg);
	return long(new_pos);
}




source_ptr_t file_source_creator::create(ion::uri const &uri_)
{
	std::string filename = uri_.get_path();

	std::ifstream f(filename.c_str());
	bool exists = f.good();
	f.close();

	if (exists)
		return source_ptr_t(new file_source(uri_));
	else
		return source_ptr_t();
}


}
}

