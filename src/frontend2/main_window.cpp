#include <ctime>
#include <cstdlib>

#include <QTimer>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSystemTrayIcon>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "main_window.hpp"
#include "playlists_ui.hpp"
#include "misc_types.hpp"

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


	get_current_position_timer = new QTimer(this);
	get_current_position_timer->setInterval(1000);
	connect(get_current_position_timer, SIGNAL(timeout()), this, SLOT(get_current_playback_position()));


	audio_frontend_ = audio_frontend_ptr_t(new audio_frontend(boost::lambda::bind(&main_window::print_backend_line, this, boost::lambda::_1)));
	audio_frontend_->get_current_uri_changed_signal().connect(boost::lambda::bind(&main_window::current_uri_changed, this, boost::lambda::_1));
	audio_frontend_->get_current_metadata_changed_signal().connect(boost::lambda::bind(&main_window::current_metadata_changed, this, boost::lambda::_1));


	settings_ = new settings(*this, this);
	apply_flags();


	playlists_ui_ = new playlists_ui(*main_window_ui.playlist_tab_widget, *audio_frontend_, this);

	audio_frontend_->get_current_uri_changed_signal().connect(boost::lambda::bind(&playlists_ui::current_uri_changed, playlists_ui_, boost::lambda::_1));
	if (!load_playlists())
	{
		playlists_t::playlist_ptr_t new_playlist(new flat_playlist());
		playlists_ui_->get_playlists().add_playlist("Default", new_playlist);
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
}


void main_window::set_current_volume()
{
}


void main_window::open_backend_filepath_filedialog()
{
}


void main_window::create_new_playlist()
{
}


void main_window::rename_playlist()
{
}


void main_window::delete_playlist()
{
}


void main_window::add_file_to_playlist()
{
}


void main_window::add_folder_contents_to_playlist()
{
}


void main_window::add_url_to_playlist()
{
}


void main_window::remove_selected_from_playlist()
{
}


void main_window::try_read_stdout_line()
{
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


void main_window::get_current_playback_position()
{
}


void main_window::start_backend()
{
}


void main_window::stop_backend()
{
}


void main_window::change_backend()
{
}


void main_window::print_backend_line(std::string const &line)
{
}

std::string main_window::get_playlists_filename()
{
}


bool main_window::load_playlists()
{
}


void main_window::save_playlists()
{
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
}


void main_window::current_metadata_changed(metadata_optional_t const &new_metadata)
{
}



}
}


#include "main_window.moc"

