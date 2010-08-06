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
	view_widget = new QTreeView(&tab_widget);
	view_widget->setRootIsDecorated(false);
	view_widget->setAlternatingRowColors(true);
	playlist_qt_model_ = new playlist_qt_model(&tab_widget, playlist_);
	view_widget->setModel(playlist_qt_model_);
	tab_widget.addTab(view_widget, name);

	connect(view_widget, SIGNAL(doubleClicked(QModelIndex const &)), this, SLOT(view_row_doubleclicked(QModelIndex const &)));
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


void playlists_entry::view_row_doubleclicked(QModelIndex const &index)
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


}
}


#include "playlists.moc"

