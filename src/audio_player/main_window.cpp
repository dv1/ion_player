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


#include <ctime>
#include <cstdlib>
#include <fstream>

#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QProcess>
#include <QMetaType>
#include <QTimer>

#include <boost/function.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <boost/spirit/home/phoenix/object/new.hpp>
#include <boost/spirit/home/phoenix/core/reference.hpp>

#include <json/reader.h>
#include <json/writer.h>

#include "main_window.hpp"
#include "playlists_ui.hpp"
#include "scanner.hpp"
#include "logger_dialog.hpp"
#include "scan_dialog.hpp"
#include "scan_indicator_icon.hpp"
#include "status_bar_ui.hpp"

#include "../audio_common/sink.hpp"


namespace ion
{
namespace audio_player
{


main_window::main_window(uri_optional_t const &command_line_uri):
	backend_process(0),
	scanner_(0)
{
	std::srand(std::time(0));

	qRegisterMetaType < QProcess::ProcessError > ("QProcess::ProcessError");
	qRegisterMetaType < QProcess::ExitStatus > ("QProcess::ExitStatus");

	main_window_ui.setupUi(this);

	QWidget *sliders_widget = new QWidget(this);
	position_volume_widget_ui.setupUi(sliders_widget);
	main_window_ui.sliders_toolbar->addWidget(sliders_widget);

	QToolButton *create_playlist_button = new QToolButton(this);
	create_playlist_button->setDefaultAction(main_window_ui.action_create_new_tab);
	main_window_ui.playlist_tab_widget->setCornerWidget(create_playlist_button);

	connect(main_window_ui.action_play,                SIGNAL(triggered()), this, SLOT(play()));
	connect(main_window_ui.action_pause,               SIGNAL(triggered()), this, SLOT(pause()));
	connect(main_window_ui.action_stop,                SIGNAL(triggered()), this, SLOT(stop()));
	connect(main_window_ui.action_previous_song,       SIGNAL(triggered()), this, SLOT(previous_song()));
	connect(main_window_ui.action_next_song,           SIGNAL(triggered()), this, SLOT(next_song()));
	connect(main_window_ui.action_move_to_currently_playing,            SIGNAL(triggered()), this, SLOT(move_to_currently_playing()));
	connect(main_window_ui.action_settings,            SIGNAL(triggered()), this, SLOT(show_settings()));
	connect(main_window_ui.action_create_new_tab,      SIGNAL(triggered()), this, SLOT(create_new_playlist()));
	connect(main_window_ui.action_add_file,            SIGNAL(triggered()), this, SLOT(add_file_to_playlist()));
	connect(main_window_ui.action_add_folder_contents, SIGNAL(triggered()), this, SLOT(add_folder_contents_to_playlist()));
	connect(main_window_ui.action_add_url,             SIGNAL(triggered()), this, SLOT(add_url_to_playlist()));
	connect(main_window_ui.action_remove_selected,     SIGNAL(triggered()), this, SLOT(remove_selected_from_playlist()));

	connect(main_window_ui.action_new_playlist, SIGNAL(triggered()), this, SLOT(create_new_playlist()));
	connect(main_window_ui.action_delete_playlist, SIGNAL(triggered()), this, SLOT(delete_playlist()));
	connect(main_window_ui.action_rename_playlist, SIGNAL(triggered()), this, SLOT(rename_playlist()));

	position_volume_widget_ui.volume->setRange(ion::audio_common::sink::min_volume(), ion::audio_common::sink::max_volume());
	position_volume_widget_ui.volume->setSliderPosition(ion::audio_common::sink::max_volume());

	connect(position_volume_widget_ui.position, SIGNAL(sliderReleased()), this, SLOT(set_current_position()));
	connect(position_volume_widget_ui.volume,   SIGNAL(sliderReleased()), this, SLOT(set_current_volume()));

	position_volume_widget_ui.position->setEnabled(false);
	position_volume_widget_ui.volume->setEnabled(false);

	backend_timeout_timer = new QTimer(this);
	backend_timeout_timer->setInterval(5000);
	connect(backend_timeout_timer, SIGNAL(timeout()), this, SLOT(backend_timeout()));

	current_position_timer = new QTimer(this);
	current_position_timer->setInterval(500);
	connect(current_position_timer, SIGNAL(timeout()), this, SLOT(get_current_playback_position()));

	backend_timeout_mode = backend_timeout_normal;
	pong_received = true;

	audio_frontend_ = audio_frontend_ptr_t(
		new audio_common::audio_frontend(
			boost::phoenix::bind(&main_window::print_backend_line, this, boost::phoenix::arg_names::arg1),
			boost::phoenix::bind(&main_window::handle_backend_pong, this)
		)
	);
	audio_frontend_->get_current_uri_changed_signal().connect(boost::phoenix::bind(&main_window::current_uri_changed, this, boost::phoenix::arg_names::arg1));
	audio_frontend_->get_current_metadata_changed_signal().connect(boost::phoenix::bind(&main_window::current_metadata_changed, this, boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));
	audio_frontend_->get_new_metadata_signal().connect(boost::phoenix::bind(&main_window::handle_new_metadata, this, boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));


	status_bar_ui_ = new status_bar_ui(this, main_window_ui.statusbar, *audio_frontend_);


	backend_log_dialog_ = new logger_dialog(this, 1000);


	settings_ = new settings(this);

	playlists_ui_ = new playlists_ui(*main_window_ui.playlist_tab_widget, *audio_frontend_, this);
	connect(&(playlists_ui_->get_tab_widget()), SIGNAL(currentChanged(int)), this, SLOT(visible_playlist_changed(int)));
	connect(main_window_ui.action_repeat, SIGNAL(toggled(bool)), this, SLOT(set_playlist_repeating(bool)));

	settings_dialog_ = new settings_dialog(
		this,
		*settings_,
		*audio_frontend_,
		playlists_ui_->get_playlists(),
		boost::phoenix::bind(&main_window::change_backend, this),
		boost::phoenix::bind(&main_window::apply_flags, this),
		boost::phoenix::bind(&logger_dialog::show, backend_log_dialog_)
	);

	get_playlist_removed_signal(playlists_ui_->get_playlists()).connect(boost::phoenix::bind(&main_window::playlist_removed, this, boost::phoenix::arg_names::arg1));

	if (!load_playlists())
	{
		playlists_traits < playlists_t > ::playlist_ptr_t new_playlist(new flat_playlist(unique_ids_));
		set_name(*new_playlist, "Default");
		add_playlist(playlists_ui_->get_playlists(), new_playlist);
	}

	ui_settings_ = new ui_settings(*this, *playlists_ui_, this);

	apply_flags();

	scan_dialog_ = new scan_dialog(this, 0);
	connect(status_bar_ui_->get_scan_indicator_icon()->get_open_scanner_dialog_action(), SIGNAL(triggered()), scan_dialog_, SLOT(show()));


	search_dialog_ = new search_dialog(this, playlists_ui_->get_playlists(), *audio_frontend_);
	connect(main_window_ui.action_find, SIGNAL(triggered()), search_dialog_, SLOT(show_search_dialog()));


	start_backend();
}


main_window::~main_window()
{
	stop_backend();
	save_playlists();
	delete ui_settings_;
	delete playlists_ui_;
	delete settings_;
	audio_frontend_ = audio_frontend_ptr_t();
}


void main_window::play()
{
	playlist_ui *playlist_ui_ = playlists_ui_->get_currently_visible_playlist_ui();
	if (playlist_ui_ == 0)
	{
		QMessageBox::warning(this, "Cannot start playback", "No playlist present in the user interface - cannot playback anything");
		return;
	}

	playlist_ui_->play_selected();
}


void main_window::pause()
{
	audio_frontend_->pause(!audio_frontend_->is_paused());
}


void main_window::stop()
{
	audio_frontend_->stop();
}


void main_window::previous_song()
{
	audio_frontend_->move_to_previous_resource();
}


void main_window::next_song()
{
	audio_frontend_->move_to_next_resource();
}


void main_window::move_to_currently_playing()
{
	playlist_ui *currently_playing_ui = playlists_ui_->get_currently_playing_playlist_ui();
	if (currently_playing_ui)
	{
		playlists_ui_->set_ui_visible(currently_playing_ui);
		currently_playing_ui->ensure_currently_playing_visible();
	}
}


void main_window::show_settings()
{
	settings_dialog_->show();
}


void main_window::set_current_position()
{
	int new_position = position_volume_widget_ui.position->value();
	audio_frontend_->set_current_position(new_position);
	status_bar_ui_->set_current_time_label(new_position);
}


void main_window::set_current_volume()
{
	int new_volume = position_volume_widget_ui.volume->value();
	audio_frontend_->set_current_volume(new_volume);
	position_volume_widget_ui.volume->set_tooltip_text(
		QString("%1%").arg(
			int(float(new_volume) / float(ion::audio_common::sink::max_volume()) * 100.0f)
		)
	);
}


void main_window::create_new_playlist()
{
	playlists_traits < playlists_t > ::playlist_ptr_t new_playlist(new flat_playlist(unique_ids_));
	set_name(*new_playlist, "New playlist");
	add_playlist(playlists_ui_->get_playlists(), new_playlist);
}


void main_window::rename_playlist()
{
	playlist *currently_visible_playlist = playlists_ui_->get_currently_visible_playlist();
	if (currently_visible_playlist == 0)
	{
		QMessageBox::warning(this, "Renaming playlist failed", "No playlist available - cannot rename");
		return;
	}

	QString new_name = QInputDialog::getText(this, "New playlist name", QString("Type in new name for playlist \"%1\"").arg(currently_visible_playlist->get_name().c_str()), QLineEdit::Normal, currently_visible_playlist->get_name().c_str());
	if (!new_name.isNull())
		currently_visible_playlist->set_name(new_name.toStdString());
}


void main_window::delete_playlist()
{
	if (playlists_ui_->get_tab_widget().count() <= 1)
		return;

	playlist *currently_visible_playlist = playlists_ui_->get_currently_visible_playlist();
	if (currently_visible_playlist == 0)
	{
		QMessageBox::warning(this, "Removing playlist failed", "No playlist available - cannot remove");
		return;
	}

	remove_playlist(playlists_ui_->get_playlists(), *currently_visible_playlist);
}


void main_window::add_file_to_playlist()
{
	if (scanner_ == 0)
		return;

	playlist *currently_visible_playlist = playlists_ui_->get_currently_visible_playlist();
	if (currently_visible_playlist == 0)
	{
		QMessageBox::warning(this, "Adding file failed", "No playlist available - cannot add a file");
		return;
	}

	QStringList song_filenames = QFileDialog::getOpenFileNames(this, QString("Add song to the playlist \"%1\"").arg(currently_visible_playlist->get_name().c_str()), "");

	BOOST_FOREACH(QString const &filename, song_filenames)
	{
		scanner_->scan_file(*currently_visible_playlist, filename);
	}
}


void main_window::add_folder_contents_to_playlist()
{
	if (scanner_ == 0)
		return;
		
	if (scanner_->is_scanning_directory())
	{
		QMessageBox::information(this, "Adding directory failed", "A directory is already being scanned");
		return;
	}

	playlist *dir_iterator_playlist = playlists_ui_->get_currently_visible_playlist();
	if (dir_iterator_playlist == 0)
	{
		QMessageBox::warning(this, "Adding file failed", "No playlist available - cannot add a file");
		return;
	}

	QString dir = QFileDialog::getExistingDirectory(this, "Select directory to scan", QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (dir.isNull())
		return;

	scanner_->scan_directory(*dir_iterator_playlist, dir);
}


void main_window::add_url_to_playlist()
{
}


void main_window::remove_selected_from_playlist()
{
	playlist_ui *playlist_ui_ = playlists_ui_->get_currently_visible_playlist_ui();
	if (playlist_ui_ == 0)
	{
		QMessageBox::warning(this, "Cannot remove items", "No playlist present in the user interface - cannot remove anything");
		return;
	}

	playlist_ui_->remove_selected();
}


void main_window::try_read_stdout_line()
{
	if (backend_process == 0)
		return;
	if (!audio_frontend_)
		return;

	while (backend_process->canReadLine())
	{
		QString line = backend_process->readLine().trimmed();
		if (!line.startsWith("current_position"))
			backend_log_dialog_->add_line("stdout", line);
		audio_frontend_->parse_incoming_line(line.toStdString());
	}
}


void main_window::backend_started()
{
	backend_log_dialog_->add_line("misc", "Backend started");
	audio_frontend_->backend_started("ion_audio");
}


void main_window::backend_error(QProcess::ProcessError process_error)
{
	switch (process_error)
	{
		case QProcess::FailedToStart: backend_log_dialog_->add_line("misc", "Backend failed to start"); break;

		case QProcess::Crashed:
			backend_log_dialog_->add_line("misc", "Backend crashed or was terminated - restarting");
			audio_frontend_->backend_terminated();
			start_backend(false);
			break;

		case QProcess::Timedout: break; // the last wait* functions timed out - ignored

		case QProcess::ReadError:
			backend_log_dialog_->add_line("misc", "Backend read error - terminating");
			backend_timeout_mode = backend_timeout_terminating;
			backend_process->terminate();
			break;

		case QProcess::WriteError:
			backend_log_dialog_->add_line("misc", "Backend standard-I/O error - terminating");
			backend_timeout_mode = backend_timeout_terminating;
			backend_process->terminate();	
			break;

		default: backend_log_dialog_->add_line("misc", "<unknown error>"); break;
	}
}


void main_window::backend_finished(int exit_code, QProcess::ExitStatus exit_status)
{
	backend_log_dialog_->add_line("misc", "Backend finished");
}


void main_window::backend_timeout()
{
	if (backend_process == 0)
		return;

	switch (backend_timeout_mode)
	{
		case backend_timeout_terminating:
		case backend_timeout_killing:
			backend_process->kill();
			backend_log_dialog_->add_line("misc", "Sending backend the KILL signal");
			backend_timeout_mode = backend_timeout_killing;
			break;

		case backend_timeout_normal:
		default:
			if (pong_received)
			{
				pong_received = false;
				audio_frontend_->send_ping();
			}
			else
			{
				backend_timeout_mode = backend_timeout_terminating;
				backend_process->terminate();
			}
	}
}


void main_window::get_current_playback_position()
{
	if (audio_frontend_->is_paused())
		return;

	audio_frontend_->issue_get_position_command();
	unsigned int current_position = audio_frontend_->get_current_position();
	if (position_volume_widget_ui.position->isEnabled())
		position_volume_widget_ui.position->set_value(current_position);

	status_bar_ui_->set_current_time_label(current_position);
}


void main_window::visible_playlist_changed(int page_index)
{
	QWidget *page_widget = playlists_ui_->get_tab_widget().widget(page_index);
	if (page_widget == 0)
	{
		main_window_ui.action_repeat->setChecked(false);
		main_window_ui.action_shuffle->setChecked(false);
		main_window_ui.repeat_shuffle_toolbar->setEnabled(false);
		return;
	}

	playlist_ui *playlist_ui_ = playlists_ui_->get_playlist_ui_for(page_widget);
	if (playlist_ui_ == 0)
	{
		// This case is reached at startup -> ignore, just exit silently
		return;
	}

	main_window_ui.repeat_shuffle_toolbar->setEnabled(true);
	main_window_ui.action_repeat->setChecked(playlist_ui_->get_playlist().is_repeating());
}


void main_window::set_playlist_repeating(bool state)
{
	playlist_ui *playlist_ui_ = playlists_ui_->get_currently_visible_playlist_ui();
	if (playlist_ui_ == 0)
	{
		QMessageBox::warning(this, "Cannot set repeat mode", "No playlist present in the user interface - cannot set repeat mode");
		return;
	}

	playlist_ui_->get_playlist().set_repeating(state);

	// tell the frontend to check whether or not its current playlist's next decoder changed after repeating was set
	if (&(playlist_ui_->get_playlist()) == audio_frontend_->get_current_playlist())
		audio_frontend_->reacquire_next_resource();
}


void main_window::start_backend(bool const start_scanner)
{
	QString backend_filepath = settings_->get_backend_filepath();

	if (!QFileInfo(backend_filepath).exists() || !QFileInfo(backend_filepath).isFile())
	{
		main_window_ui.central_pages->setCurrentWidget(main_window_ui.backend_not_configured_page);
		return;
	}

	if (start_scanner)
	{
		scanner_ = new scanner(this, playlists_ui_->get_playlists(), backend_filepath);
		connect(scanner_, SIGNAL(scan_running(bool)), status_bar_ui_->get_scan_indicator_icon(), SLOT(set_running(bool)));
		connect(status_bar_ui_->get_scan_indicator_icon()->get_cancel_action(), SIGNAL(triggered()), scanner_, SLOT(cancel_scan_slot()));
		scan_dialog_->set_scanner(scanner_);
	}

	backend_process = new QProcess(this);

	{
		main_window_ui.central_pages->setCurrentWidget(main_window_ui.tabs_page);

		connect(backend_process, SIGNAL(readyRead()),                         this, SLOT(try_read_stdout_line()));
		connect(backend_process, SIGNAL(started()),                           this, SLOT(backend_started()), Qt::DirectConnection);
		connect(backend_process, SIGNAL(error(QProcess::ProcessError)),       this, SLOT(backend_error(QProcess::ProcessError)), Qt::QueuedConnection);
		connect(backend_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(backend_finished(int, QProcess::ExitStatus)), Qt::QueuedConnection);
		backend_process->setReadChannel(QProcess::StandardOutput);
		backend_process->setProcessChannelMode(QProcess::SeparateChannels);
	}

	backend_process->start(backend_filepath);
	// TODO: waitForStarted() causes a SECOND started() signal to be sent. This is unacceptable because it confuses the audio frontend's logic. Check if this signal emission can be suppressed.
	// Note that this is a confirmed Qt bug, and seems to be present only on Linux platforms.
	//backend_process->waitForStarted(30000);

	if (backend_process->state() == QProcess::NotRunning)
	{
		delete backend_process;
		backend_process = 0;
		main_window_ui.central_pages->setCurrentWidget(main_window_ui.backend_not_configured_page);
		return;
	}

	// reset the backend timeout
	pong_received = true;
	backend_timeout_mode = backend_timeout_normal;
	backend_timeout_timer->start();

	// update the module entries for the settings UI
	audio_frontend_->update_module_entries();
}


void main_window::stop_backend(bool const send_quit_message, bool const stop_scanner, bool const send_signals)
{
	if (backend_process == 0)
		return;

	if (send_quit_message)
		print_backend_line("quit");

	if (send_signals)
	{
		backend_process->waitForFinished(4000);
		if (backend_process->state() != QProcess::NotRunning)
		{
			backend_process->terminate();
			std::cout << "Sending backend the TERM signal" << std::endl;
			backend_log_dialog_->add_line("misc", "Sending backend the TERM signal");
		}

		backend_process->waitForFinished(4000);
		if (backend_process->state() != QProcess::NotRunning)
		{
			backend_process->kill();
			std::cout << "Sending backend the KILL signal" << std::endl;
			backend_log_dialog_->add_line("misc", "Sending backend the KILL signal");
		}
	}

	// NOTE: backend_process is deleted and then immediately set to 0. It is NOT set to 0 after scanner_ has been deleted.
	// This avoids a bug where try_read_stdout_line() is called after the backend process has been deleted
	// (try_read_stdout_line() is called afterwards because Qt signal-slot calls do not have to happen immediately; they may be marshaled to the main loop)
	delete backend_process;
	backend_process = 0;

	if (stop_scanner)
	{
		scan_dialog_->set_scanner(0);
		if (scanner_ != 0)
			delete scanner_;
		scanner_ = 0;
	}
}


void main_window::change_backend()
{
	/*
	1. make a snapshot of the current playback (current volume, position, uri)
	2. disable gui
	3. disconnect playlists from the backend
	4. send "quit" to the backend
	5. wait a while, if it doesnt quit, send TERM
	6. wait a while, if it doesnt terminate, send KILL
	7. start new process; if successful, re-enable gui
	8. resume playback if possible
	*/

	// TODO: make playback snapshot
	stop_backend();
	start_backend();
	// TODO: resume playback
}


void main_window::print_backend_line(std::string const &line)
{
	if (backend_process != 0)
	{
		if (line.find("get_current_position") != 0)
			backend_log_dialog_->add_line("stdin", line.c_str());
		backend_process->write((line + "\n").c_str());
	}
}


void main_window::handle_backend_pong()
{
	backend_timeout_timer->start(); // restart timer
	pong_received = true;
	backend_timeout_mode = backend_timeout_normal;
}


std::string main_window::get_playlists_filename()
{
	return QFileInfo(settings_->fileName()).canonicalPath().toStdString() + "/playlists.json";
}


bool main_window::load_playlists()
{
	std::string playlists_filename = get_playlists_filename();
	std::ifstream playlists_file(playlists_filename.c_str());

	try
	{
		if (playlists_file.good())
		{
			Json::Value json_value;
			playlists_file >> json_value;
			load_from(playlists_ui_->get_playlists(), json_value, boost::phoenix::new_ < flat_playlist > (boost::phoenix::ref(unique_ids_)));
			return true;
		}
	}
	catch (std::runtime_error const &)
	{
	}

	return false;
}


void main_window::save_playlists()
{
	Json::Value json_value;
	save_to(playlists_ui_->get_playlists(), json_value);

	std::string playlists_filename = get_playlists_filename();

	std::ofstream playlists_file(playlists_filename.c_str());
	playlists_file << json_value;
}


void main_window::apply_flags()
{
	// always on top flag
	if (settings_->get_always_on_top_flag())
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	else
		setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	show(); // setWindowFlags() hides the window

	// notification area icon
	//system_tray_icon->setVisible(settings_->get_systray_icon_flag()); // TODO
}


void main_window::current_uri_changed(uri_optional_t const &new_current_uri)
{
	position_volume_widget_ui.volume->setEnabled(new_current_uri);
	if (new_current_uri)
		current_position_timer->start();
	else
		current_position_timer->stop();
	position_volume_widget_ui.position->set_value(0);
}


void main_window::current_metadata_changed(metadata_optional_t const &new_metadata, bool const set_slider_value)
{
	if (new_metadata)
	{
		unsigned int num_ticks = get_metadata_value < unsigned int > (*new_metadata, "num_ticks", 0);

		if (num_ticks > 0)
		{
			position_volume_widget_ui.position->setEnabled(true);
			if (set_slider_value)
				position_volume_widget_ui.position->set_value(0);
			position_volume_widget_ui.position->setRange(0, num_ticks);
		}
		else
		{
			position_volume_widget_ui.position->setEnabled(false);
			if (set_slider_value)
				position_volume_widget_ui.position->set_value(0);
			position_volume_widget_ui.position->setRange(0, 1);
		}
	}
	else
	{
		position_volume_widget_ui.position->setEnabled(false);
	}
}


void main_window::handle_new_metadata(uri const &uri_, metadata_t const &new_metadata)
{
	BOOST_FOREACH(playlists_t::playlist_ptr_t playlist_, playlists_ui_->get_playlists().get_playlists())
	{
		set_resource_metadata(*playlist_, uri_, new_metadata);
	}
}


void main_window::playlist_removed(playlist &playlist_)
{
	if (&playlist_ == audio_frontend_->get_current_playlist())
	{
		audio_frontend_->stop();
		audio_frontend_->set_current_playlist(0);
	}
}


}
}


#include "main_window.moc"

