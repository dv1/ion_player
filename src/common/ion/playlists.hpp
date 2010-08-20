#ifndef ION_PLAYLISTS_HPP
#define ION_PLAYLISTS_HPP

#include <assert.h>
#include <memory>
#include <string>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
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
	typedef boost::signals2::signal < void(playlist_t &playlist_) > playlist_event_signal_t;
	typedef boost::signals2::signal < void(playlist_t *playlist_) > active_playlist_changed_signal_t;


protected:
	struct get_from_ptr
	{
		typedef playlist_t* result_type;

		result_type operator()(playlist_ptr_t ptr) const
		{
			return ptr.get();
		}
	};


public:
	struct sequence_tag {};
	struct ordered_tag {};

	typedef boost::multi_index::multi_index_container <
		playlist_ptr_t,
		boost::multi_index::indexed_by <
			boost::multi_index::sequenced < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_non_unique < boost::multi_index::tag < ordered_tag >, get_from_ptr >
		>
	> playlists_t;

	typedef typename playlists_t::template index < sequence_tag > ::type sequenced_t;
	typedef typename playlists_t::template index < ordered_tag > ::type ordered_t;




	playlists():
		active_playlist(0)
	{
	}




	void add_playlist(std::string const &playlist_name, playlist_ptr_t playlist_ptr)
	{
		assert(playlist_ptr);
		set_name(*playlist_ptr, playlist_name); // TODO: this is unnecessary - simply expect that the name has been set already - meaning that the playlist_name parameter is unnecessary as well
		playlists_.push_back(playlist_ptr);
		playlist_added_signal(*playlist_ptr);
	}


	void remove_playlist(playlist_t const &playlist_to_be_removed)
	{
		ordered_t &ordered = playlists_.template get < ordered_tag > ();
		typename ordered_t::iterator iter = ordered.find(playlist_to_be_removed);
		playlist_removed_signal(*iter);
		ordered.erase(iter);
	}


	playlist_event_signal_t & get_playlist_added_signal() { return playlist_added_signal; }
	playlist_event_signal_t & get_playlist_removed_signal() { return playlist_removed_signal; }
	active_playlist_changed_signal_t & get_active_playlist_changed_signal() { return active_playlist_changed_signal; }


	playlist_t * get_active_playlist() const
	{
		return active_playlist;
	}


	void set_active_playlist(playlist_t *new_active_playlist)
	{
		active_playlist = new_active_playlist;
		active_playlist_changed_signal(active_playlist);
	}


	sequenced_t const & get_playlists() const
	{
		return playlists_.template get < sequence_tag > ();
	}

	
protected:
	playlist_event_signal_t
		playlist_added_signal,
		playlist_removed_signal;
	active_playlist_changed_signal_t
		active_playlist_changed_signal;
	playlist_t *active_playlist;
	playlists_t playlists_;
};


}


#endif

