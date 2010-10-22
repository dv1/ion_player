#ifndef ION_AUDIO_PLAYER_CONFIGURATION_HTML_PAGE_CONTROLLER_HPP
#define ION_AUDIO_PLAYER_CONFIGURATION_HTML_PAGE_CONTROLLER_HPP

#include <QObject>
#include <boost/function.hpp>
#include <json/value.h>


class QWebPage;


namespace ion
{
namespace audio_player
{


class configuration_html_page_controller:
	public QObject
{
	Q_OBJECT
public:
	typedef boost::function < Json::Value () > get_properties_callback_t;
	typedef boost::function < void(Json::Value const &) > set_properties_callback_t;

	explicit configuration_html_page_controller(
		QObject *parent,
		QWebPage *web_page,
		get_properties_callback_t const &get_properties_callback,
		set_properties_callback_t const &set_properties_callback,
		QString const &javascript_property_variable_name = "configuration_properties"
	);
	~configuration_html_page_controller();
	void set_web_page(QWebPage *new_web_page, QString const &new_javascript_property_variable_name);


public slots:
	void set_web_page(QWebPage *new_web_page);
	void apply_configuration();


protected slots:
	void populate_javascript();


protected:
	void connect_slots();
	void disconnect_slots();
	Json::Value get_properties_from_page() const;


	QWebPage *web_page;
	get_properties_callback_t get_properties_callback;
	set_properties_callback_t set_properties_callback;
	QString javascript_property_variable_name;
};


}
}


#endif

