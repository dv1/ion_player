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


#ifndef ION_AUDIO_PLAYER_FILTER_DIALOG_HPP
#define ION_AUDIO_PLAYER_FILTER_DIALOG_HPP

#include <QDialog>
#include <QModelIndex>

#include <boost/shared_ptr.hpp>

#include <ion/filter_playlist.hpp>

#include "ui_filter_dialog.h"
#include "misc_types.hpp"


namespace ion
{


namespace audio_common
{
class audio_frontend;
}


namespace audio_player
{


class playlist_qt_model;


class filter_dialog:
	public QDialog 
{
	Q_OBJECT
public:
	explicit filter_dialog(QWidget *parent, playlists_t &playlists_, audio_common::audio_frontend &audio_frontend_);
	~filter_dialog();


public slots:
	void show_filter_dialog();


protected slots:
	virtual void pattern_entered(QString const &pattern);
	virtual void song_in_row_activated(QModelIndex const &index);
	void filter_dialog_hidden();
	void title_checkbox_state_changed(int new_state);
	void artist_checkbox_state_changed(int new_state);
	void album_checkbox_state_changed(int new_state);


protected:
	typedef filter_playlist < playlists_t > filter_playlist_t;
	typedef boost::shared_ptr < filter_playlist_t > filter_playlist_ptr_t;

	virtual bool test_for_match(playlist::entry_t const &entry_) const;


	filter_playlist_ptr_t filter_playlist_;
	playlist_qt_model *filter_qt_model_;
	playlists_t &playlists_;
	audio_common::audio_frontend &audio_frontend_;
	boost::signals2::connection current_uri_changed_signal_connection;
	Ui_filter_dialog filter_dialog_ui;
};


}
}


#endif

