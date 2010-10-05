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


#include <QDesktopWidget>
#include "main_window.hpp"
#include "ui_settings.hpp"
#include "playlists_ui.hpp"


namespace ion
{
namespace audio_player
{


ui_settings::ui_settings(main_window &main_window_, playlists_ui &playlists_ui_, QObject *parent):
	QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName(), parent),
	main_window_(main_window_),
	playlists_ui_(playlists_ui_)
{
	restore_geometry();
	restore_playlist_ui_state();
}


ui_settings::~ui_settings()
{
	save_geometry();
	save_playlist_ui_state();
}


void ui_settings::restore_playlist_ui_state()
{
	typedef QList < QVariant > variant_list_t;

	beginGroup("playlist_ui");

	playlists_ui::ui_state playlists_ui_state;
	if (contains("visible_playlist_index"))
		playlists_ui_state.visible_playlist_index = value("visible_playlist_index").value < int > ();
	size_t num_playlist_ui_states = 0;
	if (contains("num_playlist_ui_states"))
		num_playlist_ui_states = value("num_playlist_ui_states").value < int > ();
	for (size_t i = 0; i < num_playlist_ui_states; ++i)
	{
		QString state_label = QString("playlist_ui_state_%1").arg(i);
		variant_list_t stored_state = value(state_label).value < variant_list_t > ();
		playlist_ui::ui_state_t state;
		BOOST_FOREACH(QVariant const &variant, stored_state)
		{
			state.push_back(variant.value < int > ());
		}
		playlists_ui_state.playlist_ui_states.push_back(state);
	}
	playlists_ui_.set_ui_state(playlists_ui_state);

	endGroup();
}


void ui_settings::save_playlist_ui_state()
{
	typedef QList < QVariant > variant_list_t;

	beginGroup("playlist_ui");

	playlists_ui::ui_state playlists_ui_state = playlists_ui_.get_ui_state();
	setValue("visible_playlist_index", playlists_ui_state.visible_playlist_index);
	setValue("num_playlist_ui_states", int(playlists_ui_state.playlist_ui_states.size()));
	for (size_t i = 0; i < playlists_ui_state.playlist_ui_states.size(); ++i)
	{
		playlist_ui::ui_state_t state = playlists_ui_state.playlist_ui_states[i];
		variant_list_t stored_state;
		BOOST_FOREACH(int width, state)
		{
			stored_state.push_back(QVariant::fromValue(width));
		}
		QString state_label = QString("playlist_ui_state_%1").arg(i);
		setValue(state_label, stored_state);
	}

	endGroup();
}


void ui_settings::restore_geometry()
{
	beginGroup("main_window");
	main_window_.resize(value("size", QSize(600, 400)).toSize());
	main_window_.move(value("pos", QApplication::desktop()->screenGeometry().topLeft() + QPoint(200, 200)).toPoint());
	endGroup();
}


void ui_settings::save_geometry()
{
	beginGroup("main_window");
	setValue("size", main_window_.size());
	setValue("pos", main_window_.pos());
	endGroup();
}


}
}

