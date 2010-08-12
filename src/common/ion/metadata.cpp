#include <json/reader.h>
#include <json/writer.h>
#include "metadata.hpp"


namespace ion
{


metadata_t const & empty_metadata()
{
	static metadata_t empty_metadata_(Json::objectValue);
	return empty_metadata_;
}


bool is_valid(metadata_t const &metadata)
{
	return metadata.isObject();
}


metadata_optional_t parse_metadata(std::string const &metadata_str)
{
	if (metadata_str.empty())
		return empty_metadata();

	Json::Value value;
	Json::Reader reader;
	if (reader.parse(metadata_str, value))
	{
		if (is_valid(value))
			return value;
	}

	return boost::none;
}


std::string get_metadata_string(metadata_t const &metadata)
{
	Json::StyledWriter writer;
	return writer.write(metadata);
}


}

