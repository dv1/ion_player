#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "playlist_qt_model.hpp"


namespace ion
{
namespace frontend
{


playlist_qt_model::playlist_qt_model(QObject *parent_, playlist &playlist_):
	QAbstractListModel(parent_),
	playlist_(playlist_)
{
	entry_added_signal_connection = playlist_.get_entry_added_signal().connect(boost::lambda::bind(&playlist_qt_model::entry_added, this, boost::lambda::_1));
	entry_removed_signal_connection = playlist_.get_entry_removed_signal().connect(boost::lambda::bind(&playlist_qt_model::entry_removed, this, boost::lambda::_1));
}


playlist_qt_model::~playlist_qt_model()
{
	entry_added_signal_connection.disconnect();
	entry_removed_signal_connection.disconnect();
}


int playlist_qt_model::columnCount(QModelIndex const &) const
{
	// artist, album, song title, song length
	return 4;
}


QVariant playlist_qt_model::data(QModelIndex const &index, int role) const
{
	if ((role != Qt::DisplayRole) || (index.column() >= 4) || (index.row() >= int(playlist_.get_num_entries())))
		return QVariant();

//	playlist::playlist_entry_optional_t entry = playlist_.get_entry(playlist_.get_entry_id(index.row()));

	switch (index.column())
	{
		case 0: return QString("");
		case 1: return QString("");
		case 2: return QString("");
		case 3: return QString("");
	}

	return QVariant();
}


QModelIndex playlist_qt_model::parent(QModelIndex const &) const
{
	return QModelIndex();
}


int playlist_qt_model::rowCount(QModelIndex const &parent) const
{
	return playlist_.get_num_entries();
}


void playlist_qt_model::entry_added(playlist::resource_id_t const resource_id)
{
	reset(); // TODO: use dataChanged() instead
}


void playlist_qt_model::entry_removed(playlist::resource_id_t const resource_id)
{
	reset(); // TODO: use dataChanged() instead
}


}
}

