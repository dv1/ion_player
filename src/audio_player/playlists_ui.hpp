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


#ifndef ION_AUDIO_PLAYER_PLAYLISTS_UI_HPP
#define ION_AUDIO_PLAYER_PLAYLISTS_UI_HPP

#include <vector>
#include <QObject>
#include <QModelIndex>
#include <boost/signals2/connection.hpp>
#include <ion/playlists.hpp>
#include <ion/uri.hpp>

#include "misc_types.hpp"


class QMenu;
class QTabWidget;
class QTreeView;


namespace ion
{


namespace audio_common
{
class audio_frontend;
}


namespace audio_player
{


class playlists_ui;
class playlist_qt_model;


class playlist_ui:
	public QObject
{
	Q_OBJECT
public:
	typedef std::vector < int > ui_state_t;


	explicit playlist_ui(QObject *parent, playlist &playlist_, playlists_ui &playlists_ui_);
	~playlist_ui();

	void play_selected();
	void remove_selected();

	void ensure_currently_playing_visible();

	inline playlist & get_playlist() { return playlist_; }
	inline QTreeView* get_view_widget() { return view_widget; }
	inline playlist_qt_model* get_playlist_qt_model() { return playlist_qt_model_; }

	ui_state_t get_ui_state() const;
	void set_ui_state(ui_state_t const &ui_state);


protected slots:
	void play_song_in_row(QModelIndex const &index);
	void custom_context_menu_requested(QPoint const &clicked_point);
	void play_clicked_song();
	void set_no_repeat();
	void set_repeat_once();
	void set_repeat_forever();
	void edit_metadata();


protected:
	void playlist_renamed(std::string const &new_name);
	void set_loop_mode_for_clicked(int const mode);
	QString create_tab_name(std::string const &name) const;


	playlist &playlist_;
	playlists_ui &playlists_ui_;
	QTreeView *view_widget;
	playlist_qt_model *playlist_qt_model_;
	boost::signals2::connection playlist_renamed_signal_connection, current_uri_changed_signal_connection;

	QModelIndex clicked_model_index;
	QMenu *context_menu;
};


class playlists_ui:
	public QObject
{
	Q_OBJECT
public:
	struct ui_state
	{
		typedef std::vector < playlist_ui::ui_state_t > playlist_ui_states_t;

		int visible_playlist_index;
		playlist_ui_states_t playlist_ui_states;
	};


	explicit playlists_ui(QTabWidget &tab_widget, audio_common::audio_frontend &audio_frontend_, QObject *parent);
	~playlists_ui();

	QTabWidget& get_tab_widget() { return tab_widget; }
	playlists_t & get_playlists() { return playlists_; }
	audio_common::audio_frontend& get_audio_frontend() { return audio_frontend_; }
	playlist_ui* get_playlist_ui_for(QWidget *page_widget);
	playlist_ui* get_currently_visible_playlist_ui();
	playlist_ui* get_currently_playing_playlist_ui();
	playlist* get_currently_visible_playlist();
	playlist* get_currently_playing_playlist();
	void set_ui_visible(playlist_ui *ui);

	ui_state get_ui_state() const;
	void set_ui_state(ui_state const &ui_state_);


protected slots:
	void close_current_playlist(int index);


protected:
	void playlist_added(playlists_traits < playlists_t > ::playlist_t &playlist_);
	void playlist_removed(playlists_traits < playlists_t > ::playlist_t &playlist_);


	playlists_t playlists_;
	QTabWidget &tab_widget;
	audio_common::audio_frontend &audio_frontend_;

	typedef std::vector < playlist_ui* > playlist_uis_t;
	playlist_uis_t playlist_uis;

	boost::signals2::connection
		playlist_added_signal_connection,
		playlist_removed_signal_connection,
		active_playlist_changed_connection;
};


}
}


#endif

