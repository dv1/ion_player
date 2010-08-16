#ifndef ION_PLAYLIST_HPP
#define ION_PLAYLIST_HPP

#include <stdint.h>
#include <string>

#include <boost/signals2/signal.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/optional.hpp>

#include <ion/metadata.hpp>
#include <ion/uri.hpp>


namespace ion
{


class playlist
{
public:
	typedef boost::signals2::signal < void(uri_set_t const &uris) > resource_event_signal_t;
	typedef boost::fusion::vector2 < ion::uri, metadata_t > entry_t;
	typedef boost::optional < uint64_t > index_optional_t;


	virtual ~playlist() {}


	inline resource_event_signal_t & get_resource_added_signal() { return resource_added_signal; }
	inline resource_event_signal_t & get_resource_removed_signal() { return resource_removed_signal; }
	inline resource_event_signal_t & get_resource_metadata_changed_signal() { return resource_metadata_changed_signal; }

	virtual metadata_optional_t get_metadata_for(uri const &uri_) const = 0;
	virtual uri_optional_t get_preceding_uri(uri const &uri_) const = 0;
	virtual uri_optional_t get_succeeding_uri(uri const &uri_) const = 0;
	virtual void mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type) = 0;

	virtual uint64_t get_num_entries() const = 0;
	virtual entry_t const * get_entry(uint64_t const nr) const = 0;
	virtual entry_t const * get_entry(uri const &uri_) const = 0;
	virtual index_optional_t get_entry_index(uri const &uri_) const = 0;


protected:
	resource_event_signal_t resource_added_signal, resource_removed_signal, resource_metadata_changed_signal;
};




namespace
{


inline playlist::resource_event_signal_t & get_resource_added_signal(playlist &playlist_)
{
	return playlist_.get_resource_added_signal();
}


inline playlist::resource_event_signal_t & get_resource_removed_signal(playlist &playlist_)
{
	return playlist_.get_resource_removed_signal();
}


inline playlist::resource_event_signal_t & get_resource_metadata_changed_signal(playlist &playlist_)
{
	return playlist_.get_resource_metadata_changed_signal();
}


inline metadata_optional_t get_metadata_for(playlist const &playlist_, uri const &uri_)
{
	return playlist_.get_metadata_for(uri_);
}


inline uri_optional_t get_preceding_uri(playlist const &playlist_, uri const &uri_)
{
	return playlist_.get_preceding_uri(uri_);
}


inline uri_optional_t get_succeeding_uri(playlist const &playlist_, uri const &uri_)
{
	return playlist_.get_succeeding_uri(uri_);
}


inline void mark_backend_resource_incompatibility(playlist &playlist_, uri const &uri_, std::string const &backend_type)
{
	playlist_.mark_backend_resource_incompatibility(uri_, backend_type);
}


}


}


#endif

