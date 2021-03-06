/****************************************************************************

Copyright (c) 2010 Carlos Rafael Giani

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

****************************************************************************/


#ifndef ION_FLAT_PLAYLIST_HPP
#define ION_FLAT_PLAYLIST_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/range/iterator_range.hpp>
#include <ion/unique_ids.hpp>

#include "playlist.hpp"


namespace ion
{


class flat_playlist:
	public playlist
{
protected:
	struct get_uri_from_entry
	{
		typedef ion::uri result_type;

		result_type const & operator()(entry_t const &entry) const
		{
			return boost::fusion::at_c < 0 > (entry);
		}
	};


public:
	struct sequence_tag {};
	struct uri_tag {};


	typedef boost::multi_index::multi_index_container <
		entry_t,
		boost::multi_index::indexed_by <
			boost::multi_index::random_access < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_unique <
				boost::multi_index::tag < uri_tag >,
				get_uri_from_entry
			>
		>
	> entries_t;
	typedef unique_ids < long > unique_ids_t;

	typedef boost::iterator_range < entries_t::index < sequence_tag > ::type::const_iterator > entry_range_t;


	explicit flat_playlist(unique_ids_t &unique_ids_);


	virtual metadata_optional_t get_metadata_for(uri const &uri_) const;
	virtual uri_optional_t get_preceding_uri(uri const &uri_) const;
	virtual uri_optional_t get_succeeding_uri(uri const &uri_) const;
	virtual void mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type);

	virtual num_entries_t get_num_entries() const;
	virtual entry_t const * get_entry(index_t const index) const;
	virtual entry_t const * get_entry(uri const &uri_) const;
	virtual index_optional_t get_entry_index(uri const &uri_) const;

	virtual bool is_view() const { return false; }
	virtual bool is_mutable() const { return true; }
	virtual std::string get_prefix() const { return ""; }

	entry_range_t get_entry_range() const;

	void add_entry(entry_t const &entry_, bool const emit_signal);
	void remove_entry(entry_t const &entry_, bool const emit_signal);
	void remove_entry(uri const &uri_, bool const emit_signal);
	void remove_entries(uri_set_t const &uris, bool const emit_signal);
	void set_resource_metadata(uri const &uri_, metadata_t const & new_metadata);

	void clear_entries(bool const emit_signal);


	void load_from(Json::Value const &in_value);
	void save_to(Json::Value &out_value) const;


protected:
	typedef boost::optional < unique_ids_t::id_t > unique_id_optional_t;


	unique_id_optional_t get_uri_id(uri const &uri_);
	void set_uri_id(uri &uri_, unique_ids_t::id_t const &new_id, bool const check_for_old_id = true);

	typedef entries_t::index < sequence_tag > ::type entry_sequence_t;
	typedef entries_t::index < uri_tag > ::type entries_by_uri_t;
	entries_by_uri_t::const_iterator get_uri_iterator_for(uri const &uri_) const;
	entries_by_uri_t::iterator get_uri_iterator_for(uri const &uri_);


	entries_t entries;
	unique_ids_t &unique_ids_;
};


}


#endif

