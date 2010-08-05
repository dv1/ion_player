#include <QFileDialog>
#include <QProcess>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "main_window.hpp"
#include "playlists.hpp"
#include "../backend/decoder.hpp" // TODO: this is a hack - a way to access min_volume() and max_volume(); put these in common/ instead


namespace ion
{
namespace frontend
{


main_window::main_window():
	backend_process(0)
{
	main_window_ui.setupUi(this);

	QWidget *sliders_widget = new QWidget(this);
	position_volume_widget_ui.setupUi(sliders_widget);
	main_window_ui.sliders_toolbar->addWidget(sliders_widget);

	settings_dialog = new QDialog(this);
	settings_dialog_ui.setupUi(settings_dialog);

	QToolButton *create_playlist_button = new QToolButton(this);
	create_playlist_button->setDefaultAction(main_window_ui.action_create_new_playlist);
	main_window_ui.playlist_tab_widget->setCornerWidget(create_playlist_button);

	connect(main_window_ui.action_play,                SIGNAL(triggered()), this, SLOT(play()));
	connect(main_window_ui.action_pause,               SIGNAL(triggered()), this, SLOT(pause()));
	connect(main_window_ui.action_stop,                SIGNAL(triggered()), this, SLOT(stop()));
	connect(main_window_ui.action_previous_song,       SIGNAL(triggered()), this, SLOT(previous_song()));
	connect(main_window_ui.action_next_song,           SIGNAL(triggered()), this, SLOT(next_song()));
	connect(main_window_ui.action_settings,            SIGNAL(triggered()), this, SLOT(show_settings()));
	connect(main_window_ui.action_create_new_playlist, SIGNAL(triggered()), this, SLOT(create_new_playlist()));

	position_volume_widget_ui.volume->setRange(ion::backend::decoder::min_volume(), ion::backend::decoder::max_volume()); // TODO: see above

	connect(position_volume_widget_ui.position, SIGNAL(sliderMoved(int)), this, SLOT(set_current_position(int)));
	connect(position_volume_widget_ui.volume,   SIGNAL(sliderMoved(int)), this, SLOT(set_current_volume(int)));

	connect(settings_dialog_ui.backend_filedialog, SIGNAL(clicked()), this, SLOT(backend_filepath_filedialog())); // TODO: better naming of the button and the slot


	audio_frontend_io_ = frontend_io_ptr_t(new audio_frontend_io(boost::lambda::bind(&main_window::print_backend_line, this, boost::lambda::_1)));

	settings_ = new settings(*this, this);

	playlists_ = new playlists(*main_window_ui.playlist_tab_widget, *audio_frontend_io_, this);
	playlists_entry &playlists_entry_ = playlists_->add_entry("Default");
	playlists_entry_.playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=1"), ion::metadata_t("{}")));
	playlists_entry_.playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=2"), ion::metadata_t("{}")));
	playlists_entry_.playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=3"), ion::metadata_t("{}")));
	playlists_entry_.playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=4"), ion::metadata_t("{}")));

	start_backend();
}


main_window::~main_window()
{
	stop_backend();
	delete playlists_;
	delete settings_;
	audio_frontend_io_ = frontend_io_ptr_t();
}


void main_window::play()
{
}


void main_window::pause()
{
}


void main_window::stop()
{
}


void main_window::previous_song()
{
}


void main_window::next_song()
{
}


void main_window::show_settings()
{
	settings_dialog_ui.backend_filepath->setText(settings_->get_backend_filepath());

	if (settings_dialog->exec() == QDialog::Rejected)
		return;

	settings_->set_backend_filepath(settings_dialog_ui.backend_filepath->text());
	change_backend();
}


void main_window::set_current_position(int new_position)
{
}


void main_window::set_current_volume(int new_volume)
{
}


void main_window::backend_filepath_filedialog()
{
	QString backend_filepath = QFileDialog::getOpenFileName(this, "Select backend", "");
	settings_dialog_ui.backend_filepath->setText(backend_filepath);
}


void main_window::create_new_playlist()
{
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
		std::cerr << "stdout> " << line.toStdString() << std::endl;
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
	backend_process->waitForStarted();
	if (backend_process->state() == QProcess::NotRunning)
	{
		delete backend_process;
		backend_process = 0;
		main_window_ui.central_pages->setCurrentWidget(main_window_ui.backend_not_configured_page);
	}
	else
	{
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

	backend_process->write("quit\n");
	backend_process->waitForFinished(5);
	if (backend_process->state() != QProcess::NotRunning)
		backend_process->terminate();
	backend_process->waitForFinished(5);
	if (backend_process->state() != QProcess::NotRunning)
		backend_process->kill();

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


void main_window::print_backend_line(std::string const &line)
{
	if (backend_process != 0)
		backend_process->write((line + "\n").c_str());
}


}
}


#include "main_window.moc"

