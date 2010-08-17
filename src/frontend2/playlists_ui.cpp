#include "playlists_ui.hpp"


namespace ion
{
namespace frontend
{


playlists_ui::playlists_ui(QTabWidget &tab_widget, audio_frontend &audio_frontend_, QObject *parent):
	QObject(parent),
	tab_widget(tab_widget),
	audio_frontend_(audio_frontend_)
{
}


playlists_ui::~playlists_ui()
{
}


void playlists_ui::playlist_entry_added(playlists_t::playlist_entry_t const &playlist_entry)
{
}


void playlists_ui::playlist_entry_renamed(playlists_t::playlist_entry_t const &playlist_entry, std::string const &new_name)
{
}


void playlists_ui::playlist_entry_removed(playlists_t::playlist_entry_t const &playlist_entry)
{
}


void playlists_ui::active_playlist_changed(playlists_t::playlist_entry_t const &playlist_entry)
{
}


}
}

