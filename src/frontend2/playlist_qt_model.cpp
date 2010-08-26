#include <QApplication>
#include <QFont>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include "playlist_qt_model.hpp"
#include "playlists_ui.hpp"


namespace ion
{
namespace frontend
{


playlist_qt_model::playlist_qt_model(QObject *parent_, playlists_t &playlists_, playlist &playlist_):
	QAbstractListModel(parent_),
	playlist_(playlist_),
	playlists_(playlists_)
{
	active_playlist = playlists_.get_active_playlist();

	entry_added_signal_connection = playlist_.get_resource_added_signal().connect(boost::lambda::bind(&playlist_qt_model::entries_added, this, boost::lambda::_1, boost::lambda::_2));
	entry_removed_signal_connection = playlist_.get_resource_removed_signal().connect(boost::lambda::bind(&playlist_qt_model::entries_removed, this, boost::lambda::_1, boost::lambda::_2));
	active_playlist_changed_connection = playlists_.get_active_playlist_changed_signal().connect(boost::lambda::bind(&playlist_qt_model::active_playlist_changed, this, boost::lambda::_1));
}


playlist_qt_model::~playlist_qt_model()
{
	entry_added_signal_connection.disconnect();
	entry_removed_signal_connection.disconnect();
	active_playlist_changed_connection.disconnect();
}


QVariant playlist_qt_model::headerData(int section, Qt::Orientation orientation, int role) const
{
	if ((orientation != Qt::Horizontal) || (role != Qt::DisplayRole))
		return QVariant();

	switch (section)
	{
		case 0: return "Artist";
		case 1: return "Album";
		case 2: return "Title";
		case 3: return "Length";
		default: return QVariant();
	}
}


int playlist_qt_model::columnCount(QModelIndex const &parent) const
{
	// artist, album, song title, song length
	return 4;
}


QVariant playlist_qt_model::data(QModelIndex const &index, int role) const
{
	if ((index.column() >= 4) || (index.row() >= int(playlist_.get_num_entries())))
		return QVariant();


	playlist::entry_t const *entry = playlist_.get_entry(index.row());

	if (entry == 0)
		return QVariant();


	ion::uri const & entry_uri = boost::fusion::at_c < 0 > (*entry);

	switch (role)
	{
		case Qt::ToolTipRole:
		{
			switch (index.column())
			{
				case 2:
					return QString(entry_uri.get_full().c_str());
				default:
					break;
			}
			break;
		}


		case Qt::FontRole:
		{
			if (current_uri)
			{
				if ((&playlist_ == active_playlist) && (entry_uri == *current_uri))
				{
					QFont font = QApplication::font();
					font.setBold(true);
					return font;
				}
			}
			else
				return QVariant();

			break;
		}


		case Qt::DisplayRole:
		{
			metadata_t metadata = boost::fusion::at_c < 1 > (*entry);
			switch (index.column())
			{
				case 0: return QString::fromUtf8(get_metadata_value < std::string > (metadata, "artist", "").c_str());
				case 1: return QString::fromUtf8(get_metadata_value < std::string > (metadata, "album", "").c_str());
				case 2:
				{
					std::string title = get_metadata_value < std::string > (metadata, "title", "");
					if (title.empty())
						title = entry_uri.get_basename();

					return QString::fromUtf8(title.c_str());
				}
				case 3:
				{
					unsigned int num_ticks = get_metadata_value < unsigned int > (metadata, "num_ticks", 0);
					unsigned int num_ticks_per_second = get_metadata_value < unsigned int > (metadata, "num_ticks_per_second", 1);
					unsigned int length_in_seconds = num_ticks / num_ticks_per_second;
					unsigned int minutes = length_in_seconds / 60;
					unsigned int seconds = length_in_seconds % 60;
					return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
				}
				default: return QVariant();
			}

			break;
		}


		default: break;
	}


	return QVariant();
}


QModelIndex playlist_qt_model::parent(QModelIndex const &index) const
{
	return QModelIndex();
}


int playlist_qt_model::rowCount(QModelIndex const &parent) const
{
	return playlist_.get_num_entries();
}


void playlist_qt_model::current_uri_changed(uri_optional_t const &new_current_uri)
{
	playlist::index_optional_t old_uri_index, new_uri_index;

	if (current_uri)
		old_uri_index = get_uri_index(playlist_, *current_uri);
	if (new_current_uri)
		new_uri_index = get_uri_index(playlist_, *new_current_uri);

	current_uri = new_current_uri;

	if (old_uri_index)
		dataChanged(createIndex(*old_uri_index, 0), createIndex(*old_uri_index, columnCount(QModelIndex()) - 1));
	if (new_uri_index)
		dataChanged(createIndex(*new_uri_index, 0), createIndex(*new_uri_index, columnCount(QModelIndex()) - 1));
}


void playlist_qt_model::entries_added(uri_set_t const uris, bool const before)
{
	if (uris.empty())
		return;

	if (before)
	{
		int start = rowCount(QModelIndex());
		int end = start + uris.size() - 1;
		beginInsertRows(QModelIndex(), start, end);
	}
	else
		endInsertRows();
}


void playlist_qt_model::entries_removed(uri_set_t const, bool const before)
{
	// Using reset() instead of begin/endRemoveRows(), since the selection may be complex (that is, non-contiguous; example, 5 items, and 1 2 5 are selected)
	if (!before)
		reset();
}


void playlist_qt_model::active_playlist_changed(playlists_t::playlist_t *playlist_)
{
	active_playlist = playlist_;

	if (!current_uri)
		return;
		
	playlist::index_optional_t uri_index = get_uri_index(*playlist_, *current_uri);
	if (!uri_index)
		return;

	dataChanged(createIndex(*uri_index, 0), createIndex(*uri_index, columnCount(QModelIndex()) - 1));
}


playlist_qt_model::index_pair_optional_t playlist_qt_model::get_min_max_indices_from(uri_set_t const uris) const
{
	index_pair_optional_t indices;

	BOOST_FOREACH(uri const &uri_, uris)
	{
		playlist::index_optional_t uri_index = get_uri_index(playlist_, uri_);
		if (uri_index)
		{
			if (indices)
			{
				boost::fusion::at_c < 0 > (*indices) = std::min(boost::fusion::at_c < 0 > (*indices), *uri_index);
				boost::fusion::at_c < 1 > (*indices) = std::max(boost::fusion::at_c < 1 > (*indices), *uri_index);
			}
			else
				indices = index_pair_t(*uri_index, *uri_index);
		}
	}

	return indices;
}


}
}

