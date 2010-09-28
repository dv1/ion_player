/****************************************************************************

Copyright (c) 2010 Carlos Rafael Giani

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

****************************************************************************/


#include <json/reader.h>
#include <json/writer.h>
#include "metadata.hpp"


namespace ion
{


metadata_t const & empty_metadata()
{
	static metadata_t const empty_metadata_(Json::objectValue);
	return empty_metadata_;
}


std::string const & empty_metadata_string()
{
	static std::string const str("{}");
	return str;
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


bool has_metadata_value(metadata_t const &metadata, std::string const &value_name)
{
	return metadata.isMember(value_name);
}


}

