#ifndef ION_PLAYLISTS_HPP
#define ION_PLAYLISTS_HPP

#include <memory>
#include <string>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/shared_ptr.hpp>

#include <ion/uri.hpp>


namespace ion
{


template < typename Playlist >
class playlists
{
public:
	typedef Playlist playlist_t;
	typedef boost::shared_ptr < playlist_t > playlist_ptr_t;
	typedef boost::tuple < std::string, playlist_ptr_t > playlist_entry_t;
	typedef boost::signals2::signal < void(playlist_entry_t const &playlist_entry) > playlist_entry_event_signal_t;
	typedef boost::signals2::signal < void(playlist_entry_t const *playlist_entry) > active_entry_changed_signal_t;
	typedef boost::signals2::signal < void(playlist_entry_t const &playlist_entry, std::string const &new_name) > playlist_entry_renamed_signal_t;


	struct sequence_tag {};
	struct ordered_tag {};

	typedef boost::multi_index::multi_index_container <
		playlist_entry_t,
		boost::multi_index::indexed_by <
			boost::multi_index::sequenced < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_non_unique < boost::multi_index::tag < ordered_tag >, boost::multi_index::identity < playlist_entry_t > >
		>
	> playlist_entries_t;

	typedef typename playlist_entries_t::template index < sequence_tag > ::type sequenced_entries_t;
	typedef typename playlist_entries_t::template index < ordered_tag > ::type ordered_entries_t;




	playlists():
		active_entry(0)
	{
	}




	void add_entry(std::string const &playlist_name, playlist_ptr_t playlist_ptr)
	{
		playlist_entry_t new_entry(playlist_name, playlist_ptr);
		playlist_entries.push_back(new_entry).first;
		playlist_entry_added_signal(new_entry);
	}


	void rename_entry(playlist_entry_t const &entry_to_be_renamed, std::string const &new_name)
	{
		ordered_entries_t &ordered_entries = playlist_entries.template get < ordered_tag > ();
		typename ordered_entries_t::iterator iter = ordered_entries.find(entry_to_be_renamed);
		boost::get < 0 > (*iter) = new_name;
		playlist_entry_renamed_signal(*iter, new_name);
	}


	void remove_entry(playlist_entry_t const &entry_to_be_removed)
	{
		ordered_entries_t &ordered_entries = playlist_entries.template get < ordered_tag > ();
		typename ordered_entries_t::iterator iter = ordered_entries.find(entry_to_be_removed);
		playlist_entry_removed_signal(*iter);
		ordered_entries.erase(iter);
	}


	playlist_entry_event_signal_t & get_playlist_entry_added_signal() { return playlist_entry_added_signal; }
	playlist_entry_event_signal_t & get_playlist_entry_removed_signal() { return playlist_entry_removed_signal; }
	playlist_entry_renamed_signal_t & get_playlist_entry_renamed_signal() { return playlist_entry_renamed_signal; }
	active_entry_changed_signal_t & get_active_entry_changed_signal() { return active_entry_changed_signal; }


	Playlist const * get_active_playlist() const
	{
		return (active_entry == 0) ? 0 : boost::get < 1 > (*active_entry).get();
	}


	playlist_entry_t const * get_active_entry() const
	{
		return active_entry;
	}


	void set_active_entry(playlist_entry_t const *new_active_entry)
	{
		active_entry = new_active_entry;
		active_entry_changed_signal(active_entry);
	}


	sequenced_entries_t const & get_entries() const
	{
		return playlist_entries.template get < sequence_tag > ();
	}

	
protected:
	playlist_entry_event_signal_t
		playlist_entry_added_signal,
		playlist_entry_removed_signal;
	active_entry_changed_signal_t
		active_entry_changed_signal;
	playlist_entry_renamed_signal_t playlist_entry_renamed_signal;
	playlist_entry_t const *active_entry;
	playlist_entries_t playlist_entries;
};


}


#endif

