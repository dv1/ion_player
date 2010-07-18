#ifndef ION_METADATA_HPP
#define ION_METADATA_HPP

#include <boost/optional.hpp>
#include <json/value.h>


namespace ion
{


typedef Json::Value metadata_t;
typedef boost::optional < metadata_t > metadata_optional_t;

metadata_optional_t parse_metadata(std::string const &metadata_str);


}


#endif

