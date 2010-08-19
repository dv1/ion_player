#ifndef ION_FRONTEND_PLAYLISTS_UI_HPP
#define ION_FRONTEND_PLAYLISTS_UI_HPP

#include <QObject>
#include <ion/playlists.hpp>
#include <ion/flat_playlist.hpp>
#include <ion/uri.hpp>

#include "misc_types.hpp"


class QTabWidget;
class QTreeView;


namespace ion
{
namespace frontend
{


class audio_frontend;


class playlist_entry_ui:
	public QObject
{
public:
	explicit playlist_entry_ui(QObject *parent, audio_frontend &audio_frontend_);
	void play_selected();
};


class playlists_ui:
	public QObject
{
public:
	explicit playlists_ui(QTabWidget &tab_widget, audio_frontend &audio_frontend_, QObject *parent);
	~playlists_ui();

	playlists_t & get_playlists() { return playlists_; }
	void current_uri_changed(uri_optional_t const &new_current_uri);
	playlist_entry_ui* get_currently_visible_entry_ui();


protected:
	void playlist_entry_added(playlists_t::playlist_entry_t const &playlist_entry);
	void playlist_entry_renamed(playlists_t::playlist_entry_t const &playlist_entry, std::string const &new_name);
	void playlist_entry_removed(playlists_t::playlist_entry_t const &playlist_entry);
	void active_playlist_changed(playlists_t::playlist_entry_t const &playlist_entry);


	playlists_t playlists_;
	QTabWidget &tab_widget;
	audio_frontend &audio_frontend_;
};


}
}


#endif

