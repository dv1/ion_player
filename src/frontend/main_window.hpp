#ifndef ION_FRONTEND_QT4_MAIN_WINDOW_HPP
#define ION_FRONTEND_QT4_MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QProcess>
#include "ui_main_window.h"
#include "ui_position_volume_widget.h"
#include "ui_settings.h"
#include "settings.hpp"
#include "audio_frontend_io.hpp"


namespace ion
{
namespace frontend
{


class playlists;


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
	void set_current_volume(int new_volume);

	void backend_filepath_filedialog();

	void create_new_playlist();

	void add_file_to_playlist();
	void add_folder_contents_to_playlist();
	void add_url_to_playlist();
	void remove_selected_from_playlist();

	void try_read_stdout_line();
	void backend_started();
	void backend_error(QProcess::ProcessError process_error);
	void backend_finished(int exit_code, QProcess::ExitStatus exit_status);


protected:
	void start_backend();
	void stop_backend();
	void change_backend();

	// used when the backend is not initialized properly (the settings button is never disabled!)
	void disable_gui();
	void enable_gui();

	void print_backend_line(std::string const &line);

	std::string get_playlists_filename();
	bool load_playlists();
	void save_playlists();


	typedef boost::shared_ptr < audio_frontend_io > frontend_io_ptr_t;
	frontend_io_ptr_t audio_frontend_io_;
	QProcess *backend_process;
	playlists *playlists_;
	settings *settings_;

	QDialog *settings_dialog;

	Ui_main_window main_window_ui;
	Ui_position_volume_widget position_volume_widget_ui;
	Ui_settings_dialog settings_dialog_ui;
};


}
}


#endif

