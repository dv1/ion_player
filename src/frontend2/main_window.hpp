#ifndef ION_FRONTEND_QT4_MAIN_WINDOW_HPP
#define ION_FRONTEND_QT4_MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QProcess>

#include "ui_main_window.h"
#include "ui_position_volume_widget.h"
#include "ui_settings.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <ion/uri.hpp>

#include "settings.hpp"
#include "audio_frontend.hpp"


namespace ion
{
namespace frontend
{


class playlists;
class scanner;


class main_window:
	public QMainWindow 
{
	Q_OBJECT
public:
	explicit main_window(uri_optional_t const &command_line_uri = boost::none);
	~main_window();


protected:
	typedef boost::shared_ptr < audio_frontend > audio_frontend_ptr_t;
	audio_frontend_ptr_t audio_frontend_;
	QProcess *backend_process;
	scanner *scanner_;
	playlists *playlists_;
	settings *settings_;

	QDialog *settings_dialog;
	QTimer *get_current_position_timer;

	Ui_main_window main_window_ui;
	Ui_position_volume_widget position_volume_widget_ui;
	Ui_settings_dialog settings_dialog_ui;
};


}
}


#endif

