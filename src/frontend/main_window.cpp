#include <QFileDialog>
#include <QProcess>
#include "main_window.hpp"
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
	connect(position_volume_widget_ui.volume,   SIGNAL(sliderMoved(int)), this, SLOT(set_current_voume(int)));

	connect(settings_dialog_ui.backend_filedialog, SIGNAL(clicked()), this, SLOT(backend_filepath_filedialog())); // TODO: better naming of the button and the slot


	settings_ = new settings(*this, this);
	start_backend();
}


main_window::~main_window()
{
	stop_backend();
	delete settings_;
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


void main_window::set_current_voume(int new_volume)
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
		main_window_ui.central_pages->setCurrentWidget(main_window_ui.tabs_page);
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


}
}


#include "main_window.moc"

