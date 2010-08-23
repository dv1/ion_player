#ifndef ION_METADATA_HPP
#define ION_METADATA_HPP

#include <boost/mpl/map.hpp>
#include <boost/mpl/at.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <json/value.h>


namespace ion
{


/*
Metadata types and functions. In Ion, resources can (and usually have) metadata, like length, title, artist ..
The metadata structure is convertible from/to a JSON object. In fact, in this implementation, the metadata_t type *is* a JSON value
(using jsoncpp from http://jsoncpp.sf.net). In any way, the metadata_t type is always an associative container.

A default-constructed metadata_t type is set to a null value, meaning its invalid. An empty JSON object is available by calling empty_metadata().
So, to get a new empty JSON object, one uses code like this:   metadata_t new_metadata = empty_metadata();

To convert metadata to a string containing a JSON object representation, the get_metadata_string() function is used. Its counterpart,
parse_metadata(), gets a JSON string, parses it, and outputs a metadata value. However, it outputs metadata_optional_t, not metadata_t;
metadata_optional_t is a boost::optional <> version of metadata. If parsing fails, this result is set to boost::none.

To avoid unnecessary conversion work for empty objects, it is recommended to call empty_metadata_string() to get a JSON string representation of an empty object.


TODO: since metadata_optional_t already allows for invalid values using boost::none, it is unnecessary to allow metadata_t to be a nullvalue (= an invalid value).
Unfortunately, jsoncpp's Json::Value default constructor sets the value type to nullValue. Therefore, replace the metadata_t typedef with an inherited class;
inherit from Json::Value, and use objectValue in the default constructor instead. (Don't forget to inherit all Json::Value constructors!) This also makes
empty_metadata() and empty_metadata_string() obsolete.
*/


typedef Json::Value metadata_t;
typedef boost::optional < metadata_t > metadata_optional_t;

metadata_t const & empty_metadata();
std::string const & empty_metadata_string();
bool is_valid(metadata_t const &metadata);
metadata_optional_t parse_metadata(std::string const &metadata_str);
std::string get_metadata_string(metadata_t const &metadata);
bool has_metadata_value(metadata_t const &metadata, std::string const &value_name);



// The following code implements a get_metadata_value() function that is used for retrieving and storing metadata contents.
// As mentioned above, metadata_t is an associative container. The get/set_metadata_value() function access the container's values.


namespace detail
{

template < typename T >
struct convert_json_value;

template < > struct convert_json_value < short > { short operator()(Json::Value const &value) const { return value.asInt(); } };
template < > struct convert_json_value < int >   { int operator()(Json::Value const &value) const { return value.asInt(); } };
template < > struct convert_json_value < long >  { long operator()(Json::Value const &value) const { return value.asInt(); } };
template < > struct convert_json_value < unsigned short > { unsigned short operator()(Json::Value const &value) const { return value.asUInt(); } };
template < > struct convert_json_value < unsigned int >   { unsigned int operator()(Json::Value const &value) const { return value.asUInt(); } };
template < > struct convert_json_value < unsigned long >  { unsigned long operator()(Json::Value const &value) const { return value.asUInt(); } };
template < > struct convert_json_value < float > { float operator()(Json::Value const &value) const { return value.asDouble(); } };
template < > struct convert_json_value < double > { double operator()(Json::Value const &value) const { return value.asDouble(); } };
template < > struct convert_json_value < std::string > { std::string operator()(Json::Value const &value) const { return value.asString(); } };
template < > struct convert_json_value < bool > { bool operator()(Json::Value const &value) const { return value.asBool(); } };

typedef boost::mpl::map <
	boost::mpl::pair < short, boost::mpl::integral_c < Json::ValueType, Json::intValue > >,
	boost::mpl::pair < int, boost::mpl::integral_c < Json::ValueType, Json::intValue > >,
	boost::mpl::pair < long, boost::mpl::integral_c < Json::ValueType, Json::intValue > >,
	boost::mpl::pair < unsigned short, boost::mpl::integral_c < Json::ValueType, Json::uintValue > >,
	boost::mpl::pair < unsigned int, boost::mpl::integral_c < Json::ValueType, Json::uintValue > >,
	boost::mpl::pair < unsigned long, boost::mpl::integral_c < Json::ValueType, Json::uintValue > >,
	boost::mpl::pair < float, boost::mpl::integral_c < Json::ValueType, Json::realValue > >,
	boost::mpl::pair < double, boost::mpl::integral_c < Json::ValueType, Json::realValue > >,
	boost::mpl::pair < std::string, boost::mpl::integral_c < Json::ValueType, Json::stringValue > >,
	boost::mpl::pair < bool, boost::mpl::integral_c < Json::ValueType, Json::booleanValue > >
> json_value_type_map_t;

}


// The default_value parameter is used in case the value with the given name does not exist or is not convertible to the T type.
template < typename T >
T get_metadata_value(metadata_t const &metadata, std::string const &value_name, T const &default_value)
{
	if (!metadata.isObject())
		return default_value;

	try
	{
		Json::Value const &json_value = metadata[value_name];
		if (json_value.isNull())
			return default_value;

		Json::ValueType const type_specifier = boost::mpl::at < detail::json_value_type_map_t, T > ::type::value;
		if (!json_value.isConvertibleTo(type_specifier))
			return default_value;

		return detail::convert_json_value < T > ()(json_value);
	}
	catch (std::exception const &)
	{
		return default_value;
	}
}


template < typename T >
void set_metadata_value(metadata_t &metadata, std::string const &value_name, T const &new_value)
{
	metadata[value_name] = new_value;
}


}


#endif

