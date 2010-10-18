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


#include <assert.h>
#include <QTreeView>
#include <QTabWidget>
#include <boost/foreach.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <ion/playlist.hpp>
#include "playlist_qt_model.hpp"
#include "playlists_ui.hpp"
#include "audio_frontend.hpp"


namespace ion
{
namespace audio_player
{


playlist_ui::playlist_ui(QObject *parent, playlist &playlist_, playlists_ui &playlists_ui_):
	QObject(parent),
	playlist_(playlist_),
	playlists_ui_(playlists_ui_),
	view_widget(0),
	playlist_qt_model_(0)
{
	QTabWidget &tab_widget = playlists_ui_.get_tab_widget();

	playlist_renamed_signal_connection = get_playlist_renamed_signal(playlist_).connect(boost::phoenix::bind(&playlist_ui::playlist_renamed, this, boost::phoenix::arg_names::arg1));

	playlist_qt_model_ = new playlist_qt_model(&tab_widget, playlists_ui_.get_playlists(), playlist_);
	current_uri_changed_signal_connection = playlists_ui_.get_audio_frontend().get_current_uri_changed_signal().connect(boost::phoenix::bind(&playlist_qt_model::current_uri_changed, playlist_qt_model_, boost::phoenix::arg_names::arg1));

	view_widget = new QTreeView(&tab_widget);
	view_widget->setRootIsDecorated(false);
	view_widget->setAlternatingRowColors(true);
	view_widget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	view_widget->setModel(playlist_qt_model_);
	tab_widget.addTab(view_widget, get_name(playlist_).c_str());


	connect(view_widget, SIGNAL(doubleClicked(QModelIndex const &)), this, SLOT(play_song_in_row(QModelIndex const &)));
}


playlist_ui::~playlist_ui()
{
	current_uri_changed_signal_connection.disconnect();
	playlist_renamed_signal_connection.disconnect();

	QTabWidget &tab_widget = playlists_ui_.get_tab_widget();
	int index = tab_widget.indexOf(view_widget);
	assert(index != -1);
	tab_widget.removeTab(index);

	delete view_widget;
	delete playlist_qt_model_;
}


void playlist_ui::play_selected()
{
	QModelIndexList selected_rows = view_widget->selectionModel()->selectedRows();
	if (!selected_rows.empty())
		play_song_in_row(*selected_rows.begin());
}


void playlist_ui::remove_selected()
{
	//std::vector < ion::uri > uris_to_be_removed;
	uri_set_t uris_to_be_removed;

	QModelIndexList selected_rows = view_widget->selectionModel()->selectedRows();
	BOOST_FOREACH(QModelIndex const &index, selected_rows)
	{
		playlist_traits < playlist > ::entry_t const *playlist_entry = get_entry(playlist_, index.row());
		uris_to_be_removed.insert(boost::fusion::at_c < 0 > (*playlist_entry));
	}

	remove_entries(playlist_, uris_to_be_removed, true);
}


void playlist_ui::ensure_currently_playing_visible()
{
	QModelIndex model_index = playlist_qt_model_->get_current_uri_model_index();
	if (model_index.isValid())
		view_widget->scrollTo(model_index, QAbstractItemView::PositionAtCenter);
}


playlist_ui::ui_state_t playlist_ui::get_ui_state() const
{
	ui_state_t ui_state(playlist_qt_model_->columnCount(QModelIndex()));
	for (int i = 0; i < playlist_qt_model_->columnCount(QModelIndex()); ++i)
		ui_state[i] = view_widget->columnWidth(i);
	return ui_state;
}


void playlist_ui::set_ui_state(ui_state_t const &ui_state)
{
	for (size_t i = 0; i < ui_state.size(); ++i)
		view_widget->setColumnWidth(i, ui_state[i]);
}


void playlist_ui::play_song_in_row(QModelIndex const &index)
{
	playlist_traits < playlist > ::entry_t const *playlist_entry = get_entry(playlist_, index.row());
	if (playlist_entry != 0)
	{
		set_active_playlist(playlists_ui_.get_playlists(), &playlist_);
		playlists_ui_.get_audio_frontend().set_current_playlist(&playlist_);
		playlists_ui_.get_audio_frontend().play(boost::fusion::at_c < 0 > (*playlist_entry));
	}
}


void playlist_ui::playlist_renamed(std::string const &new_name)
{
	int index = playlists_ui_.get_tab_widget().indexOf(view_widget);
	if (index != -1)
		playlists_ui_.get_tab_widget().setTabText(index, new_name.c_str());
}






playlists_ui::playlists_ui(QTabWidget &tab_widget, audio_common::audio_frontend &audio_frontend_, QObject *parent):
	QObject(parent),
	tab_widget(tab_widget),
	audio_frontend_(audio_frontend_)
{
	connect(&tab_widget, SIGNAL(tabCloseRequested(int)), this, SLOT(close_current_playlist(int)));

	playlist_added_signal_connection = get_playlist_added_signal(playlists_).connect(boost::phoenix::bind(&playlists_ui::playlist_added, this, boost::phoenix::arg_names::arg1));
	playlist_removed_signal_connection = get_playlist_removed_signal(playlists_).connect(boost::phoenix::bind(&playlists_ui::playlist_removed, this, boost::phoenix::arg_names::arg1));
}


playlists_ui::~playlists_ui()
{
	disconnect(&tab_widget, SIGNAL(tabCloseRequested(int)), this, SLOT(close_current_playlist(int)));

	// deleting the uis explicitely, to ensure they are gone *before* the playlists_ instance is destroyed
	BOOST_FOREACH(playlist_ui *ui, playlist_uis)
	{
		delete ui;
	}

	playlist_added_signal_connection.disconnect();
	playlist_removed_signal_connection.disconnect();
}


playlist_ui* playlists_ui::get_currently_visible_playlist_ui()
{
	QWidget *page_widget = tab_widget.currentWidget();
	BOOST_FOREACH(playlist_ui *ui, playlist_uis)
	{
		if (ui->get_view_widget() == page_widget)
			return ui;
	}

	return 0;
}


playlist* playlists_ui::get_currently_visible_playlist()
{
	playlist_ui *ui = get_currently_visible_playlist_ui();
	return (ui == 0) ? 0 : &(ui->get_playlist());
}


playlist_ui* playlists_ui::get_currently_playing_playlist_ui()
{
	// TODO: add a current_uri event handler to playlists_ui that caches the currently playing playlist's UI (get_currently_playing_playlist_ui() would then turn into a simple getter)
	// HOWEVER, the current code is OK if get_currently_playing_playlist_ui() is called rarely; in that case, caching would be actually the slower variant
	BOOST_FOREACH(playlist_ui *ui, playlist_uis)
	{
		if (ui->get_playlist_qt_model()->is_currently_playing())
			return ui;
	}

	return 0;
}


void playlists_ui::set_ui_visible(playlist_ui *ui)
{
	if (ui != 0)
		tab_widget.setCurrentWidget(ui->get_view_widget());
}


playlists_ui::ui_state playlists_ui::get_ui_state() const
{
	ui_state state;
	state.visible_playlist_index = tab_widget.currentIndex();

	BOOST_FOREACH(playlist_ui *playlist_ui_, playlist_uis)
	{
		playlist_ui::ui_state_t playlist_ui_state = playlist_ui_->get_ui_state();
		state.playlist_ui_states.push_back(playlist_ui_state);
	}

	return state;
}


void playlists_ui::set_ui_state(ui_state const &ui_state_)
{
	for (size_t i = 0; i < std::min(ui_state_.playlist_ui_states.size(), playlist_uis.size()); ++i)
		playlist_uis[i]->set_ui_state(ui_state_.playlist_ui_states[i]);

	tab_widget.setCurrentIndex(ui_state_.visible_playlist_index);
}


void playlists_ui::close_current_playlist(int index)
{
	if (index == -1)
		return;
	if (tab_widget.count() <= 1)
		return;

	QWidget *page_widget = tab_widget.widget(index);
	BOOST_FOREACH(playlist_ui *ui, playlist_uis)
	{
		if (ui->get_view_widget() == page_widget)
		{
			remove_playlist(playlists_, ui->get_playlist());
			break;
		}
	}
}


void playlists_ui::playlist_added(playlists_traits < playlists_t > ::playlist_t &playlist_)
{
	playlist_ui *new_playlist_ui = new playlist_ui(this, playlist_, *this);
	playlist_uis.push_back(new_playlist_ui);
}


void playlists_ui::playlist_removed(playlists_traits < playlists_t > ::playlist_t &playlist_)
{
	for (playlist_uis_t::iterator iter = playlist_uis.begin(); iter != playlist_uis.end(); ++iter)
	{
		playlist_ui *ui = *iter;
		if (&(ui->get_playlist()) == &playlist_)
		{
			delete ui;
			playlist_uis.erase(iter);
			break;
		}
	}
}


}
}


#include "playlists_ui.moc"

