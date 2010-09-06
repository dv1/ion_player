#ifndef ION_FRONTEND_QT4_MAIN_WINDOW_HPP
#define ION_FRONTEND_QT4_MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QProcess>
#include <QTimer>
#include <QDirIterator>

#include "ui_main_window.h"
#include "ui_position_volume_widget.h"
#include "ui_settings.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <ion/uri.hpp>
#include <ion/flat_playlist.hpp>

#include "settings.hpp"
#include "audio_frontend.hpp"
#include "misc_types.hpp"


class QLabel;
class QMovie;


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

	void move_to_currently_playing();

	void show_settings();

	void set_current_position();
	void set_current_volume();

	void open_backend_filepath_filedialog();

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

	void scan_running(bool state);

	void get_current_playback_position();

	void scan_directory();


protected:
	void start_backend(bool const start_scanner = true);
	void stop_backend(bool const send_quit_message = true, bool const stop_scanner = true, bool const send_signals = true);
	void change_backend();

	void print_backend_line(std::string const &line);

	std::string get_playlists_filename();
	bool load_playlists();
	void save_playlists();

	void apply_flags();

	void current_uri_changed(uri_optional_t const &new_current_uri);
	void current_metadata_changed(metadata_optional_t const &new_metadata);

	void set_current_time_label(unsigned int const current_position);
	void set_label_time(QLabel *label, int const minutes, int const seconds);


	typedef boost::shared_ptr < audio_frontend > audio_frontend_ptr_t;
	audio_frontend_ptr_t audio_frontend_;

	// TODO: put the unique_ids instance in the playlist class, as a flyweight (!), to guarantee that only one instance exists for all playlists
	// do not put it in flat_playlist, since the unique_ids instance should be used in *all* playlists, even non-flat ones (but not in filter/view playlists)
	flat_playlist::unique_ids_t unique_ids_;

	QProcess *backend_process;
	scanner *scanner_;
	QTimer *current_position_timer;
	QTimer scan_directory_timer;

	typedef boost::shared_ptr < QDirIterator > dir_iterator_ptr_t;
	dir_iterator_ptr_t dir_iterator;
	playlist *dir_iterator_playlist;

	playlists_ui *playlists_ui_;
	settings *settings_;
	QDialog *settings_dialog;

	QLabel *current_song_title, *current_playback_time, *current_song_length, *current_scan_status;
	QMovie *busy_indicator;
	unsigned int current_num_ticks_per_second;

	Ui_main_window main_window_ui;
	Ui_position_volume_widget position_volume_widget_ui;
	Ui_settings_dialog settings_dialog_ui;
};


}
}


#endif

