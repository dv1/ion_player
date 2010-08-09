#include <ctime>
#include <cstdlib>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QProcess>
#include <QMessageBox>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <fstream>
#include <json/reader.h>
#include <json/writer.h>
#include "main_window.hpp"
#include "playlists.hpp"
#include "../backend/decoder.hpp" // TODO: this is a hack - a way to access min_volume() and max_volume(); put these in common/ instead


namespace ion
{
namespace frontend
{


main_window::main_window(uri_optional_t const &command_line_uri):
	backend_process(0)
{
	std::srand(std::time(0));

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

	position_volume_widget_ui.volume->setRange(ion::backend::decoder::min_volume(), ion::backend::decoder::max_volume()); // TODO: see above
	position_volume_widget_ui.volume->setSliderPosition(ion::backend::decoder::max_volume());

	connect(position_volume_widget_ui.position, SIGNAL(sliderMoved(int)), this, SLOT(set_current_position(int)));
	connect(position_volume_widget_ui.volume,   SIGNAL(sliderMoved(int)), this, SLOT(set_current_volume(int)));

	connect(settings_dialog_ui.backend_filedialog, SIGNAL(clicked()), this, SLOT(backend_filepath_filedialog())); // TODO: better naming of the button and the slot


	audio_frontend_io_ = frontend_io_ptr_t(new audio_frontend_io(boost::lambda::bind(&main_window::print_backend_line, this, boost::lambda::_1)));

	settings_ = new settings(*this, this);
	std::cout << settings_->fileName().toStdString() << std::endl;

	playlists_ = new playlists(*main_window_ui.playlist_tab_widget, *audio_frontend_io_, this);
	if (!load_playlists())
		playlists_->add_entry("Default");

	start_backend();


	if (command_line_uri)
	{
		QString singleplay_playlist = settings_->get_singleplay_playlist();
		playlists::entries_t::iterator playlist_entry_iter = playlists_->get_entry(singleplay_playlist);

		if (playlist_entry_iter == playlists_->get_entries().end())
		{
			playlists_->add_entry(singleplay_playlist);
			playlist_entry_iter = playlists_->get_entry(singleplay_playlist);
		}

		playlist_entry_iter->playlist_.add_entry(simple_playlist::entry(*command_line_uri, metadata_t(Json::objectValue)));

		// TODO: start playback of this URI (switch to the playlist, that is, move the corresponding tab to front, and call play())
	}
}


main_window::~main_window()
{
	stop_backend();
	save_playlists();
	delete playlists_;
	delete settings_;
	audio_frontend_io_ = frontend_io_ptr_t();
}


void main_window::play()
{
	playlists_entry *playlists_entry_ = playlists_->get_currently_visible_entry();
	if (playlists_entry_ == 0)
	{
		QMessageBox::warning(this, "Cannot start playback", "No playlist available - cannot playback anything");
		return;
	}

	playlists_entry_->play_selected();
}


void main_window::pause()
{
	audio_frontend_io_->pause(!audio_frontend_io_->is_paused());
}


void main_window::stop()
{
	audio_frontend_io_->stop();
}


void main_window::previous_song()
{
	audio_frontend_io_->move_to_previous_resource();
}


void main_window::next_song()
{
	audio_frontend_io_->move_to_next_resource();
}


void main_window::show_settings()
{
	settings_dialog_ui.backend_filepath->setText(settings_->get_backend_filepath());

	settings_dialog_ui.singleplay_playlist->clear();
	BOOST_FOREACH(playlists_entry const &entry, playlists_->get_entries())
	{
		settings_dialog_ui.singleplay_playlist->addItem(entry.name);
	}
	settings_dialog_ui.singleplay_playlist->setEditText(settings_->get_singleplay_playlist());

	if (settings_dialog->exec() == QDialog::Rejected)
		return;

	settings_->set_backend_filepath(settings_dialog_ui.backend_filepath->text());
	if (!settings_dialog_ui.singleplay_playlist->currentText().isNull() || !settings_dialog_ui.singleplay_playlist->currentText().isEmpty())
		settings_->set_singleplay_playlist(settings_dialog_ui.singleplay_playlist->currentText());
	change_backend();
}


void main_window::set_current_position(int new_position)
{
}


void main_window::set_current_volume(int new_volume)
{
	audio_frontend_io_->set_current_volume(new_volume);
}


void main_window::backend_filepath_filedialog()
{
	QString backend_filepath = QFileDialog::getOpenFileName(this, "Select backend", "");
	settings_dialog_ui.backend_filepath->setText(backend_filepath);
}


void main_window::create_new_playlist()
{
	playlists_->add_entry("New playlist");
}


void main_window::rename_playlist()
{
	playlists_entry *playlists_entry_ = playlists_->get_currently_visible_entry();
	if (playlists_entry_ == 0)
	{
		QMessageBox::warning(this, "Adding file failed", "No playlist available - cannot rename");
		return;
	}

	QString new_name = QInputDialog::getText(this, "New playlist name", QString("Type in new name for playlist \"%1\"").arg(playlists_entry_->name), QLineEdit::Normal, playlists_entry_->name);
	if (!new_name.isNull())
		playlists_->rename_entry(*playlists_entry_, new_name);
}


void main_window::delete_playlist()
{
	if (playlists_->get_tab_widget().count() <= 1)
		return;


	playlists_entry *playlists_entry_ = playlists_->get_currently_visible_entry();
	if (playlists_entry_ == 0)
	{
		QMessageBox::warning(this, "Adding file failed", "No playlist available - cannot delete");
		return;
	}

	playlists_->remove_entry(*playlists_entry_);
}


void main_window::add_file_to_playlist()
{
	playlists_entry *playlists_entry_ = playlists_->get_currently_visible_entry();
	if (playlists_entry_ == 0)
	{
		QMessageBox::warning(this, "Adding file failed", "No playlist available - cannot add a file");
		return;
	}

	QStringList song_filenames = QFileDialog::getOpenFileNames(this, QString("Add song to the playlist \"%1\"").arg(playlists_entry_->name), "");

	BOOST_FOREACH(QString const &filename, song_filenames)
	{
		playlists_entry_->playlist_.add_entry(simple_playlist::entry(ion::uri(std::string("file://") + filename.toStdString()), metadata_t(Json::objectValue)));
	}
}


void main_window::add_folder_contents_to_playlist()
{
}


void main_window::add_url_to_playlist()
{
}


void main_window::remove_selected_from_playlist()
{
	playlists_entry *playlists_entry_ = playlists_->get_currently_visible_entry();
	if (playlists_entry_ == 0)
	{
		QMessageBox::warning(this, "Removing song(s) failed", "No playlist available - cannot remove anything");
		return;
	}

	playlists_entry_->remove_selected();
}


void main_window::try_read_stdout_line()
{
	if (backend_process == 0)
		return;
	if (!audio_frontend_io_)
		return;

	while (backend_process->canReadLine())
	{
		QString line = backend_process->readLine().trimmed();
		std::cerr << "backend stdout> " << line.toStdString() << std::endl;
		audio_frontend_io_->parse_incoming_line(line.toStdString());
	}
}


void main_window::backend_started()
{
}


void main_window::backend_error(QProcess::ProcessError process_error)
{
}


void main_window::backend_finished(int exit_code, QProcess::ExitStatus exit_status)
{
}


void main_window::start_backend()
{
	stop_backend();

	QString backend_filepath = settings_->get_backend_filepath();
	backend_process = new QProcess(this);
	backend_process->start(backend_filepath);
	backend_process->waitForStarted(30000);
	if (backend_process->state() == QProcess::NotRunning)
	{
		delete backend_process;
		backend_process = 0;
		main_window_ui.central_pages->setCurrentWidget(main_window_ui.backend_not_configured_page);
	}
	else
	{
		std::cerr << backend_process->pid() << std::endl;
		main_window_ui.central_pages->setCurrentWidget(main_window_ui.tabs_page);

		connect(backend_process, SIGNAL(readyRead()),                         this, SLOT(try_read_stdout_line()));
		connect(backend_process, SIGNAL(started()),                           this, SLOT(backend_started()));
		connect(backend_process, SIGNAL(error(QProcess::ProcessError)),       this, SLOT(backend_error(QProcess::ProcessError)));
		connect(backend_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(backend_finished(int, QProcess::ExitStatus)));
		backend_process->setReadChannel(QProcess::StandardOutput);
		backend_process->setProcessChannelMode(QProcess::SeparateChannels);
	}
}


void main_window::stop_backend()
{
	if (backend_process == 0)
		return;

	print_backend_line("quit");
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

	delete backend_process;
	backend_process = 0;
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


void main_window::disable_gui()
{
}


void main_window::enable_gui()
{
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
			load_from(*playlists_, json_value);
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
	save_to(*playlists_, json_value);

	std::string playlists_filename = get_playlists_filename();

	std::ofstream playlists_file(playlists_filename.c_str());
	playlists_file << json_value;
}


void main_window::print_backend_line(std::string const &line)
{
	if (backend_process != 0)
	{
		std::cerr << "backend stdin> " << line << std::endl;
		backend_process->write((line + "\n").c_str());
	}
}


}
}


#include "main_window.moc"

