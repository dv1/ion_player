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


#ifndef ION_AUDIO_PLAYER_STATUS_BAR_UI_HPP
#define ION_AUDIO_PLAYER_STATUS_BAR_UI_HPP

#include <QObject>
#include <boost/signals2/connection.hpp>
#include <ion/uri.hpp>
#include "audio_frontend.hpp"


class QLabel;
class QWidget;
class QStatusBar;


namespace ion
{
namespace audio_player
{


class scan_indicator_icon;


class status_bar_ui:
	public QObject
{
public:	
	explicit status_bar_ui(QWidget *parent, QStatusBar *status_bar, audio_common::audio_frontend &audio_frontend_);
	~status_bar_ui();


	scan_indicator_icon* get_scan_indicator_icon()
	{
		return scan_indicator_icon_;
	}


	void set_current_time_label(unsigned int const current_position);


protected:
	void current_uri_changed(uri_optional_t const &new_current_uri);
	void current_metadata_changed(metadata_optional_t const &new_metadata, bool const reset_playback_position);

	QString get_time_string(int const minutes, int const seconds) const;

	
	QLabel *current_song_title, *current_playback_time, *current_song_length;
	scan_indicator_icon *scan_indicator_icon_;
	unsigned int current_num_ticks_per_second;

	boost::signals2::connection
		current_uri_changed_signal_connection,
		current_metadata_changed_signal_connection;
};


}
}


#endif

