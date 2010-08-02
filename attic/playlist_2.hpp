#ifndef ION_PLAYLIST_IO_HPP
#define ION_PLAYLIST_IO_HPP

#include <boost/optional.hpp>
#include <ion/uri.hpp>
#include "metadata.hpp"


namespace ion
{


class playlist
{
public:
	virtual ~playlist() {}
	virtual uri_optional_t get_succeeding_uri(uri const &uri_) const = 0;
	virtual metadata_optional_t get_metadata_for(uri const &uri_) const = 0;
	virtual void backend_resource_incompatibility(std::string const &backend_type, uri const &uri_) = 0;
};


}


#endif

