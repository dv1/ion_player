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


class playlists_ui;
class scanner;


class main_window:
	public QMainWindow 
{
	Q_OBJECT
public:
	explicit main_window(uri_optional_t const &command_line_uri = boost::none);
	~main_window();


protected slots:
	void play();
	void pause();
	void stop();
	void previous_song();
	void next_song();
	void show_settings();

	void set_current_position();
	void set_current_volume();

	void backend_filepath_filedialog();

	void create_new_playlist();
	void rename_playlist();
	void delete_playlist();

	void add_file_to_playlist();
	void add_folder_contents_to_playlist();
	void add_url_to_playlist();
	void remove_selected_from_playlist();

	void try_read_stdout_line();
	void backend_started();
	void backend_error(QProcess::ProcessError process_error);
	void backend_finished(int exit_code, QProcess::ExitStatus exit_status);

	void get_current_playback_position();


protected:
	void start_backend();
	void stop_backend();
	void change_backend();

	void print_backend_line(std::string const &line);

	std::string get_playlists_filename();
	bool load_playlists();
	void save_playlists();

	void apply_flags();

	void current_uri_changed(uri_optional_t const &new_current_uri);
	void current_metadata_changed(metadata_optional_t const &new_metadata);


	typedef boost::shared_ptr < audio_frontend > audio_frontend_ptr_t;
	audio_frontend_ptr_t audio_frontend_;

	QProcess *backend_process;
	scanner *scanner_;
	QTimer *get_current_position_timer;

	playlists_ui *playlists_ui_;
	settings *settings_;
	QDialog *settings_dialog;

	Ui_main_window main_window_ui;
	Ui_position_volume_widget position_volume_widget_ui;
	Ui_settings_dialog settings_dialog_ui;
};


}
}


#endif

