#ifndef ION_SIMPLE_PLAYLIST_HPP
#define ION_SIMPLE_PLAYLIST_HPP

#include <iostream>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/signals2/signal.hpp>

#include <json/value.h>

#include <ion/metadata.hpp>
#include <ion/uri.hpp>
#include <ion/unique_ids.hpp>


namespace ion
{


class simple_playlist
{
public:
	typedef boost::signals2::signal < void(ion::uri const &uri_) > resource_event_signal_t;


	struct entry
	{
		ion::uri uri_;
		metadata_t metadata;

		explicit entry() {}
		explicit entry(ion::uri const &uri_, metadata_t const &metadata): uri_(uri_), metadata(metadata) {}
	};

	struct sequence_tag {};
	struct uri_tag {};

	typedef boost::multi_index::multi_index_container <
		entry,
		boost::multi_index::indexed_by <
			boost::multi_index::random_access < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_unique <
				boost::multi_index::tag < uri_tag >,
				boost::multi_index::member < entry, ion::uri, &entry::uri_ >
			>
		>
	> entries_t;

	typedef boost::iterator_range < entries_t::index < sequence_tag > ::type::const_iterator > entry_range_t;




	~simple_playlist();

	inline resource_event_signal_t & get_resource_added_signal() { return resource_added_signal; }
	inline resource_event_signal_t & get_resource_removed_signal() { return resource_removed_signal; }
	inline resource_event_signal_t & get_resource_metadata_changed_signal() { return resource_metadata_changed_signal; }

	metadata_optional_t get_metadata_for(uri const &uri_) const;
	uri_optional_t get_preceding_uri(uri const &uri_) const;
	uri_optional_t get_succeeding_uri(uri const &uri_) const;
	void mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type);

	void add_entry(entry entry_, bool const emit_signal = true);
	void remove_entry(entry const &entry_, bool const emit_signal = true);
	void remove_entry(uri const &uri_, bool const emit_signal = true);
	void set_resource_metadata(uri const &uri_, metadata_t const & new_metadata);

	uint64_t get_num_entries() const;
	entry const * get_entry(uint64_t const nr) const;
	entry const * get_entry(uri const &uri_) const;
	boost::optional < uint64_t > get_entry_index(uri const &uri_) const;
	entry_range_t get_entry_range() const;


protected:
	typedef unique_ids < long > unique_ids_t;
	typedef boost::optional < unique_ids_t::id_t > unique_id_optional_t;

	unique_id_optional_t get_uri_id(uri const &uri_);
	void set_uri_id(uri &uri_, unique_ids_t::id_t const &new_id, bool const check_for_old_id = true);


	typedef entries_t::index < sequence_tag > ::type entry_sequence_t;
	entry_sequence_t::const_iterator get_seq_iterator_for(uri const &uri_) const;


	entries_t entries;
	resource_event_signal_t resource_added_signal, resource_removed_signal, resource_metadata_changed_signal;
	unique_ids_t unique_ids_;
};


void load_from(simple_playlist &playlist_, Json::Value const &in_value);
void save_to(simple_playlist const &playlist_, Json::Value &out_value);


simple_playlist::resource_event_signal_t & get_resource_added_signal(simple_playlist &playlist);
simple_playlist::resource_event_signal_t & get_resource_removed_signal(simple_playlist &playlist);
metadata_optional_t get_metadata_for(simple_playlist const &playlist, uri const &uri_);
uri_optional_t get_preceding_uri(simple_playlist const &playlist, uri const &uri_);
uri_optional_t get_succeeding_uri(simple_playlist const &playlist, uri const &uri_);
void mark_backend_resource_incompatibility(simple_playlist &playlist, uri const &uri_, std::string const &backend_type);


}

#endif

