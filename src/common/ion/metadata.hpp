#ifndef ION_METADATA_HPP
#define ION_METADATA_HPP

#include <boost/mpl/map.hpp>
#include <boost/mpl/at.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <json/value.h>


namespace ion
{


typedef Json::Value metadata_t;
typedef boost::optional < metadata_t > metadata_optional_t;

metadata_t const & empty_metadata();
bool is_valid(metadata_t const &metadata);
metadata_optional_t parse_metadata(std::string const &metadata_str);
std::string get_metadata_string(metadata_t const &metadata);


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


}


#endif

