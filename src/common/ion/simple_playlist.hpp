#ifndef ION_SIMPLE_PLAYLIST_HPP
#define ION_SIMPLE_PLAYLIST_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/signals2/signal.hpp>

#include <ion/metadata.hpp>
#include <ion/uri.hpp>

#include "playlist.hpp"


namespace ion
{


class frontend_io;


class simple_playlist:
	public playlist
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
			boost::multi_index::sequenced < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_unique <
				boost::multi_index::tag < uri_tag >,
				boost::multi_index::member < entry, ion::uri, &entry::uri_ >
			>
		>
	> entries_t;


	virtual uri_optional_t get_succeeding_uri(uri const &uri_) const;
	virtual metadata_optional_t get_metadata_for(uri const &uri_) const;
	virtual void backend_resource_incompatibility(std::string const &backend_type, uri const &uri_);

	void add_to_frontend_io(frontend_io &frontend_io_);

	void add_entry(entry const &entry_);
	void remove_entry(entry const &entry_);


protected:
	entries_t entries;
	resource_event_signal_t resource_added_signal, resource_removed_signal;
};


}

#endif
