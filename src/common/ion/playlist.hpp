#ifndef ION_PLAYLIST_HPP
#define ION_PLAYLIST_HPP

#include <stdint.h>
#include <string>

#include <boost/signals2/signal.hpp>
#include <boost/function.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/optional.hpp>

#include <ion/playlist_traits.hpp>
#include <ion/metadata.hpp>
#include <ion/uri.hpp>


namespace ion
{


class playlist
{
public:
	typedef boost::signals2::signal < void(bool const before) > all_resources_changed_signal_t;
	typedef boost::signals2::signal < void(uri_set_t const &uris, bool const before) > resource_event_signal_t;
	typedef boost::signals2::signal < void(std::string const &new_name) > playlist_renamed_signal_t;
	typedef boost::fusion::vector2 < ion::uri, metadata_t > entry_t;
	typedef uint64_t index_t;
	typedef uint64_t num_entries_t;
	typedef boost::optional < index_t > index_optional_t;


	virtual ~playlist() {}


	inline resource_event_signal_t & get_resource_added_signal() { return resource_added_signal; }
	inline resource_event_signal_t & get_resource_removed_signal() { return resource_removed_signal; }
	inline resource_event_signal_t & get_resource_metadata_changed_signal() { return resource_metadata_changed_signal; }
	inline all_resources_changed_signal_t & get_all_resources_changed_signal() { return all_resources_changed_signal; }

	inline playlist_renamed_signal_t & get_playlist_renamed_signal() { return playlist_renamed_signal; }

	virtual metadata_optional_t get_metadata_for(uri const &uri_) const = 0;
	virtual uri_optional_t get_preceding_uri(uri const &uri_) const = 0;
	virtual uri_optional_t get_succeeding_uri(uri const &uri_) const = 0;
	virtual void mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type) = 0;

	virtual num_entries_t get_num_entries() const = 0;
	virtual entry_t const * get_entry(index_t const index) const = 0;
	virtual entry_t const * get_entry(uri const &uri_) const = 0;
	virtual index_optional_t get_entry_index(uri const &uri_) const = 0;

	std::string get_name() const { return name; }

	void set_name(std::string const &new_name)
	{
		name = new_name;
		playlist_renamed_signal(name);
	}

	virtual void add_entry(entry_t const &entry_, bool const emit_signal) = 0;
	virtual void remove_entry(entry_t const &entry_, bool const emit_signal) = 0;
	virtual void remove_entry(uri const &uri_, bool const emit_signal) = 0;
	virtual void remove_entries(uri_set_t const &uris, bool const emit_signal) = 0;
	virtual void set_resource_metadata(uri const &uri_, metadata_t const & new_metadata) = 0;

	virtual void clear_entries(bool const emit_signal) = 0;


protected:
	resource_event_signal_t resource_added_signal, resource_removed_signal, resource_metadata_changed_signal;
	all_resources_changed_signal_t all_resources_changed_signal;
	playlist_renamed_signal_t playlist_renamed_signal;
	std::string name;
};




template < >
struct playlist_traits < playlist >
{
	typedef playlist::resource_event_signal_t resource_event_signal_t;
	typedef playlist::playlist_renamed_signal_t playlist_renamed_signal_t;
	typedef playlist::all_resources_changed_signal_t all_resources_changed_signal_t;
	
	typedef playlist::entry_t entry_t;
	typedef playlist::num_entries_t num_entries_t;
	typedef playlist::index_t index_t;
	typedef playlist::index_optional_t index_optional_t;
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


inline playlist::all_resources_changed_signal_t & get_all_resources_changed_signal(playlist &playlist_)
{
	return playlist_.get_all_resources_changed_signal();
}


inline playlist::playlist_renamed_signal_t & get_playlist_renamed_signal(playlist &playlist_)
{
	return playlist_.get_playlist_renamed_signal();
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


inline std::string get_name(playlist const &playlist_)
{
	return playlist_.get_name();
}


inline void set_name(playlist &playlist_, std::string const &new_name)
{
	playlist_.set_name(new_name);
}




inline playlist::num_entries_t get_num_entries(playlist const &playlist_)
{
	return playlist_.get_num_entries();
}


inline playlist::entry_t const * get_entry(playlist const &playlist_, playlist::index_t const &index)
{
	return playlist_.get_entry(index);
}


inline playlist::entry_t const * get_entry(playlist const &playlist_, uri const &uri_)
{
	return playlist_.get_entry(uri_);
}


inline playlist::index_optional_t get_entry_index(playlist const &playlist_, uri const &uri_)
{
	return playlist_.get_entry_index(uri_);
}




inline void add_entry(playlist &playlist_, playlist::entry_t const &entry_, bool const emit_signal)
{
	playlist_.add_entry(entry_, emit_signal);
}


inline void remove_entry(playlist &playlist_, playlist::entry_t const &entry_, bool const emit_signal)
{
	playlist_.remove_entry(entry_, emit_signal);
}


inline void remove_entry(playlist &playlist_, uri const &uri_, bool const emit_signal)
{
	playlist_.remove_entry(uri_, emit_signal);
}


inline void remove_entries(playlist &playlist_, uri_set_t const &uris, bool const emit_signal)
{
	playlist_.remove_entries(uris, emit_signal);
}


inline void set_resource_metadata(playlist &playlist_, uri const &uri_, metadata_t const &new_metadata)
{
	playlist_.set_resource_metadata(uri_, new_metadata);
}


}


}


#endif

