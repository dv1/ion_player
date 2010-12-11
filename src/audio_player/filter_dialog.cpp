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


#include <QItemSelectionModel>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include "playlist_qt_model.hpp"
#include "filter_dialog.hpp"
#include "audio_frontend.hpp"


namespace ion
{
namespace audio_player
{


filter_dialog::filter_dialog(QWidget *parent, playlists_t &playlists_, audio_common::audio_frontend &audio_frontend_):
	QDialog(parent),
	playlists_(playlists_),
	audio_frontend_(audio_frontend_)
{
	filter_dialog_ui.setupUi(this);

	connect(this, SIGNAL(finished(int)), this, SLOT(filter_dialog_hidden()));
	connect(filter_dialog_ui.filter_pattern_edit, SIGNAL(textEdited(QString const &)), this, SLOT(pattern_entered(QString const &)));
	connect(filter_dialog_ui.filter_results_view, SIGNAL(doubleClicked(QModelIndex const &)), this, SLOT(song_in_row_activated(QModelIndex const &)));
	connect(filter_dialog_ui.filter_title, SIGNAL(stateChanged(int)), this, SLOT(title_checkbox_state_changed(int)));
	connect(filter_dialog_ui.filter_album, SIGNAL(stateChanged(int)), this, SLOT(album_checkbox_state_changed(int)));
	connect(filter_dialog_ui.filter_artist, SIGNAL(stateChanged(int)), this, SLOT(artist_checkbox_state_changed(int)));
}


filter_dialog::~filter_dialog()
{
	current_uri_changed_signal_connection.disconnect();
}


void filter_dialog::show_filter_dialog()
{
	if (isVisible())
		return;

	filter_playlist_ = filter_playlist_ptr_t(
		new filter_playlist_t(
			playlists_,
			boost::phoenix::bind(&filter_dialog::test_for_match, this, boost::phoenix::arg_names::arg1)
		)
	);
	filter_qt_model_ = new playlist_qt_model(this, playlists_, *filter_playlist_);
	current_uri_changed_signal_connection = audio_frontend_.get_current_uri_changed_signal().connect(boost::phoenix::bind(&playlist_qt_model::current_uri_changed, filter_qt_model_, boost::phoenix::arg_names::arg1));
	filter_qt_model_->current_uri_changed(audio_frontend_.get_current_uri());

	QItemSelectionModel *old_selection_model = filter_dialog_ui.filter_results_view->selectionModel();
	filter_dialog_ui.filter_results_view->setModel(filter_qt_model_);
	if (old_selection_model != 0)
		delete old_selection_model;

	show();
}


void filter_dialog::pattern_entered(QString const &)
{
	if (filter_playlist_)
		filter_playlist_->update_entries();
}


void filter_dialog::filter_dialog_hidden()
{
	filter_dialog_ui.filter_results_view->setModel(0);
	QItemSelectionModel *old_selection_model = filter_dialog_ui.filter_results_view->selectionModel();
	if (old_selection_model != 0)
		delete old_selection_model;

	current_uri_changed_signal_connection.disconnect();
	delete filter_qt_model_;
	filter_playlist_ = filter_playlist_ptr_t();
}


void filter_dialog::song_in_row_activated(QModelIndex const &)
{
}


void filter_dialog::title_checkbox_state_changed(int)
{
	if (filter_playlist_)
		filter_playlist_->update_entries();
}


void filter_dialog::artist_checkbox_state_changed(int)
{
	if (filter_playlist_)
		filter_playlist_->update_entries();
}


void filter_dialog::album_checkbox_state_changed(int)
{
	if (filter_playlist_)
		filter_playlist_->update_entries();
}


bool filter_dialog::test_for_match(playlist::entry_t const &) const
{
	return false;
}


}
}


#include "filter_dialog.moc"

