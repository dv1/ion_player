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


#ifndef ION_AUDIO_PLAYER_PLAYLIST_QT_MODEL_HPP
#define ION_AUDIO_PLAYER_PLAYLIST_QT_MODEL_HPP

#include <QAbstractListModel>
#include <QVariant>
#include <boost/signals2/connection.hpp>
#include <boost/fusion/container/vector.hpp>
#include <ion/playlist.hpp>

#include "misc_types.hpp"


namespace ion
{
namespace audio_player
{


class playlist_qt_model:
	public QAbstractListModel
{
public:
	explicit playlist_qt_model(QObject *parent_, playlists_t &playlists_, playlist &playlist_);
	~playlist_qt_model();


	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int columnCount(QModelIndex const &parent) const;
	virtual QVariant data(QModelIndex const &index, int role) const;
	virtual QModelIndex parent(QModelIndex const &index) const;
	virtual int rowCount(QModelIndex const &parent) const;

	void current_uri_changed(uri_optional_t const &new_current_uri);
	bool is_currently_playing() const;
	QModelIndex get_current_uri_model_index() const;


protected:
	void entries_added(uri_set_t const uris, bool const before);
	void entries_removed(uri_set_t const uris, bool const before);
	void metadata_changed(uri_set_t const uris, bool const before);
	void resource_incompatible(uri const uri_);
	void all_resources_changed(bool const before);
	void active_playlist_changed(playlists_traits < playlists_t > ::playlist_t *playlist_);

	typedef boost::fusion::vector2 < playlist_traits < playlist > ::index_t, playlist_traits < playlist > ::index_t > index_pair_t;
	typedef boost::optional < index_pair_t > index_pair_optional_t;
	index_pair_optional_t get_min_max_indices_from(uri_set_t const uris) const;


	playlist &playlist_;
	playlists_t &playlists_;
	playlist const *active_playlist;
	boost::signals2::connection
		entry_added_signal_connection,
		entry_removed_signal_connection,
		metadata_changed_signal_connection,
		resource_incompatible_connection,
		all_resources_changed_connection,
		active_playlist_changed_connection;
	uri_optional_t current_uri;
};


}
}


#endif

