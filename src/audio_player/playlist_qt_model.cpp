/**************************************************************************

    Copyright (C) 2010  Carlos Rafael Giani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**************************************************************************/


#include <QApplication>
#include <QBrush>
#include <QFont>
#include <QIcon>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include "playlist_qt_model.hpp"
#include "playlists_ui.hpp"


namespace ion
{
namespace audio_player
{


playlist_qt_model::playlist_qt_model(QObject *parent_, playlists_t &playlists_, playlist &playlist_):
	QAbstractListModel(parent_),
	playlist_(playlist_),
	playlists_(playlists_)
{
	active_playlist = get_active_playlist(playlists_);

	entry_added_signal_connection = get_resource_added_signal(playlist_).connect(boost::phoenix::bind(&playlist_qt_model::entries_added, this, boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));
	entry_removed_signal_connection = get_resource_removed_signal(playlist_).connect(boost::phoenix::bind(&playlist_qt_model::entries_removed, this, boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));
	metadata_changed_signal_connection = get_resource_metadata_changed_signal(playlist_).connect(boost::phoenix::bind(&playlist_qt_model::metadata_changed, this, boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));
	resource_incompatible_connection = get_resource_incompatible_signal(playlist_).connect(boost::phoenix::bind(&playlist_qt_model::resource_incompatible, this, boost::phoenix::arg_names::arg1));
	all_resources_changed_connection = get_all_resources_changed_signal(playlist_).connect(boost::phoenix::bind(&playlist_qt_model::all_resources_changed, this, boost::phoenix::arg_names::arg1));
	active_playlist_changed_connection = get_active_playlist_changed_signal(playlists_).connect(boost::phoenix::bind(&playlist_qt_model::active_playlist_changed, this, boost::phoenix::arg_names::arg1));
}


playlist_qt_model::~playlist_qt_model()
{
	entry_added_signal_connection.disconnect();
	entry_removed_signal_connection.disconnect();
	metadata_changed_signal_connection.disconnect();
	resource_incompatible_connection.disconnect();
	all_resources_changed_connection.disconnect();
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
		case 4: return "Repeat";
		default: return QVariant();
	}
}


int playlist_qt_model::columnCount(QModelIndex const &parent) const
{
	// artist, album, song title, song length, repeat mode
	return 5;
}


QVariant playlist_qt_model::data(QModelIndex const &index, int role) const
{
	if ((index.column() >= 5) || (index.row() >= int(get_num_entries(playlist_))))
		return QVariant();


	playlist_traits < playlist > ::entry_t const *entry = get_entry(playlist_, index.row());

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


		case Qt::BackgroundRole:
			if (boost::fusion::at_c < 2 > (*entry))
				return QBrush(Qt::darkRed);
			break;


		case Qt::FontRole:
		{
			if (current_uri)
			{
				if (entry_uri == *current_uri)
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


		case Qt::DecorationRole:
		{
			metadata_t metadata = boost::fusion::at_c < 1 > (*entry);
			switch (index.column())
			{
				case 4:
				{
					int loop_mode = get_metadata_value < int > (metadata, "loop_mode", -1);
					if (loop_mode < 0)
						return QVariant();
					else
						return QIcon(":/icons/repeat");
				}
				default: return QVariant();
			}
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

					if ((num_ticks == 0) || (num_ticks_per_second == 0))
						return QString("--");
					else
					{
						unsigned int length_in_seconds = num_ticks / num_ticks_per_second;
						unsigned int minutes = length_in_seconds / 60;
						unsigned int seconds = length_in_seconds % 60;
						return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
					}
				}
				case 4:
				{
					int loop_mode = get_metadata_value < int > (metadata, "loop_mode", -1);
					if (loop_mode < 0)
						return QVariant();
					else if (loop_mode == 0)
						return QString("inf.");
					else
						return QString::number(loop_mode);
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
	return get_num_entries(playlist_);
}


void playlist_qt_model::current_uri_changed(uri_optional_t const &new_current_uri)
{
	playlist_traits < playlist > ::index_optional_t old_uri_index, new_uri_index;

	if (current_uri)
		old_uri_index = get_entry_index(playlist_, *current_uri);
	if (new_current_uri)
		new_uri_index = get_entry_index(playlist_, *new_current_uri);

	current_uri = new_current_uri;

	if (old_uri_index)
		dataChanged(createIndex(*old_uri_index, 0), createIndex(*old_uri_index, columnCount(QModelIndex()) - 1));
	if (new_uri_index)
		dataChanged(createIndex(*new_uri_index, 0), createIndex(*new_uri_index, columnCount(QModelIndex()) - 1));
}


bool playlist_qt_model::is_currently_playing() const
{
	if (current_uri)
	{
		playlist_traits < playlist > ::entry_t const *entry = get_entry(playlist_, *current_uri);
		return (entry != 0);
	}
	else
		return false;
}


QModelIndex playlist_qt_model::get_current_uri_model_index() const
{
	if (!current_uri)
		return QModelIndex();

	playlist_traits < playlist > ::index_optional_t cur_uri_index = get_entry_index(playlist_, *current_uri);
	if (!cur_uri_index)
		return QModelIndex();

	return createIndex(*cur_uri_index, 0);
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


void playlist_qt_model::metadata_changed(uri_set_t const uris, bool const before)
{
	if (before)
		return;

	playlist_traits < playlist > ::index_optional_t start, end;

	BOOST_FOREACH(uri const &uri_, uris)
	{
		playlist_traits < playlist > ::index_optional_t uri_index = get_entry_index(playlist_, uri_);
		if (!uri_index)
			return;

		if (start)
			start = std::min(*start, *uri_index);
		else
			start = *uri_index;

		if (end)
			end = std::max(*end, *uri_index);
		else
			end = *uri_index;
	}

	if (start && end)
		dataChanged(createIndex(*start, 0), createIndex(*end, columnCount(QModelIndex()) - 1));
}


void playlist_qt_model::resource_incompatible(uri const uri_)
{
	playlist_traits < playlist > ::index_optional_t uri_index = get_entry_index(playlist_, uri_);
	if (!uri_index)
		return;

	dataChanged(createIndex(*uri_index, 0), createIndex(*uri_index, columnCount(QModelIndex()) - 1));
}


void playlist_qt_model::all_resources_changed(bool const before)
{
	if (!before)
		reset();
}


void playlist_qt_model::active_playlist_changed(playlists_traits < playlists_t > ::playlist_t *playlist_)
{
	active_playlist = playlist_;

	if (!current_uri)
		return;

	playlist_traits < playlist > ::index_optional_t uri_index = get_entry_index(*playlist_, *current_uri);
	if (!uri_index)
		return;

	dataChanged(createIndex(*uri_index, 0), createIndex(*uri_index, columnCount(QModelIndex()) - 1));
}


playlist_qt_model::index_pair_optional_t playlist_qt_model::get_min_max_indices_from(uri_set_t const uris) const
{
	index_pair_optional_t indices;

	BOOST_FOREACH(uri const &uri_, uris)
	{
		playlist_traits < playlist > ::index_optional_t uri_index = get_entry_index(playlist_, uri_);
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

