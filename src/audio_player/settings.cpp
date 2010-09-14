#include <iostream>
#include <QDesktopWidget>
#include "main_window.hpp"
#include "settings.hpp"


namespace ion
{
namespace audio_player
{


settings::settings(main_window &main_window_, QObject *parent):
	QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName(), parent),
	main_window_(main_window_)
{
	std::cout << "organization: " << QCoreApplication::organizationName().toStdString() << std::endl;
	std::cout << "application:  " << QCoreApplication::applicationName().toStdString() << std::endl;
	restore_geometry();
}


settings::~settings()
{
	save_geometry();
}


QString settings::get_backend_filepath() const
{
	return value("backend_filepath", QString("")).value < QString > ();
}


QString settings::get_singleplay_playlist() const
{
	return value("singleplay_playlist", QString("Default")).value < QString > ();
}


bool settings::get_always_on_top_flag() const
{
	return value("always_on_top", false).value < bool > ();
}


bool settings::get_on_all_workspaces_flag() const
{
	return value("on_all_workspaces", false).value < bool > ();
}


bool settings::get_systray_icon_flag() const
{
	return value("systray_icon", false).value < bool > ();
}


bool settings::get_backend_log_dialog_shown_flag() const
{
	return value("backend_log_dialog_shown", false).value < bool > ();
}


void settings::set_backend_filepath(QString const &new_filepath)
{
	setValue("backend_filepath", new_filepath);
}


void settings::set_singleplay_playlist(QString const &new_singleplay_playlist)
{
	setValue("singleplay_playlist", new_singleplay_playlist);
}


void settings::set_always_on_top_flag(bool const new_flag)
{
	setValue("always_on_top", new_flag);
}


void settings::set_on_all_workspaces_flag(bool const new_flag)
{
	setValue("on_all_workspaces", new_flag);
}


void settings::set_systray_icon_flag(bool const new_flag)
{
	setValue("systray_icon", new_flag);
}


void settings::set_backend_log_dialog_shown_flag(bool const new_flag)
{
	setValue("backend_log_dialog_shown", new_flag);
}


void settings::restore_geometry()
{
	beginGroup("main_window");
	main_window_.resize(value("size", QSize(600, 400)).toSize());
	main_window_.move(value("pos", QApplication::desktop()->screenGeometry().topLeft() + QPoint(200, 200)).toPoint());
	endGroup();
}


void settings::save_geometry()
{
	beginGroup("main_window");
	setValue("size", main_window_.size());
	setValue("pos", main_window_.pos());
	endGroup();
}


}
}

