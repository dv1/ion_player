#include <ctime>
#include <cstdlib>
#include <fstream>

#include <QFileInfo>
#include <QFileDialog>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QProcess>
#include <QMetaType>

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
#include "backend_log_dialog.hpp"
#include "scan_indicator_icon.hpp"

#include "../audio_backend/decoder.hpp"


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

	position_volume_widget_ui.volume->setRange(ion::audio_backend::decoder::min_volume(), ion::audio_backend::decoder::max_volume());
	position_volume_widget_ui.volume->setSliderPosition(ion::audio_backend::decoder::max_volume());

	connect(position_volume_widget_ui.position, SIGNAL(sliderReleased()), this, SLOT(set_current_position()));
	connect(position_volume_widget_ui.volume,   SIGNAL(sliderReleased()), this, SLOT(set_current_volume()));

	position_volume_widget_ui.position->setEnabled(false);
	position_volume_widget_ui.volume->setEnabled(false);

	current_song_title = new QLabel(this);
	current_playback_time = new QLabel(this);
	current_song_length = new QLabel(this);
	current_scan_status = new scan_indicator_icon(this);
	current_song_title->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	current_playback_time->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	current_song_length->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	current_scan_status->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	main_window_ui.statusbar->addPermanentWidget(current_scan_status);
	main_window_ui.statusbar->addPermanentWidget(current_song_title);
	main_window_ui.statusbar->addPermanentWidget(current_playback_time);
	main_window_ui.statusbar->addPermanentWidget(current_song_length);


	current_position_timer = new QTimer(this);
	current_position_timer->setInterval(1000);
	connect(current_position_timer, SIGNAL(timeout()), this, SLOT(get_current_playback_position()));

	scan_directory_timer.setSingleShot(false);
	connect(&scan_directory_timer, SIGNAL(timeout()), this, SLOT(scan_directory()));


	audio_frontend_ = audio_frontend_ptr_t(new audio_frontend(boost::phoenix::bind(&main_window::print_backend_line, this, boost::phoenix::arg_names::arg1)));
	audio_frontend_->get_current_uri_changed_signal().connect(boost::phoenix::bind(&main_window::current_uri_changed, this, boost::phoenix::arg_names::arg1));
	audio_frontend_->get_current_metadata_changed_signal().connect(boost::phoenix::bind(&main_window::current_metadata_changed, this, boost::phoenix::arg_names::arg1));


	backend_log_dialog_ = new backend_log_dialog(this, 1000);


	settings_ = new settings(*this, this);
	apply_flags();


	playlists_ui_ = new playlists_ui(*main_window_ui.playlist_tab_widget, *audio_frontend_, this);
	settings_dialog_ = new settings_dialog(
		this,
		*settings_,
		*audio_frontend_,
		playlists_ui_->get_playlists(),
		boost::phoenix::bind(&main_window::change_backend, this),
		boost::phoenix::bind(&main_window::apply_flags, this),
		boost::phoenix::bind(&backend_log_dialog::show, backend_log_dialog_)
	);

	if (!load_playlists())
	{
		playlists_traits < playlists_t > ::playlist_ptr_t new_playlist(new flat_playlist(unique_ids_));
		set_name(*new_playlist, "Default");
		add_playlist(playlists_ui_->get_playlists(), new_playlist);
	}


	search_dialog_ = new search_dialog(this, playlists_ui_->get_playlists(), *audio_frontend_);
	connect(main_window_ui.action_find, SIGNAL(triggered()), search_dialog_, SLOT(show_search_dialog()));


	start_backend();
}


main_window::~main_window()
{
	dir_iterator = dir_iterator_ptr_t();
	stop_backend();
	save_playlists();
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
	set_current_time_label(new_position);
}


