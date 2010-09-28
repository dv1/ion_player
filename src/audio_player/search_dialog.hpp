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


#ifndef ION_AUDIO_PLAYER_SEARCH_DIALOG_HPP
#define ION_AUDIO_PLAYER_SEARCH_DIALOG_HPP

#include <QDialog>
#include <QModelIndex>
#include <QStringMatcher>

#include <boost/shared_ptr.hpp>

#include <ion/filter_playlist.hpp>

#include "ui_search_dialog.h"
#include "misc_types.hpp"


class QLabel;
class QMovie;


namespace ion
{
namespace audio_player
{


class audio_frontend;
class playlist_qt_model;


class search_dialog:
	public QDialog 
{
	Q_OBJECT
public:
	explicit search_dialog(QWidget *parent, playlists_t &playlists_, audio_frontend &audio_frontend_);
	~search_dialog();


public slots:
	void show_search_dialog();


protected slots:
	void search_term_entered(QString const &text);
	void search_dialog_hidden();
	void play_song_in_row(QModelIndex const &index);
	void title_checkbox_state_changed(int new_state);
	void artist_checkbox_state_changed(int new_state);
	void album_checkbox_state_changed(int new_state);


protected:
	typedef filter_playlist < playlists_t > filter_playlist_t;
	typedef boost::shared_ptr < filter_playlist_t > filter_playlist_ptr_t;

	bool match_search_string(playlist::entry_t const &entry_) const;


	QStringMatcher search_string_matcher;
	filter_playlist_ptr_t search_playlist_;
	playlist_qt_model *search_qt_model_;
	playlists_t &playlists_;
	audio_frontend &audio_frontend_;
	boost::signals2::connection current_uri_changed_signal_connection;
	Ui_search_dialog search_dialog_ui;
};


}
}


#endif

