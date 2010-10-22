#include <QWebFrame>
#include <QWebPage>
#include <assert.h>
#include <json/reader.h>
#include <json/writer.h>
#include "configuration_html_page_controller.hpp"
#include "variant_to_json.hpp"


namespace ion
{
namespace audio_player
{


configuration_html_page_controller::configuration_html_page_controller(
	QObject *parent,
	QWebPage *web_page,
	get_properties_callback_t const &get_properties_callback,
	set_properties_callback_t const &set_properties_callback,
	QString const &javascript_property_variable_name
):
	QObject(parent),
	web_page(web_page),
	get_properties_callback(get_properties_callback),
	set_properties_callback(set_properties_callback),
	javascript_property_variable_name(javascript_property_variable_name)
{
	assert(!javascript_property_variable_name.isNull());
	assert(!javascript_property_variable_name.isEmpty());
	assert(web_page != 0);
	assert(get_properties_callback);
	assert(set_properties_callback);

	connect_slots();
}


configuration_html_page_controller::~configuration_html_page_controller()
{
	disconnect_slots();
}


void configuration_html_page_controller::connect_slots()
{
	connect(web_page->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populate_javascript()));
}


void configuration_html_page_controller::disconnect_slots()
{
	disconnect(web_page->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populate_javascript()));
}


void configuration_html_page_controller::set_web_page(QWebPage *new_web_page)
{
	disconnect_slots();
	web_page = new_web_page;
	connect_slots();
}


void configuration_html_page_controller::set_web_page(QWebPage *new_web_page, QString const &new_javascript_property_variable_name)
{
	assert(!new_javascript_property_variable_name.isNull());
	assert(!new_javascript_property_variable_name.isEmpty());
	assert(web_page != 0);

	set_web_page(new_web_page);
	javascript_property_variable_name = new_javascript_property_variable_name;
}


void configuration_html_page_controller::populate_javascript()
{
	Json::Value properties = get_properties_callback();
	if (!properties.isObject())
		return;

	// This transmits ui properties to WebKit; uiProperties becomes a global variable in the Javascript
	web_page->mainFrame()->evaluateJavaScript(javascript_property_variable_name + " = " + Json::FastWriter().write(properties).c_str() + ";");
}


void configuration_html_page_controller::apply_configuration()
{
	Json::Value properties = get_properties_from_page();
	if (properties.isObject())
		set_properties_callback(properties);
}


Json::Value configuration_html_page_controller::get_properties_from_page() const
{
	QVariant properties_variant = web_page->mainFrame()->evaluateJavaScript(javascript_property_variable_name);
	Json::Value properties;

	if (properties_variant.isValid())
		properties = variant_to_json(properties_variant);

	return properties;
}


}
}


#include "configuration_html_page_controller.moc"