void main_window::set_current_volume()
{
	int new_volume = position_volume_widget_ui.volume->value();
	audio_frontend_->set_current_volume(new_volume);
	position_volume_widget_ui.volume->set_tooltip_text(
		QString("%1%").arg(
			int(float(new_volume) / float(ion::audio_backend::decoder::max_volume()) * 100.0f)
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
	playlist *currently_visible_playlist = playlists_ui_->get_currently_visible_playlist();
	if (currently_visible_playlist == 0)
	{
		QMessageBox::warning(this, "Adding file failed", "No playlist available - cannot add a file");
		return;
	}

	QStringList song_filenames = QFileDialog::getOpenFileNames(this, QString("Add song to the playlist \"%1\"").arg(currently_visible_playlist->get_name().c_str()), "");

	BOOST_FOREACH(QString const &filename, song_filenames)
	{
		ion::uri uri_(std::string("file://") + filename.toStdString());
		scanner_->start_scan(*currently_visible_playlist, uri_);
	}
}


void main_window::add_folder_contents_to_playlist()
{
	if (dir_iterator)
		return;

	dir_iterator_playlist = playlists_ui_->get_currently_visible_playlist();
	if (dir_iterator_playlist == 0)
	{
		QMessageBox::warning(this, "Adding file failed", "No playlist available - cannot add a file");
		return;
	}

	QString dir = QFileDialog::getExistingDirectory(this, "Select directory to scan", QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (dir.isNull())
		return;

	dir_iterator = dir_iterator_ptr_t(new QDirIterator(dir, QDirIterator::Subdirectories));
	scan_directory_timer.start(0);
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
			backend_log_dialog_->add_line(backend_log_model::log_line_stdout, line);
		audio_frontend_->parse_incoming_line(line.toStdString());
	}
}


void main_window::backend_started()
{
	backend_log_dialog_->add_line(backend_log_model::log_line_misc, "Backend started");
	audio_frontend_->backend_started("ion_audio");
}


void main_window::backend_error(QProcess::ProcessError process_error)
{
	bool restart = false, send_signals = true;
	std::stringstream sstr;
	sstr << "BACKEND ERROR: ";
	switch (process_error)
	{
		case QProcess::FailedToStart: sstr << "failed to start"; break;
		case QProcess::Crashed: sstr << "crashed"; restart = true; send_signals = false; break;
		case QProcess::Timedout: sstr << "timeout"; break;
		case QProcess::WriteError: sstr << "write error"; restart = true; break;
		case QProcess::ReadError: sstr << "read error"; restart = true; break;
		default: sstr << "<unknown error>"; break;
	}
	backend_log_dialog_->add_line(backend_log_model::log_line_misc, sstr.str().c_str());

	audio_frontend_->backend_terminated();

	if (restart)
	{
		backend_log_dialog_->add_line(backend_log_model::log_line_misc, "Restarting backend");
		stop_backend(false, false, send_signals);
		start_backend(false);
	}
}


void main_window::backend_finished(int exit_code, QProcess::ExitStatus exit_status)
{
	backend_log_dialog_->add_line(backend_log_model::log_line_misc, "Backend finished");
}


void main_window::get_current_playback_position()
{
	audio_frontend_->issue_get_position_command();
	unsigned int current_position = audio_frontend_->get_current_position();
	if (position_volume_widget_ui.position->isEnabled())
		position_volume_widget_ui.position->set_value(current_position);

	set_current_time_label(current_position);
}


void main_window::scan_directory()
{
	if (scanner_ == 0)
		return;

	if (!dir_iterator || !dir_iterator_playlist)
		return;

	if (dir_iterator->hasNext())
	{
		QString filename = dir_iterator->next();
		ion::uri uri_(std::string("file://") + filename.toStdString());
		scanner_->start_scan(*dir_iterator_playlist, uri_);
	}
	else
	{
		dir_iterator_playlist = 0;
		dir_iterator = dir_iterator_ptr_t();
		scan_directory_timer.stop();
	}
}


void main_window::start_backend(bool const start_scanner)
{
	stop_backend();

	QString backend_filepath = settings_->get_backend_filepath();

	if (!QFileInfo(backend_filepath).exists() || !QFileInfo(backend_filepath).isFile())
	{
		main_window_ui.central_pages->setCurrentWidget(main_window_ui.backend_not_configured_page);
		return;
	}

	if (start_scanner)
	{
		scanner_ = new scanner(this, backend_filepath);
		connect(scanner_, SIGNAL(scan_running(bool)), current_scan_status, SLOT(set_running(bool)));
		connect(current_scan_status->get_cancel_action(), SIGNAL(triggered()), scanner_, SLOT(cancel_scan_slot()));
		connect(current_scan_status->get_cancel_action(), SIGNAL(triggered()), &scan_directory_timer, SLOT(stop()));
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
		backend_process->waitForFinished(30000);
		if (backend_process->state() != QProcess::NotRunning)
		{
			backend_process->terminate();
			backend_log_dialog_->add_line(backend_log_model::log_line_misc, "Sending backend the TERM signal");
		}

		backend_process->waitForFinished(30000);
		if (backend_process->state() != QProcess::NotRunning)
		{
			backend_process->kill();
			backend_log_dialog_->add_line(backend_log_model::log_line_misc, "Sending backend the KILL signal");
		}
	}

	// NOTE: backend_process is deleted and then immediately set to 0. It is NOT set to 0 after scanner_ has been deleted.
	// This avoids a bug where try_read_stdout_line() is called after the backend process has been deleted
	// (try_read_stdout_line() is called afterwards because Qt signal-slot calls do not have to happen immediately; they may be marshaled to the main loop)
	delete backend_process;
	backend_process = 0;

	if (stop_scanner)
	{
		scan_directory_timer.stop();		
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
			backend_log_dialog_->add_line(backend_log_model::log_line_stdin, line.c_str());
		backend_process->write((line + "\n").c_str());
	}
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

	current_playback_time->setText(get_time_string(0, 0));
}


void main_window::current_metadata_changed(metadata_optional_t const &new_metadata)
{
	if (new_metadata)
	{
		std::string title = get_metadata_value < std::string > (*new_metadata, "title", "");
		if (!title.empty())
			current_song_title->setText(QString::fromUtf8(title.c_str()));

		unsigned int num_ticks = get_metadata_value < unsigned int > (*new_metadata, "num_ticks", 0);
		current_num_ticks_per_second = get_metadata_value < unsigned int > (*new_metadata, "num_ticks_per_second", 0);

		if (num_ticks > 0)
		{
			position_volume_widget_ui.position->setEnabled(true);
			position_volume_widget_ui.position->set_value(0);
			position_volume_widget_ui.position->setRange(0, num_ticks);

			if (current_num_ticks_per_second > 0)
			{
				unsigned int length_in_seconds = num_ticks / current_num_ticks_per_second;
				unsigned int minutes = length_in_seconds / 60;
				unsigned int seconds = length_in_seconds % 60;

				current_playback_time->setText(get_time_string(0, 0));
				current_song_length->setText(get_time_string(minutes, seconds));
			}
		}
		else
		{
			position_volume_widget_ui.position->setEnabled(false);
			position_volume_widget_ui.position->set_value(0);
			position_volume_widget_ui.position->setRange(0, 1);
		}
	}
	else
	{
		current_song_title->setText("");
		current_song_length->setText("");
		current_playback_time->setText("");
		position_volume_widget_ui.position->setEnabled(false);
	}
}


void main_window::set_current_time_label(unsigned int const current_position)
{
	if (current_num_ticks_per_second > 0)
	{
		unsigned int time_in_seconds = current_position / current_num_ticks_per_second;
		unsigned int minutes = time_in_seconds / 60;
		unsigned int seconds = time_in_seconds % 60;

		current_playback_time->setText(get_time_string(minutes, seconds));
	}
	else
		current_playback_time->setText("");
}


QString main_window::get_time_string(int const minutes, int const seconds) const
{
	return QString("%1:%2")
		.arg(minutes, 2, 10, QChar('0'))
		.arg(seconds, 2, 10, QChar('0'))
		;
}


}
}


#include "main_window.moc"

