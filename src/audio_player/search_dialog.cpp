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


#include <iostream>
#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include "search_dialog.hpp"
#include "audio_frontend.hpp"


namespace ion
{
namespace audio_player
{


search_dialog::search_dialog(QWidget *parent, playlists_t &playlists_, audio_common::audio_frontend &audio_frontend_):
	filter_dialog(parent, playlists_, audio_frontend_)
{
	search_string_matcher.setCaseSensitivity(Qt::CaseInsensitive);
}


void search_dialog::pattern_entered(QString const &pattern)
{
	search_string_matcher.setPattern(pattern);
	filter_dialog::pattern_entered(pattern);
}


void search_dialog::song_in_row_activated(QModelIndex const &index)
{
	if (!filter_playlist_)
		return;

	filter_playlist_t::proxy_entry_optional_t proxy_entry_ = filter_playlist_->get_proxy_entry(index.row());
	if (!proxy_entry_)
		return;

	set_active_playlist(playlists_, proxy_entry_->playlist_);
	audio_frontend_.set_current_playlist(proxy_entry_->playlist_);
	audio_frontend_.play(proxy_entry_->uri_);
}


bool search_dialog::test_for_match(playlist::entry_t const &entry_) const
{
	QString search_term = filter_dialog_ui.filter_pattern_edit->text();
	if (search_term.isNull() || search_term.isEmpty())
		return true;

	metadata_t const &metadata = boost::fusion::at_c < 1 > (entry_);

	typedef boost::fusion::vector2 < QCheckBox*, std::string > check_box_entry_t;

	boost::array < check_box_entry_t, 3 > check_box_entries = {{
		check_box_entry_t(filter_dialog_ui.filter_title, "title"),
		check_box_entry_t(filter_dialog_ui.filter_artist, "artist"),
		check_box_entry_t(filter_dialog_ui.filter_album, "album"),
	}};

	BOOST_FOREACH(check_box_entry_t const &check_box_entry_, check_box_entries)
	{
		QCheckBox *check_box = boost::fusion::at_c < 0 > (check_box_entry_);
		if (check_box->checkState() == Qt::Unchecked)
			continue;

		QString text = QString::fromUtf8(get_metadata_value < std::string > (metadata, boost::fusion::at_c < 1 > (check_box_entry_), "").c_str());
		if (search_string_matcher.indexIn(text) != -1)
			return true;
	}

	return false;
}


}
}

