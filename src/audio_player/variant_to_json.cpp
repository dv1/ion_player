#include <boost/foreach.hpp>
#include "variant_to_json.hpp"


namespace ion
{
namespace audio_player
{


namespace
{


Json::Value variant_to_json_impl(QVariant const &variant);
Json::Value variant_to_json_impl(QVariantList const &variant_list);
Json::Value variant_to_json_impl(QVariantMap const &variant_map);


Json::Value variant_to_json_impl(QVariant const &variant)
{
	switch (variant.type())
	{
		case QVariant::Int: return Json::Value(variant.toInt());
		case QVariant::UInt: return Json::Value(variant.toUInt());
		case QVariant::Bool: return Json::Value(variant.toBool());
		case QVariant::Char: return Json::Value(std::string("") + variant.toChar().toAscii());
		case QVariant::Double: return Json::Value(variant.toDouble());
		case QVariant::String: return Json::Value(variant.toString().toStdString());
		case QVariant::LongLong: return Json::Value(int(variant.toLongLong()));
		case QVariant::ULongLong: return Json::Value(int(variant.toULongLong()));

		case QVariant::List: return variant_to_json_impl(variant.toList());
		case QVariant::Map: return variant_to_json_impl(variant.toMap());

		default: return Json::Value();
	}
}


Json::Value variant_to_json_impl(QVariantList const &variant_list)
{
	Json::Value array_value(Json::arrayValue);
	BOOST_FOREACH(QVariant const &variant, variant_list)
	{
		Json::Value array_element = variant_to_json_impl(variant);
		array_value.append(array_element);
	}
	return array_value;
}


Json::Value variant_to_json_impl(QVariantMap const &variant_map)
{
	Json::Value object_value(Json::objectValue);
	for (QVariantMap::const_iterator iter = variant_map.begin(); iter != variant_map.end(); ++iter)
	{
		Json::Value object_element = variant_to_json_impl(iter.value());
		object_value[iter.key().toStdString()] = object_element;
	}
	return object_value;
}


}


Json::Value variant_to_json(QVariant const &variant)
{
	return variant_to_json_impl(variant);
}


}
}

