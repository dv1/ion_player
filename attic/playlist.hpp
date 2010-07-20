#ifndef ION_FRONTEND_PLAYLIST_HPP
#define ION_FRONTEND_PLAYLIST_HPP

#include <stdint.h>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <ion/metadata.hpp>
#include <ion/uri.hpp>

#include "unique_ids.hpp"


namespace ion
{
namespace frontend
{


class playlist:
	private boost::noncopyable
{
public:
	/*
	On the issue of resource IDs:
	since a playlist can have entries with identical UIDs, they cannot be used for identifying resources in the playlist. Solutions for this problem are:
	- append an index to each UID, to distinguish identical UID entries (the order of the identical UIDs does not need to be stable)
	- generate dedicated IDs
	the latter solution is preferred, since the former has problems: it does not fit well in typical playlist structures, requires extra logic built inside
	the playlist structure to assign the indices; when an entry is removed, all greater indices have to be decremented, leading to costly reference updates everywhere;
	comparisons always need to take two values into account.
	Generated IDs avoid these problems because their generation is done separately. When an entry is removed, no other IDs need to be modified, since dedicated IDs
	are generated randomly, and stored in an STL set to be able to check for duplicates in logarithmic time.
	uint64_t is chosen as the ID type to allow for a vast maximal amount of unique IDs (2^64 ~ 2^64 = 1.84467441 x 10^19 ~ 18.4 quintillion IDs).
	Another idea would be to exploit the fact that playlist entries usually reside in the heap, and therefore reside in memory blocks with unique pointers. However,
	these are unique in a heap only, meaning that after reloading and saving a playlist, the IDs would change. Furthermore, the pointers may not be accessible.
	*/

	typedef uint64_t resource_id_t;
	typedef boost::optional < resource_id_t > resource_id_optional_t;

	struct current_resource_changed_data
	{
		resource_id_t current_resource_id, next_resource_id;
		ion::uri current_resource_uri, next_resource_uri;
		ion::metadata_t current_resource_metadata, next_resource_metadata;
	};

	typedef boost::signals2::signal < void(current_resource_changed_data const &data) > current_resource_changed_signal_t;
	typedef boost::signals2::signal < void(resource_id_optional_t const &next_resource_id, ion::uri const &next_resource_uri) > next_resource_changed_signal_t;
	typedef boost::signals2::signal < void(resource_id_t const) > playlist_entry_signal_t;

	struct playlist_entry
	{
		resource_id_t resource_id;
		ion::uri uri_;
		metadata_t metadata;
	};
	typedef boost::optional < playlist_entry > playlist_entry_optional_t;

	typedef unique_ids < resource_id_t > unique_ids_t;


	explicit playlist();
	~playlist();

	void transition();
	void play(resource_id_t const &resource_id);

	resource_id_optional_t get_entry_id(uint64_t const nr);
	uint64_t get_num_entries();
	playlist_entry_optional_t get_entry(resource_id_t const &resource_id);

	inline current_resource_changed_signal_t & get_current_resource_changed_signal() { return current_resource_changed_signal; }
	inline next_resource_changed_signal_t & get_next_resource_changed_signal() { return next_resource_changed_signal; }
	inline playlist_entry_signal_t & get_entry_added_signal() { return entry_added_signal; }
	inline playlist_entry_signal_t & get_entry_removed_signal() { return entry_removed_signal; }


protected:
	struct by_index_tag {};
	struct by_resource_id_tag {};

	typedef boost::multi_index::multi_index_container <
		playlist_entry,
		boost::multi_index::indexed_by <
			boost::multi_index::random_access < boost::multi_index::tag < by_index_tag > >,
			boost::multi_index::ordered_unique <
				boost::multi_index::tag < by_resource_id_tag >,
				boost::multi_index::member < playlist_entry, resource_id_t, &playlist_entry::resource_id >
			>
		>
	> playlist_entries_t;


	current_resource_changed_signal_t current_resource_changed_signal;
	next_resource_changed_signal_t next_resource_changed_signal;
	playlist_entry_signal_t entry_added_signal;
	playlist_entry_signal_t entry_removed_signal;

	playlist_entries_t playlist_entries;
	unique_ids_t unique_ids_;
};


}
}


#endif

