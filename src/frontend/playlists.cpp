#include <QString>
#include <QTabWidget>
#include <QTreeView>

#include <assert.h>
#include <memory>
#include "playlists.hpp"
#include "playlist_qt_model.hpp"
#include "audio_frontend_io.hpp"


namespace ion
{
namespace frontend
{


playlists_entry::playlists_entry(playlists &playlists_, QString const &name):
	name(name),
	playlists_(playlists_)
{
	QTabWidget &tab_widget = playlists_.get_tab_widget();
	playlist_qt_model_ = new playlist_qt_model(&tab_widget, playlist_);
	view_widget = new QTreeView(&tab_widget);
	view_widget->setRootIsDecorated(false);
	view_widget->setAlternatingRowColors(true);
	view_widget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	view_widget->setModel(playlist_qt_model_);
	tab_widget.addTab(view_widget, name);

	connect(view_widget, SIGNAL(doubleClicked(QModelIndex const &)), this, SLOT(play_song_in_row(QModelIndex const &)));
}


playlists_entry::~playlists_entry()
{
	QTabWidget &tab_widget = playlists_.get_tab_widget();

	int index = tab_widget.indexOf(view_widget);
	assert(index != -1);

	tab_widget.removeTab(index);
	delete view_widget;
	delete playlist_qt_model_;
}


void playlists_entry::play_selected()
{
	QModelIndexList selected_rows = view_widget->selectionModel()->selectedRows();
	if (!selected_rows.empty())
		play_song_in_row(*selected_rows.begin());
}


void playlists_entry::remove_selected()
{
	std::vector < ion::uri > uris_to_be_removed;

	QModelIndexList selected_rows = view_widget->selectionModel()->selectedRows();
	BOOST_FOREACH(QModelIndex const &index, selected_rows)
	{
		simple_playlist::entry const *playlist_entry = playlist_.get_entry(index.row());
		uris_to_be_removed.push_back(playlist_entry->uri_);
	}

	BOOST_FOREACH(ion::uri const &uri_, uris_to_be_removed)
	{
		playlist_.remove_entry(uri_);
	}
}


void playlists_entry::play_song_in_row(QModelIndex const &index)
{
	simple_playlist::entry const *playlist_entry = playlist_.get_entry(index.row());
	playlists_.get_audio_frontend_io().set_current_playlist(&playlist_);
	playlists_.get_audio_frontend_io().play(playlist_entry->uri_);
}




playlists::playlists(QTabWidget &tab_widget, audio_frontend_io &audio_frontend_io_, QObject *parent):
	QObject(parent),
	audio_frontend_io_(audio_frontend_io_),
	tab_widget(tab_widget),
	active_entry(0)
{
}


playlists_entry& playlists::add_entry(QString const &playlist_name)
{
	std::unique_ptr < playlists_entry > new_entry(new playlists_entry(*this, playlist_name));
	entries.push_back(new_entry.get());
	playlists_entry &result = *new_entry;
	new_entry.release();
	return result;
}


void playlists::remove_entry(QTreeView *entry_view_widget)
{
	for (entries_t::iterator iter = entries.begin(); iter != entries.end(); ++iter)
	{
		if (iter->view_widget == entry_view_widget)
		{
			if (&(*iter) == active_entry)
			{
				audio_frontend_io_.set_current_playlist(0);
				active_entry = 0;
			}

			entries.erase(iter);
			break;
		}
	}
}


void playlists::set_active_entry(playlists_entry &new_active_entry)
{
	active_entry = &new_active_entry;
	audio_frontend_io_.set_current_playlist(&(new_active_entry.playlist_));
}


playlists_entry* playlists::get_currently_visible_entry()
{
	QWidget *page_widget = tab_widget.currentWidget();
	BOOST_FOREACH(playlists_entry &entry, entries)
	{
		if (entry.view_widget == page_widget)
			return &entry;
	}

	return 0;
}




void load_from(playlists &playlists_, Json::Value const &in_value)
{
	// TODO: clear out existing playlists

	for (unsigned int index = 0; index < in_value.size(); ++index)
	{
		Json::Value json_entry = in_value[index];

		playlists_entry &playlists_entry_ = playlists_.add_entry(json_entry["name"].asCString());
		load_from(playlists_entry_.playlist_, json_entry["playlist"]);
	}
}


void save_to(playlists const &playlists_, Json::Value &out_value)
{
	out_value = Json::Value(Json::arrayValue);

	BOOST_FOREACH(playlists_entry const &entry_, playlists_.get_entries())
	{
		Json::Value json_playlist;
		save_to(entry_.playlist_, json_playlist);

		Json::Value json_entry(Json::objectValue);
		json_entry["name"] = entry_.name.toStdString();
		json_entry["playlist"] = json_playlist;

		out_value.append(json_entry);
	}
}


}
}


#include "playlists.moc"

