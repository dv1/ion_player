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
#include <QItemSelectionModel>
#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include "playlist_qt_model.hpp"
#include "search_dialog.hpp"
#include "audio_frontend.hpp"


namespace ion
{
namespace audio_player
{


search_dialog::search_dialog(QWidget *parent, playlists_t &playlists_, audio_common::audio_frontend &audio_frontend_):
	QDialog(parent),
	playlists_(playlists_),
	audio_frontend_(audio_frontend_)
{
	search_string_matcher.setCaseSensitivity(Qt::CaseInsensitive);
	search_string_matcher.setPattern("ocean");
	search_dialog_ui.setupUi(this);

	connect(this, SIGNAL(finished(int)), this, SLOT(search_dialog_hidden()));
	connect(search_dialog_ui.search_term_edit, SIGNAL(textEdited(QString const &)), this, SLOT(search_term_entered(QString const &)));
	connect(search_dialog_ui.search_results_view, SIGNAL(doubleClicked(QModelIndex const &)), this, SLOT(play_song_in_row(QModelIndex const &)));
	connect(search_dialog_ui.filter_title, SIGNAL(stateChanged(int)), this, SLOT(title_checkbox_state_changed(int)));
	connect(search_dialog_ui.filter_album, SIGNAL(stateChanged(int)), this, SLOT(album_checkbox_state_changed(int)));
	connect(search_dialog_ui.filter_artist, SIGNAL(stateChanged(int)), this, SLOT(artist_checkbox_state_changed(int)));
}


search_dialog::~search_dialog()
{
}


void search_dialog::show_search_dialog()
{
	if (isVisible())
		return;

	search_playlist_ = filter_playlist_ptr_t(
		new filter_playlist_t(
			playlists_,
			boost::phoenix::bind(&search_dialog::match_search_string, this, boost::phoenix::arg_names::arg1)
		)
	);
	search_qt_model_ = new playlist_qt_model(this, playlists_, *search_playlist_);
	current_uri_changed_signal_connection = audio_frontend_.get_current_uri_changed_signal().connect(boost::phoenix::bind(&playlist_qt_model::current_uri_changed, search_qt_model_, boost::phoenix::arg_names::arg1));
	search_qt_model_->current_uri_changed(audio_frontend_.get_current_uri());

	QItemSelectionModel *old_selection_model = search_dialog_ui.search_results_view->selectionModel();
	search_dialog_ui.search_results_view->setModel(search_qt_model_);
	if (old_selection_model != 0)
		delete old_selection_model;

	show();
}


void search_dialog::search_term_entered(QString const &text)
{
	search_string_matcher.setPattern(text);

	if (search_playlist_)
		search_playlist_->update_entries();
}


void search_dialog::search_dialog_hidden()
{
	search_dialog_ui.search_results_view->setModel(0);
	QItemSelectionModel *old_selection_model = search_dialog_ui.search_results_view->selectionModel();
	if (old_selection_model != 0)
		delete old_selection_model;

	current_uri_changed_signal_connection.disconnect();
	delete search_qt_model_;
	search_playlist_ = filter_playlist_ptr_t();
}


void search_dialog::play_song_in_row(QModelIndex const &index)
{
	if (!search_playlist_)
		return;

	filter_playlist_t::proxy_entry_optional_t proxy_entry_ = search_playlist_->get_proxy_entry(index.row());
	if (!proxy_entry_)
		return;

	set_active_playlist(playlists_, proxy_entry_->playlist_);
	audio_frontend_.set_current_playlist(proxy_entry_->playlist_);
	audio_frontend_.play(proxy_entry_->uri_);
}


void search_dialog::title_checkbox_state_changed(int)
{
	if (search_playlist_)
		search_playlist_->update_entries();
}


void search_dialog::artist_checkbox_state_changed(int)
{
	if (search_playlist_)
		search_playlist_->update_entries();
}


void search_dialog::album_checkbox_state_changed(int)
{
	if (search_playlist_)
		search_playlist_->update_entries();
}


bool search_dialog::match_search_string(playlist::entry_t const &entry_) const
{
	QString search_term = search_dialog_ui.search_term_edit->text();
	if (search_term.isNull() || search_term.isEmpty())
		return true;

	metadata_t const &metadata = boost::fusion::at_c < 1 > (entry_);

	typedef boost::fusion::vector2 < QCheckBox*, std::string > check_box_entry_t;

	boost::array < check_box_entry_t, 3 > check_box_entries = {{
		check_box_entry_t(search_dialog_ui.filter_title, "title"),
		check_box_entry_t(search_dialog_ui.filter_artist, "artist"),
		check_box_entry_t(search_dialog_ui.filter_album, "album"),
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


#include "search_dialog.moc"

