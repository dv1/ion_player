#include <QIcon>
#include "playlist_qt_model.hpp"


namespace ion
{
namespace frontend
{


playlist_qt_model::playlist_qt_model(QObject *parent_, simple_playlist &playlist_):
	QAbstractListModel(parent_),
	playlist_(playlist_)
{
	entry_added_signal_connection = playlist_.get_resource_added_signal().connect(boost::lambda::bind(&playlist_qt_model::entry_added, this, boost::lambda::_1, boost::lambda::_2));
	entry_removed_signal_connection = playlist_.get_resource_removed_signal().connect(boost::lambda::bind(&playlist_qt_model::entry_removed, this, boost::lambda::_1, boost::lambda::_2));
}


playlist_qt_model::~playlist_qt_model()
{
	entry_added_signal_connection.disconnect();
	entry_removed_signal_connection.disconnect();
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


	simple_playlist::entry const *entry = playlist_.get_entry(index.row());

	if (entry == 0)
		return QVariant();


	switch (role)
	{
		case Qt::ToolTipRole:
		{
			switch (index.column())
			{
				case 2:
					return QString(entry->uri_.get_full().c_str());
				default:
					break;
			}
			break;
		}


		case Qt::DecorationRole:
		{
			switch (index.column())
			{
				case 0:
				{
					if (current_uri)
					{
						if (entry->uri_ == *current_uri)
							return QIcon(":/icons/play");
					}

					return QVariant();
				}
				default: return QVariant();
			}

			break;
		}


		case Qt::DisplayRole:
		{
			switch (index.column())
			{
				case 0: return QString(get_metadata_value < std::string > (entry->metadata, "artist", "").c_str());
				case 1: return QString(get_metadata_value < std::string > (entry->metadata, "album", "").c_str());
				case 2:
				{
					std::string title = get_metadata_value < std::string > (entry->metadata, "title", "");
					if (title.empty())
						title = entry->uri_.get_basename();

					return QString(title.c_str());
				}
				case 3:
				{
					unsigned int num_ticks = get_metadata_value < unsigned int > (entry->metadata, "num_ticks", 0);
					unsigned int num_ticks_per_second = get_metadata_value < unsigned int > (entry->metadata, "num_ticks_per_second", 1);
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
/*	boost::optional < uint64_t > old_uri_index, new_uri_index;

	if (current_uri)
		old_uri_index = playlist_.get_entry_index(*current_uri);
	if (new_current_uri)
		new_uri_index = playlist_.get_entry_index(*new_current_uri);*/

	current_uri = new_current_uri;
	reset();
}


void playlist_qt_model::entry_added(uri_set_t const uri_, bool const before)
{
	reset(); // TODO: do something smarter than this
}


void playlist_qt_model::entry_removed(uri_set_t const uri_, bool const before)
{
	reset(); // TODO: do something smarter than this
}


}
}

