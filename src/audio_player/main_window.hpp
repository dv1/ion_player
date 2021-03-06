/**************************************************************************

    Copyright (C) 2010  Carlos Rafael Giani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**************************************************************************/


#ifndef ION_AUDIO_PLAYER_MAIN_WINDOW_HPP
#define ION_AUDIO_PLAYER_MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QProcess>

#include "ui_main_window.h"
#include "ui_position_volume_widget.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <ion/uri.hpp>
#include <ion/flat_playlist.hpp>

#include "settings.hpp"
#include "ui_settings.hpp"
#include "audio_frontend.hpp"
#include "settings_dialog.hpp"
#include "search_dialog.hpp"
#include "misc_types.hpp"


class QLabel;
class QTimer;


namespace ion
{
namespace audio_player
{


class playlists_ui;
class scanner;
class logger_dialog;
class scan_dialog;
class scan_indicator_icon;
class status_bar_ui;


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

	void create_new_playlist();
	void create_new_all_playlist();
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

	void backend_timeout();

	void visible_playlist_changed(int page_index);
	void set_playlist_repeating(bool state);


protected:
	void start_backend(bool const start_scanner = true);
	void stop_backend(bool const send_quit_message = true, bool const stop_scanner = true, bool const send_signals = true);
	void change_backend();

	void print_backend_line(std::string const &line);
	void handle_backend_pong();

	std::string get_playlists_filename();
	bool load_playlists();
	void save_playlists();

	void apply_flags();

	void current_uri_changed(uri_optional_t const &new_current_uri);
	void current_metadata_changed(metadata_optional_t const &new_metadata, bool const set_slider_value);
	void handle_new_metadata(uri const &uri_, metadata_t const &new_metadata);

	void playlist_removed(playlist &playlist_);


	typedef boost::shared_ptr < audio_common::audio_frontend > audio_frontend_ptr_t;
	audio_frontend_ptr_t audio_frontend_;

	// TODO: put the unique_ids instance in the playlist class, as a flyweight (!), to guarantee that only one instance exists for all playlists
	// do not put it in flat_playlist, since the unique_ids instance should be used in *all* playlists, even non-flat ones (but not in filter/view playlists)
	flat_playlist::unique_ids_t unique_ids_;

	QProcess *backend_process;
	scanner *scanner_;
	QTimer *current_position_timer, *backend_timeout_timer;

	enum backend_timeout_modes
	{
		backend_timeout_normal,
		backend_timeout_terminating,
		backend_timeout_killing
	};
	backend_timeout_modes backend_timeout_mode;
	bool pong_received;

	playlists_ui *playlists_ui_;

	settings *settings_;
	ui_settings *ui_settings_;
	settings_dialog *settings_dialog_;

	search_dialog *search_dialog_;

	logger_dialog *backend_log_dialog_;

	scan_dialog *scan_dialog_;

	status_bar_ui *status_bar_ui_;

	Ui_main_window main_window_ui;
	Ui_position_volume_widget position_volume_widget_ui;
};


}
}


#endif

