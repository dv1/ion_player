#ifndef ION_FRONTEND_PLAYLIST_QT_MODEL_HPP
#define ION_FRONTEND_PLAYLIST_QT_MODEL_HPP

#include <QAbstractListModel>
#include <QVariant>
#include <boost/signals2/connection.hpp>
#include <ion/playlist.hpp>


namespace ion
{
namespace frontend
{


class playlists_ui;


class playlist_qt_model:
	public QAbstractListModel
{
public:
	explicit playlist_qt_model(QObject *parent_, playlists_ui &playlists_ui_, playlist &playlist_);
	~playlist_qt_model();


	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int columnCount(QModelIndex const &parent) const;
	virtual QVariant data(QModelIndex const &index, int role) const;
	virtual QModelIndex parent(QModelIndex const &index) const;
	virtual int rowCount(QModelIndex const &parent) const;

	void current_uri_changed(uri_optional_t const &new_current_uri);


protected:
	void entries_added(uri_set_t const uri_, bool const before);
	void entries_removed(uri_set_t const uri_, bool const before);
	void active_playlist_changed(playlist *new_active_playlist);


	playlist &playlist_, *active_playlist;
	boost::signals2::connection
		entry_added_signal_connection,
		entry_removed_signal_connection,
		active_playlist_changed_connection;
	uri_optional_t current_uri;
};


}
}


#endif

