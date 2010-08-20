#include "playlists_ui.hpp"


namespace ion
{
namespace frontend
{


playlist_ui::playlist_ui(QObject *parent, audio_frontend &audio_frontend_)
{
}


void playlist_ui::play_selected()
{
}




playlists_ui::playlists_ui(QTabWidget &tab_widget, audio_frontend &audio_frontend_, QObject *parent):
	QObject(parent),
	tab_widget(tab_widget),
	audio_frontend_(audio_frontend_)
{
}


playlists_ui::~playlists_ui()
{
}


void playlists_ui::current_uri_changed(uri_optional_t const &new_current_uri)
{
}


playlist_ui* playlists_ui::get_currently_visible_playlist_ui()
{
	return 0;
}


void playlists_ui::playlist_added(playlists_t::playlist_t &playlist_)
{
}


void playlists_ui::playlist_removed(playlists_t::playlist_t &playlist_)
{
}


void playlists_ui::active_playlist_changed(playlists_t::playlist_t *playlist_)
{
}


}
}

