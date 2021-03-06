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

#include <QStringMatcher>
#include "filter_dialog.hpp"


namespace ion
{
namespace audio_player
{


class search_dialog:
	public filter_dialog
{
public:
	explicit search_dialog(QWidget *parent, playlists_t &playlists_, audio_common::audio_frontend &audio_frontend_);


protected:
	virtual void pattern_entered(QString const &pattern);
	virtual void song_in_row_activated(QModelIndex const &index);
	virtual bool test_for_match(playlist::entry_t const &entry_) const;


	QStringMatcher search_string_matcher;
};


}
}


#endif

