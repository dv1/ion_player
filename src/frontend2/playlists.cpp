#include <QTreeView>
#include "audio_frontend.hpp"
#include "playlists.hpp"
#include "playlist_qt_model.hpp"


namespace ion
{
namespace frontend
{


playlist_entry::playlist_entry(playlists &playlists_, QString const &name):
	playlists_(playlists_),
	view_widget(0)
{
	QTabWidget &tab_widget = playlists_.get_tab_widget();
	playlist_qt_model_ = new playlist_qt_model(&tab_widget, playlists_, playlist_);
	view_widget = new QTreeView(&tab_widget);
	view_widget->setRootIsDecorated(false);
	view_widget->setAlternatingRowColors(true);
	view_widget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	view_widget->setModel(playlist_qt_model_);
	tab_widget.addTab(view_widget, name);

	connect(view_widget, SIGNAL(doubleClicked(QModelIndex const &)), this, SLOT(play_song_in_row(QModelIndex const &)));
}


void playlist_entry::play_song_in_row(QModelIndex const &index)
{
}




playlists::playlists(QTabWidget &tab_widget, audio_frontend &audio_frontend_, QObject *parent):
	QObject(parent),
	tab_widget(tab_widget),
	active_entry(0),
	audio_frontend_(audio_frontend_)
{
}


playlists::~playlists()
{
}


playlist_entry& playlists::add_entry(QString const &playlist_name)
{
	playlist_entry_ptr_t new_entry(new playlist_entry(*this, playlist_name));
	playlist_entries.push_back(new_entry);
	return *new_entry;
}


void playlists::rename_entry(playlist_entry const &entry_to_be_renamed, QString const &new_name)
{
/*	entries_t::iterator iter = get_entry(entry_to_be_renamed.view_widget);
	if (iter == entries.end())
		return;

	iter->name = new_name;
	int tab_index = tab_widget.indexOf(iter->view_widget);
	if (tab_index != -1)
		tab_widget.setTabText(tab_index, new_name);*/
}


void playlists::remove_entry(playlist_entry const &entry_to_be_removed)
{
/*	entries_t::iterator iter = get_entry(entry_to_be_removed.view_widget);
	if (iter == entries.end())
		return;

	if (&(*iter) == active_entry)
	{
		audio_frontend_io_.set_current_playlist(0);
		active_entry = 0;
	}

	entries.erase(iter);*/
}


void playlists::set_active_entry(playlist_entry &new_active_entry)
{
	active_entry = &new_active_entry;
	audio_frontend_.set_current_playlist(&(new_active_entry.get_playlist()));
}


playlist_entry* playlists::get_currently_visible_entry()
{
/*	QWidget *page_widget = tab_widget.currentWidget();
	BOOST_FOREACH(playlists_entry &entry, entries)
	{
		if (entry.view_widget == page_widget)
			return &entry;
	}*/

	return 0;
}


void playlists::close_active_playlist(int index)
{
}


}
}


#include "playlists.moc"

