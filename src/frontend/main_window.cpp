#include <ctime>
#include <cstdlib>
#include <fstream>

#include <QDirIterator>
#include <QTimer>
#include <QFileInfo>
#include <QFileDialog>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMovie>
#include <QProcess>
#include <QMetaType>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/spirit/home/phoenix/object/new.hpp>

#include <json/reader.h>
#include <json/writer.h>

#include "main_window.hpp"
#include "playlists_ui.hpp"
#include "misc_types.hpp"
#include "scanner.hpp"

#include "../backend/decoder.hpp"


namespace ion
{
namespace frontend
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

	settings_dialog = new QDialog(this);
	settings_dialog_ui.setupUi(settings_dialog);

	QToolButton *create_playlist_button = new QToolButton(this);
	create_playlist_button->setDefaultAction(main_window_ui.action_create_new_tab);
	main_window_ui.playlist_tab_widget->setCornerWidget(create_playlist_button);

	connect(main_window_ui.action_play,                SIGNAL(triggered()), this, SLOT(play()));
	connect(main_window_ui.action_pause,               SIGNAL(triggered()), this, SLOT(pause()));
	connect(main_window_ui.action_stop,                SIGNAL(triggered()), this, SLOT(stop()));
	connect(main_window_ui.action_previous_song,       SIGNAL(triggered()), this, SLOT(previous_song()));
	connect(main_window_ui.action_next_song,           SIGNAL(triggered()), this, SLOT(next_song()));
	connect(main_window_ui.action_settings,            SIGNAL(triggered()), this, SLOT(show_settings()));
	connect(main_window_ui.action_create_new_tab,      SIGNAL(triggered()), this, SLOT(create_new_playlist()));
	connect(main_window_ui.action_add_file,            SIGNAL(triggered()), this, SLOT(add_file_to_playlist()));
	connect(main_window_ui.action_add_folder_contents, SIGNAL(triggered()), this, SLOT(add_folder_contents_to_playlist()));
	connect(main_window_ui.action_add_url,             SIGNAL(triggered()), this, SLOT(add_url_to_playlist()));
	connect(main_window_ui.action_remove_selected,     SIGNAL(triggered()), this, SLOT(remove_selected_from_playlist()));

	connect(main_window_ui.action_new_playlist, SIGNAL(triggered()), this, SLOT(create_new_playlist()));
	connect(main_window_ui.action_delete_playlist, SIGNAL(triggered()), this, SLOT(delete_playlist()));
	connect(main_window_ui.action_rename_playlist, SIGNAL(triggered()), this, SLOT(rename_playlist()));

	position_volume_widget_ui.volume->setRange(ion::backend::decoder::min_volume(), ion::backend::decoder::max_volume());
	position_volume_widget_ui.volume->setSliderPosition(ion::backend::decoder::max_volume());

	connect(position_volume_widget_ui.position, SIGNAL(sliderReleased()), this, SLOT(set_current_position()));
	connect(position_volume_widget_ui.volume,   SIGNAL(sliderReleased()), this, SLOT(set_current_volume()));

	connect(settings_dialog_ui.backend_filedialog, SIGNAL(clicked()), this, SLOT(open_backend_filepath_filedialog())); // TODO: better naming of the button

	busy_indicator = new QMovie(":/icons/busy_indicator", QByteArray(), this);

	current_song_title = new QLabel(this);
	current_playback_time = new QLabel(this);
	current_song_length = new QLabel(this);
	current_scan_status = new QLabel(this);
	current_song_title->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	current_playback_time->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	current_song_length->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	current_scan_status->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	main_window_ui.statusbar->addPermanentWidget(current_scan_status);
	main_window_ui.statusbar->addPermanentWidget(current_song_title);
	main_window_ui.statusbar->addPermanentWidget(current_playback_time);
	main_window_ui.statusbar->addPermanentWidget(current_song_length);


	get_current_position_timer = new QTimer(this);
	get_current_position_timer->setInterval(1000);
	connect(get_current_position_timer, SIGNAL(timeout()), this, SLOT(get_current_playback_position()));


	audio_frontend_ = audio_frontend_ptr_t(new audio_frontend(boost::lambda::bind(&main_window::print_backend_line, this, boost::lambda::_1)));
	audio_frontend_->get_current_uri_changed_signal().connect(boost::lambda::bind(&main_window::current_uri_changed, this, boost::lambda::_1));
	audio_frontend_->get_current_metadata_changed_signal().connect(boost::lambda::bind(&main_window::current_metadata_changed, this, boost::lambda::_1));


