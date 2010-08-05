#ifndef ION_FRONTEND_QT4_MAIN_WINDOW_HPP
#define ION_FRONTEND_QT4_MAIN_WINDOW_HPP

#include <QMainWindow>
#include "ui_main_window.h"
#include "ui_position_volume_widget.h"
#include "ui_settings.h"
#include "settings.hpp"


class QProcess;


namespace ion
{
namespace frontend
{


class main_window:
	public QMainWindow 
{
	Q_OBJECT
public:
	explicit main_window();
	~main_window();


protected slots:
	void play();
	void pause();
	void stop();
	void previous_song();
	void next_song();
	void show_settings();

	void set_current_position(int new_position);
	void set_current_voume(int new_volume);

	void backend_filepath_filedialog();


protected:
	void start_backend();
	void stop_backend();
	void change_backend();

	// used when the backend is not initialized properly (the settings button is never disabled!)
	void disable_gui();
	void enable_gui();


	QProcess *backend_process;
	//playlists playlists_;
	settings *settings_;

	QDialog *settings_dialog;

	Ui_main_window main_window_ui;
	Ui_position_volume_widget position_volume_widget_ui;
	Ui_settings_dialog settings_dialog_ui;
};


}
}


#endif

