#ifndef ION_FRONTEND_PLAYLISTS_HPP
#define ION_FRONTEND_PLAYLISTS_HPP

#include <QObject>

#include <ion/simple_playlist.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <json/value.h>


class QTabWidget;
class QTreeView;


namespace ion
{
namespace frontend
{


class audio_frontend_io;
class playlist_qt_model;
class playlists;


class playlists_entry:
	public QObject
{
	Q_OBJECT
public:

	QString name;
	simple_playlist playlist_;
	QTreeView *view_widget;
	playlist_qt_model *playlist_qt_model_;

	explicit playlists_entry(playlists &playlists_, QString const &name);
	~playlists_entry();

	void remove_selected();


protected slots:
	void view_row_doubleclicked(QModelIndex const &index);


protected:
	playlists &playlists_;
};


class playlists:
	public QObject
{
public:
	// TODO: use a better structure (multi-index? map?) that allows for indexing by the view widget as well
	typedef boost::ptr_vector < playlists_entry > entries_t;


	explicit playlists(QTabWidget &tab_widget, audio_frontend_io &audio_frontend_io_, QObject *parent);

	playlists_entry& add_entry(QString const &playlist_name);
	void remove_entry(QTreeView *entry_view_widget);

	void set_active_entry(playlists_entry &new_active_entry);
	playlists_entry *get_currently_visible_entry();

	inline entries_t const & get_entries() const { return entries; }

	inline audio_frontend_io& get_audio_frontend_io() { return audio_frontend_io_; }
	inline QTabWidget& get_tab_widget() { return tab_widget; }


protected:
	audio_frontend_io &audio_frontend_io_;
	QTabWidget &tab_widget;
	entries_t entries;
	playlists_entry *active_entry; // the active entry is connected to the backend process
};




void load_from(playlists &playlists_, Json::Value const &in_value);
void save_to(playlists const &playlists_, Json::Value &out_value);


}
}


#endif
