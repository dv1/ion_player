#ifndef ION_PLAYLISTS_HPP
#define ION_PLAYLISTS_HPP

#include <memory>
#include <string>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/fusion/sequence/instrinsic/at.hpp>
#include <boost/fusion/container/vector.hpp>
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
	typedef boost::fusion::vector2 < std::string, playlist_ptr_t > playlist_entry_t;
	typedef boost::signals2::signal < void(playlist_entry_t const &playlist_entry) > playlist_entry_event_signal_t;
	typedef boost::signals2::signal < void(playlist_entry_t const &playlist_entry, std::string const &new_name) > playlist_entry_renamed_signal_t;


	struct sequence_tag {};
	struct ordered_tag {};

	typedef boost::multi_index::multi_index_container <
		playlist_entry_t,
		boost::multi_index::indexed_by <
			boost::multi_index::random_access < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_unique < boost::multi_index::tag < ordered_tag > >
		>
	> playlist_entries_t;

	typedef typename playlist_entries_t::index < sequence_tag > ::type sequenced_entries_t;
	typedef typename playlist_entries_t::index < ordered_tag > ::type ordered_entries_t;




	playlist_entry& add_entry(std::string const &playlist_name, playlist_ptr_t playlist_ptr)
	{
		std::unique_ptr < playlists_entry_t > new_entry(new playlists_entry_t(playlist_name, playlist_ptr));
		playlist_entries.push_back(new_entry.get());
		playlists_entry &result = *new_entry;
		new_entry.release();
		playlist_entry_added_signal(*iter);
		return result;
	}


	void rename_entry(playlist_entry const &entry_to_be_renamed, std::string const &new_name)
	{
		ordered_entries_t &ordered_entries = playlist_entries.get < ordered_tag > ();
		ordered_entries_t::iterator iter = ordered_entries.find(entry_to_be_removed);
		boost::fusion::at_c < 0 > (*iter) = new_name;
		playlist_entry_renamed_signal(*iter, new_name);
	}


	void remove_entry(playlist_entry const &entry_to_be_removed)
	{
		ordered_entries_t &ordered_entries = playlist_entries.get < ordered_tag > ();
		ordered_entries_t::iterator iter = ordered_entries.find(entry_to_be_removed);
		playlist_entry_removed_signal(*iter);
		ordered_entries.erase(iter);
	}


	playlist_entry_event_signal_t & get_playlist_entry_added_signal() { return playlist_entry_added_signal; }
	playlist_entry_event_signal_t & get_playlist_entry_removed_signal() { return playlist_entry_removed_signal; }
	playlist_entry_renamed_signal_t & get_playlist_entry_renamed_signal() { return playlist_entry_renamed_signal; }
	playlist_entry_event_signal_t & get_active_playlist_changed_signal() { return active_playlist_changed_signal; }


	typename playlist_entry_t * get_active_entry()
	{
		return active_entry;
	}


	void set_active_entry(playlist_entry &new_active_entry)
	{
		active_entry = &new_active_entry;
		active_playlist_changed_signal(*active_entry);
	}


	playlist_entries_t const & get_entries() const { return playlist_entries; }

	
protected:
	playlist():
		active_entry(0)
	{
	}


	playlist_entry_event_signal_t
		playlist_entry_added_signal,
		playlist_entry_removed_signal,
		active_playlist_changed_signal;
	playlist_entry_renamed_signal_t playlist_entry_renamed_signal;
	playlist_entry_t *active_entry;
	playlist_entries_t playlist_entries;
};


}


#endif

