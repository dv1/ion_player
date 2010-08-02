#ifndef ION_SIMPLE_PLAYLIST_HPP
#define ION_SIMPLE_PLAYLIST_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/signals2/signal.hpp>

#include <ion/metadata.hpp>
#include <ion/uri.hpp>

#include "playlist.hpp"


namespace ion
{


class simple_playlist
{
public:
	typedef boost::signals2::signal < void(ion::uri const &uri_) > resource_event_signal_t;
	typedef boost::function < void(uri_optional_t const &new_current_uri) > current_uri_changed_callback_t;


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
			boost::multi_index::sequenced < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_unique <
				boost::multi_index::tag < uri_tag >,
				boost::multi_index::member < entry, ion::uri, &entry::uri_ >
			>
		>
	> entries_t;





	inline current_uri_changed_callback_t & get_current_uri_changed_callback() { return current_uri_changed_callback; }
	inline resource_event_signal_t & get_resource_added_signal() { return resource_added_signal; }
	inline resource_event_signal_t & get_resource_removed_signal() { return resource_removed_signal; }

	metadata_optional_t get_metadata_for(uri const &uri_);
	uri_optional_t get_succeeding_uri(uri const &uri_);
	void mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type);

	void add_entry(entry const &entry_);
	void remove_entry(entry const &entry_);


protected:
	current_uri_changed_callback_t current_uri_changed_callback;
	resource_event_signal_t resource_added_signal, resource_removed_signal;
};


}

#endif