	settings_ = new settings(*this, this);
	apply_flags();


	playlists_ui_ = new playlists_ui(*main_window_ui.playlist_tab_widget, *audio_frontend_, this);

	if (!load_playlists())
	{
		playlists_t::playlist_ptr_t new_playlist(new flat_playlist());
		set_name(*new_playlist, "Default");
		playlists_ui_->get_playlists().add_playlist(new_playlist);
	}


	start_backend();
}


main_window::~main_window()
{
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


void main_window::show_settings()
{
	// transfer settings from settings structure to dialog GUI
	settings_dialog_ui.always_on_top->setCheckState(settings_->get_always_on_top_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.on_all_workspaces->setCheckState(settings_->get_on_all_workspaces_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.notification_area_icon->setCheckState(settings_->get_systray_icon_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.backend_filepath->setText(settings_->get_backend_filepath());

	// in case of the singleplay playlist, fill the combobox with the playlist names
	settings_dialog_ui.singleplay_playlist->clear();
	BOOST_FOREACH(playlists_t::playlist_ptr_t const &playlist_, playlists_ui_->get_playlists().get_playlists())
	{
		settings_dialog_ui.singleplay_playlist->addItem(playlist_->get_name().c_str());
	}
	settings_dialog_ui.singleplay_playlist->setEditText(settings_->get_singleplay_playlist()); // and set the current singleplay playlist namie


	// if user rejected the dialog, stop processing
	if (settings_dialog->exec() == QDialog::Rejected)
		return;


	// update settings
	settings_->set_always_on_top_flag(settings_dialog_ui.always_on_top->checkState() == Qt::Checked);
	settings_->set_on_all_workspaces_flag(settings_dialog_ui.on_all_workspaces->checkState() == Qt::Checked);
	settings_->set_systray_icon_flag(settings_dialog_ui.notification_area_icon->checkState() == Qt::Checked);

	// flags
	apply_flags();

	// singleplay playlist
	if (!settings_dialog_ui.singleplay_playlist->currentText().isNull() || !settings_dialog_ui.singleplay_playlist->currentText().isEmpty())
		settings_->set_singleplay_playlist(settings_dialog_ui.singleplay_playlist->currentText());

	// backend filepath
	if (settings_dialog_ui.backend_filepath->text() != settings_->get_backend_filepath())
	{
		settings_->set_backend_filepath(settings_dialog_ui.backend_filepath->text());
		change_backend();
	}
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
}


void main_window::open_backend_filepath_filedialog()
{
	QString backend_filepath = QFileDialog::getOpenFileName(this, "Select backend", "");
	settings_dialog_ui.backend_filepath->setText(backend_filepath);
}


void main_window::create_new_playlist()
{
	playlists_t::playlist_ptr_t new_playlist(new flat_playlist());
	set_name(*new_playlist, "New playlist");
	playlists_ui_->get_playlists().add_playlist(new_playlist);
}


void main_window::rename_playlist()
{
	playlists_t::playlist_t *currently_visible_playlist = playlists_ui_->get_currently_visible_playlist();
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

	playlists_t::playlist_t *currently_visible_playlist = playlists_ui_->get_currently_visible_playlist();
	if (currently_visible_playlist == 0)
	{
		QMessageBox::warning(this, "Removing playlist failed", "No playlist available - cannot remove");
		return;
	}

	playlists_ui_->get_playlists().remove_playlist(*currently_visible_playlist);
}


void main_window::add_file_to_playlist()
{
	playlists_t::playlist_t *currently_visible_playlist = playlists_ui_->get_currently_visible_playlist();
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
	QString dir = QFileDialog::getExistingDirectory(this, "Select directory to scan", QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (dir.isNull())
		return;

	playlists_t::playlist_t *currently_visible_playlist = playlists_ui_->get_currently_visible_playlist();
	if (currently_visible_playlist == 0)
	{
		QMessageBox::warning(this, "Adding file failed", "No playlist available - cannot add a file");
		return;
	}

	QDirIterator dir_iterator(dir, QDirIterator::Subdirectories);
	while (dir_iterator.hasNext())
	{
		QString filename = dir_iterator.next();
		ion::uri uri_(std::string("file://") + filename.toStdString());
		scanner_->start_scan(*currently_visible_playlist, uri_);
	}
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
		std::cerr << "backend stdout> " << line.toStdString() << std::endl;
		audio_frontend_->parse_incoming_line(line.toStdString());
	}
}


void main_window::backend_started()
{
	std::cerr << "Backend started." << std::endl;
	audio_frontend_->backend_started("ion_audio");
}


void main_window::backend_error(QProcess::ProcessError process_error)
{
	bool restart = false, send_signals = true;
	std::cerr << "BACKEND ERROR: ";
	switch (process_error)
	{
		case QProcess::FailedToStart: std::cerr << "failed to start"; break;
		case QProcess::Crashed: std::cerr << "crashed"; restart = true; send_signals = false; break;
		case QProcess::Timedout: std::cerr << "timeout"; break;
		case QProcess::WriteError: std::cerr << "write error"; restart = true; break;
		case QProcess::ReadError: std::cerr << "read error"; restart = true; break;
		default: std::cerr << "<unknown error>"; break;
	}
	std::cerr << std::endl;
	audio_frontend_->backend_terminated();

	if (restart)
	{
		std::cerr << "Restarting backend" << std::endl;
		stop_backend(false, false, send_signals);
		start_backend(false);
	}
}


void main_window::backend_finished(int exit_code, QProcess::ExitStatus exit_status)
{
	std::cerr << "Backend finished." << std::endl;
}


void main_window::scan_running(bool state)
{
	if (state)
	{
		current_scan_status->setMovie(busy_indicator);
		busy_indicator->start();
	}
	else
	{
		busy_indicator->stop();
		current_scan_status->clear();
	}
}


void main_window::get_current_playback_position()
{
	audio_frontend_->issue_get_position_command();
	unsigned int current_position = audio_frontend_->get_current_position();
	position_volume_widget_ui.position->setValue(current_position);

	set_current_time_label(current_position);
}


void main_window::start_backend(bool const start_scanner)
{
	stop_backend();

	QString backend_filepath = settings_->get_backend_filepath();

	if (start_scanner)
	{
		scanner_ = new scanner(this, backend_filepath);
		connect(scanner_, SIGNAL(scan_running(bool)), this, SLOT(scan_running(bool)));
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
	//backend_process->waitForStarted(30000);

	if (backend_process->state() == QProcess::NotRunning)
	{
		delete backend_process;
		backend_process = 0;
		main_window_ui.central_pages->setCurrentWidget(main_window_ui.backend_not_configured_page);
	}
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
			std::cerr << "sending backend the TERM signal" << std::endl;
		}

		backend_process->waitForFinished(30000);
		if (backend_process->state() != QProcess::NotRunning)
		{
			backend_process->kill();
			std::cerr << "sending backend the KILL signal" << std::endl;
		}
	}

	// NOTE: backend_process is deleted and then immediately set to 0. It is NOT set to 0 after scanner_ has been deleted.
	// This avoids a bug where try_read_stdout_line() is called after the backend process has been deleted
	// (try_read_stdout_line() is called afterwards because Qt signal-slot calls do not have to happen immediately; they may be marshaled to the main loop)
	delete backend_process;
	backend_process = 0;

	if (stop_scanner)
	{
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
		std::cerr << "backend stdin> " << line << std::endl;
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
			load_from(playlists_ui_->get_playlists(), json_value, boost::phoenix::new_ < flat_playlist > ());
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
	if (new_current_uri)
		get_current_position_timer->start();
	else
		get_current_position_timer->stop();
	position_volume_widget_ui.position->setValue(0);
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
			position_volume_widget_ui.position->setValue(0);
			position_volume_widget_ui.position->setRange(0, num_ticks);

			if (current_num_ticks_per_second > 0)
			{
				unsigned int length_in_seconds = num_ticks / current_num_ticks_per_second;
				unsigned int minutes = length_in_seconds / 60;
				unsigned int seconds = length_in_seconds % 60;
				set_label_time(current_playback_time, 0, 0);
				set_label_time(current_song_length, minutes, seconds);
			}
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

		set_label_time(current_playback_time, minutes, seconds);
	}
	else
		current_playback_time->setText("");
}


void main_window::set_label_time(QLabel *label, int const minutes, int const seconds)
{
	label->setText(
		QString("%1:%2")
			.arg(minutes, 2, 10, QChar('0'))
			.arg(seconds, 2, 10, QChar('0'))
	);
}



}
}


#include "main_window.moc"

