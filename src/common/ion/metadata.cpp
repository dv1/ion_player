#include <json/reader.h>
#include "metadata.hpp"


namespace ion
{


metadata_optional_t parse_metadata(std::string const &metadata_str)
{
	if (metadata_str.empty())
		return Json::Value(Json::objectValue);

	Json::Value value;
	Json::Reader reader;
	if (reader.parse(metadata_str, value))
	{
		if (value.isObject())
			return value;
	}

	return boost::none;
}


}

