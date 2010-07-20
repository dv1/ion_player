#ifndef ION_FRONTEND_PLAYLIST_QT_MODEL_HPP
#define ION_FRONTEND_PLAYLIST_QT_MODEL_HPP

#include <QAbstractListModel>
#include <QVariant>
#include <boost/signals2/connection.hpp>
#include "playlist.hpp"


namespace ion
{
namespace frontend
{


class playlist_qt_model:
	public QAbstractListModel
{
public:
	explicit playlist_qt_model(QObject *parent_, playlist &playlist_);
	~playlist_qt_model();


	virtual int columnCount(QModelIndex const &parent) const;
	virtual QVariant data(QModelIndex const &index, int role) const;
	virtual QModelIndex parent(QModelIndex const &index) const;
	virtual int rowCount(QModelIndex const &parent) const;


protected:
	void entry_added(playlist::resource_id_t const resource_id);
	void entry_removed(playlist::resource_id_t const resource_id);


	playlist &playlist_;
	boost::signals2::connection
		entry_added_signal_connection,
		entry_removed_signal_connection;
};


}
}


#endif

