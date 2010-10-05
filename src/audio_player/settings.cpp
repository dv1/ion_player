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


#include "main_window.hpp"
#include "settings.hpp"


namespace ion
{
namespace audio_player
{


settings::settings(QObject *parent):
	QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName(), parent)
{
}


settings::~settings()
{
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


}
}

