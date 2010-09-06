#ifndef ION_FILTER_PLAYLIST_HPP
#define ION_FILTER_PLAYLIST_HPP

#include <boost/function.hpp>
#include "playlist.hpp"


namespace ion
{


template < typename Playlists >
class filter_playlist:
	public playlist
{
public:
	typedef boost::function < bool(playlist::entry_t const &entry_) > matching_function_t;
	typedef Playlists playlists_t;



	explicit filter_playlist(playlists_t &playlists_, matching_function_t const &matching_function = matching_function_t()):
		playlists_(playlists_),
		matching_function(matching_function)
	{
	}


	void set_matching_function(matching_function_t const &new_matching_function)
	{
		matching_function = new_matching_function;
		update_entries();
	}


	virtual metadata_optional_t get_metadata_for(uri const &uri_) const
	{
	}


	virtual uri_optional_t get_preceding_uri(uri const &uri_) const
	{
	}


	virtual uri_optional_t get_succeeding_uri(uri const &uri_) const
	{
	}


	virtual void mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type)
	{
	}


	virtual num_entries_t get_num_entries() const
	{
	}


	virtual entry_t const * get_entry(index_t const index) const
	{
	}


	virtual entry_t const * get_entry(uri const &uri_) const
	{
	}


	virtual index_optional_t get_entry_index(uri const &uri_) const
	{
	}


	virtual bool is_mutable() const
	{
		return false;
	}


	virtual void add_entry(entry_t const &entry_, bool const emit_signal)
	{
	}


	virtual void remove_entry(entry_t const &entry_, bool const emit_signal)
	{
	}


	virtual void remove_entry(uri const &uri_, bool const emit_signal)
	{
	}


	virtual void remove_entries(uri_set_t const &uris, bool const emit_signal)
	{
	}


	virtual void set_resource_metadata(uri const &uri_, metadata_t const & new_metadata)
	{
	}


	virtual void clear_entries(bool const emit_signal)
	{
	}


	virtual void load_from(Json::Value const &in_value)
	{
	}


	virtual void save_to(Json::Value &out_value) const
	{
	}


protected:
	void playlist_added(playlist &playlist)
	{
	}


	void playlist_removed(playlist &playlist)
	{
	}


	void update_entries()
	{
	}



	playlists_t &playlists_;
	matching_function_t matching_function;
};


}


#endif

