#ifndef ION_PLAYLISTS_HPP
#define ION_PLAYLISTS_HPP

#include <QObject>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/shared_ptr.hpp>

#include <ion/playlist.hpp>
#include <ion/uri.hpp>
#include <ion/flat_playlist.hpp>


class QTabWidget;
class QTreeView;


namespace ion
{
namespace frontend
{


class playlists;
class audio_frontend;
class playlist_qt_model;


class playlist_entry:
	public QObject
{
	Q_OBJECT
public:
	explicit playlist_entry(playlists &playlists_, QString const &name);

	inline playlist& get_playlist() { return playlist_; }
	inline QTreeView* get_view_widget() const { return view_widget; }


protected slots:
	void play_song_in_row(QModelIndex const &index);


protected:
	playlists &playlists_;
	flat_playlist playlist_;
	QTreeView *view_widget;
	QString name;
	playlist_qt_model *playlist_qt_model_;
};


typedef boost::shared_ptr < playlist_entry > playlist_entry_ptr_t;


class playlists:
	public QObject
{
	Q_OBJECT

	struct get_view_widget_from_entry
	{
		typedef QTreeView* result_type;

		result_type operator()(playlist_entry_ptr_t const &entry) const
		{
			return entry->get_view_widget();
		}
	};


public:
	struct sequence_tag {};
	struct view_widget_tag {};


	typedef boost::signals2::signal < void(playlist *new_active_playlist) > active_playlist_changed_signal_t;
	typedef boost::multi_index::multi_index_container <
		playlist_entry_ptr_t,
		boost::multi_index::indexed_by <
			boost::multi_index::random_access < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_unique <
				boost::multi_index::tag < view_widget_tag >,
				get_view_widget_from_entry
			>
		>
	> playlist_entries_t;


	explicit playlists(QTabWidget &tab_widget, audio_frontend &audio_frontend_, QObject *parent);
  	~playlists();


	playlist_entry& add_entry(QString const &playlist_name);
	void rename_entry(playlist_entry const &entry_to_be_renamed, QString const &new_name);
	void remove_entry(playlist_entry const &entry_to_be_removed);


	inline active_playlist_changed_signal_t & get_active_playlist_changed_signal() { return active_playlist_changed_signal; }
	inline playlist * get_active_playlist() { return (active_entry != 0) ? &(active_entry->get_playlist()) : 0; }

	void set_active_entry(playlist_entry &new_active_entry);
	playlist_entry *get_currently_visible_entry();

	inline playlist_entries_t const & get_playlist_entries() const { return playlist_entries; }

	inline audio_frontend& get_audio_frontend() { return audio_frontend_; }
	inline QTabWidget& get_tab_widget() { return tab_widget; }


protected slots:
	void close_active_playlist(int index);


protected:
	QTabWidget &tab_widget;
	playlist_entry *active_entry;
	audio_frontend &audio_frontend_;
	active_playlist_changed_signal_t active_playlist_changed_signal;
	playlist_entries_t playlist_entries;
};


}
}


#endif

