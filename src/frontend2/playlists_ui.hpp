#ifndef ION_FRONTEND_PLAYLISTS_UI_HPP
#define ION_FRONTEND_PLAYLISTS_UI_HPP

#include <QObject>
#include <ion/playlists.hpp>
#include <ion/flat_playlist.hpp>


class QTabWidget;
class QTreeView;


namespace ion
{
namespace frontend
{


class audio_frontend;


class playlists_ui:
	public QObject
{
public:
	typedef playlists < flat_playlist > playlists_t;


	explicit playlists_ui(QTabWidget &tab_widget, audio_frontend &audio_frontend_, QObject *parent);
	~playlists_ui();


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

