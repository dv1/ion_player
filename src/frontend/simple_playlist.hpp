#ifndef ION_FRONTEND_SIMPLE_PLAYLIST_HPP
#define ION_FRONTEND_SIMPLE_PLAYLIST_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#include <ion/metadata.hpp>
#include <ion/uri.hpp>

#include "backend_handler.hpp"

namespace ion
{
namespace frontend
{


class simple_playlist:
	public playlist
{
public:
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


	entries_t entries;
};


}
}

#endif

