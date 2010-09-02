#ifndef ION_FRONTEND_PLAYLISTS_UI_HPP
#define ION_FRONTEND_PLAYLISTS_UI_HPP

#include <vector>
#include <QObject>
#include <boost/signals2/connection.hpp>
#include <ion/playlists.hpp>
#include <ion/uri.hpp>

#include "misc_types.hpp"


class QTabWidget;
class QTreeView;


namespace ion
{
namespace frontend
{


class audio_frontend;
class playlists_ui;
class playlist_qt_model;


class playlist_ui:
	public QObject
{
	Q_OBJECT
public:
	explicit playlist_ui(QObject *parent, playlist &playlist_, playlists_ui &playlists_ui_);
	~playlist_ui();

	void play_selected();
	void remove_selected();

	inline playlist & get_playlist() { return playlist_; }
	inline QTreeView* get_view_widget() { return view_widget; }
	inline playlist_qt_model* get_playlist_qt_model() { return playlist_qt_model_; }


protected slots:
	void play_song_in_row(QModelIndex const &index);


protected:
	void playlist_renamed(std::string const &new_name);


	playlist &playlist_;
	playlists_ui &playlists_ui_;
	QTreeView *view_widget;
	playlist_qt_model *playlist_qt_model_;
	boost::signals2::connection playlist_renamed_signal_connection, current_uri_changed_signal_connection;
};


class playlists_ui:
	public QObject
{
	Q_OBJECT
public:
	explicit playlists_ui(QTabWidget &tab_widget, audio_frontend &audio_frontend_, QObject *parent);
	~playlists_ui();

	QTabWidget& get_tab_widget() { return tab_widget; }
	playlists_t & get_playlists() { return playlists_; }
	audio_frontend& get_audio_frontend() { return audio_frontend_; }
	playlist_ui* get_currently_visible_playlist_ui();
	playlist* get_currently_visible_playlist();


protected slots:
	void close_current_playlist(int index);


protected:
	void playlist_added(playlists_traits < playlists_t > ::playlist_t &playlist_);
	void playlist_removed(playlists_traits < playlists_t > ::playlist_t &playlist_);


	playlists_t playlists_;
	QTabWidget &tab_widget;
	audio_frontend &audio_frontend_;

	typedef std::vector < playlist_ui* > playlist_uis_t;
	playlist_uis_t playlist_uis;

	boost::signals2::connection
		playlist_added_signal_connection,
		playlist_removed_signal_connection,
		active_playlist_changed_connection;
};


}
}


#endif

