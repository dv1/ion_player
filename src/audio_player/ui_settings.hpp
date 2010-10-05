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


#ifndef ION_AUDIO_PLAYER_UI_SETTINGS_HPP
#define ION_AUDIO_PLAYER_UI_SETTINGS_HPP

#include <QSettings>


namespace ion
{
namespace audio_player
{


class main_window;
class playlists_ui;


class ui_settings:
	public QSettings 
{
public:
	explicit ui_settings(main_window &main_window_, playlists_ui &playlists_ui_, QObject *parent = 0);
	~ui_settings();


protected:
	void restore_playlist_ui_state();
	void save_playlist_ui_state();
	void restore_geometry();
	void save_geometry();


	main_window &main_window_;
	playlists_ui &playlists_ui_;
};


}
}


#endif

