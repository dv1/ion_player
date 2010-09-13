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
#include <boost/range/iterator_range.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/shared_ptr.hpp>

#include <json/value.h>

#include <ion/uri.hpp>
#include <ion/playlists_traits.hpp>
#include <ion/persistent_traits.hpp>


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
		typedef playlist_t const * result_type;

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




	void add_playlist(playlist_ptr_t playlist_ptr)
	{
		assert(playlist_ptr);
		playlists_.push_back(playlist_ptr);
		playlist_added_signal(*playlist_ptr);
	}


	void remove_playlist(playlist_t const &playlist_to_be_removed)
	{
		ordered_t &ordered = playlists_.template get < ordered_tag > ();
		typename ordered_t::iterator iter = ordered.find(&playlist_to_be_removed);
		playlist_removed_signal(*(*iter));
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



template < typename Playlist >
struct playlists_traits < playlists < Playlist > >
{
	typedef playlists < Playlist > playlists_t;
	typedef typename playlists_t::playlist_t playlist_t;
	typedef typename playlists_t::playlist_ptr_t playlist_ptr_t;
	typedef typename playlists_t::playlist_event_signal_t playlist_event_signal_t;
	typedef typename playlists_t::active_playlist_changed_signal_t active_playlist_changed_signal_t;
	typedef boost::iterator_range < typename playlists_t::sequenced_t::const_iterator > const_playlist_range_t;
};


template < typename Playlist >
struct persistent_traits < playlists < Playlist > >
{
	typedef Json::Value container_t;
};



namespace
{


template < typename Playlist >
inline void add_playlist(playlists < Playlist > &playlists_, typename playlists < Playlist > ::playlist_ptr_t const &playlist_ptr)
{
	playlists_.add_playlist(playlist_ptr);
}


template < typename Playlist >
inline void remove_playlist(playlists < Playlist > &playlists_, typename playlists < Playlist > ::playlist_t const &playlist_to_be_removed)
{
	playlists_.remove_playlist(playlist_to_be_removed);
}


template < typename Playlist >
inline typename playlists < Playlist > ::playlist_event_signal_t & get_playlist_added_signal(playlists < Playlist > &playlists_)
{
	return playlists_.get_playlist_added_signal();
}


template < typename Playlist >
inline typename playlists < Playlist > ::playlist_event_signal_t & get_playlist_removed_signal(playlists < Playlist > &playlists_)
{
	return playlists_.get_playlist_removed_signal();
}


template < typename Playlist >
inline typename playlists < Playlist > ::active_playlist_changed_signal_t & get_active_playlist_changed_signal(playlists < Playlist > &playlists_)
{
	return playlists_.get_active_playlist_changed_signal();
}


template < typename Playlist >
inline typename playlists < Playlist > ::playlist_t * get_active_playlist(playlists < Playlist > const &playlists_)
{
	return playlists_.get_active_playlist();
}


template < typename Playlist >
inline void set_active_playlist(playlists < Playlist > &playlists_, typename playlists < Playlist > ::playlist_t * new_active_playlist)
{
	playlists_.set_active_playlist(new_active_playlist);
}


template < typename Playlist >
inline typename playlists_traits < playlists < Playlist > > ::const_playlist_range_t get_playlists(playlists < Playlist > const &playlists_)
{
	typename playlists < Playlist > ::sequenced_t const & sequenced_ = playlists_.get_playlists();
	return typename playlists_traits < playlists < Playlist > > ::const_playlist_range_t(sequenced_.begin(), sequenced_.end());
}



template < typename Playlist, typename CreatePlaylistFunc >
inline void load_from(playlists < Playlist > &playlists_, Json::Value const &in_value, CreatePlaylistFunc const &create_playlist_func)
{
	// TODO: clear out existing playlists

	for (unsigned int index = 0; index < in_value.size(); ++index)
	{
		Json::Value json_entry = in_value[index];
		typename playlists < Playlist > ::playlist_ptr_t new_playlist(create_playlist_func());
		load_from(*new_playlist, json_entry);
		playlists_.add_playlist(new_playlist);
	}
}


template < typename Playlist >
inline void save_to(playlists < Playlist > const &playlists_, Json::Value &out_value)
{
	out_value = Json::Value(Json::arrayValue);

	BOOST_FOREACH(typename playlists < Playlist > ::playlist_ptr_t const &entry_, playlists_.get_playlists())
	{
		Json::Value playlist_value(Json::objectValue);
		save_to(*entry_, playlist_value);
		out_value.append(playlist_value);
	}
}


}


}


#endif

