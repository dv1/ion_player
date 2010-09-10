#ifndef ION_FILTER_PLAYLIST_HPP
#define ION_FILTER_PLAYLIST_HPP

#include <assert.h>
#include <map>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>

#include "playlist.hpp"


namespace ion
{


template < typename Playlists >
class filter_playlist:
	public playlist
{
public:
	typedef filter_playlist < Playlists > self_t;
	typedef boost::function < bool(playlist::entry_t const &entry_) > matching_function_t;
	typedef Playlists playlists_t;

	struct proxy_entry
	{
		uri uri_;
		playlist *playlist_;

		proxy_entry():
			playlist_(0)
		{
		}

		proxy_entry(uri const &uri_, playlist *playlist_):
			uri_(uri_),
			playlist_(playlist_)
		{
		}
	};

	typedef boost::optional < proxy_entry > proxy_entry_optional_t;


	struct sequence_tag {};
	struct uri_tag {};
	struct playlist_tag {};

	typedef boost::multi_index::multi_index_container <
		proxy_entry,
		boost::multi_index::indexed_by <
			boost::multi_index::random_access < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_unique <
				boost::multi_index::tag < uri_tag >,
				boost::multi_index::member < proxy_entry, uri, &proxy_entry::uri_ >
			>,
			boost::multi_index::ordered_non_unique <
				boost::multi_index::tag < playlist_tag >,
				boost::multi_index::member < proxy_entry, playlist*, &proxy_entry::playlist_ >
			>
		>
	> proxy_entries_t;

	typedef typename proxy_entries_t::template index < sequence_tag > ::type proxy_entry_sequence_t;
	typedef typename proxy_entries_t::template index < uri_tag > ::type proxy_entries_by_uri_t;
	typedef typename proxy_entries_t::template index < playlist_tag > ::type proxy_entries_by_playlist_t;



	explicit filter_playlist(playlists_t &playlists_, matching_function_t const &matching_function = matching_function_t()):
		playlists_(playlists_),
		matching_function(matching_function)
	{
		playlist_added_connection = get_playlist_added_signal(playlists_).connect(boost::phoenix::bind(&self_t::playlist_added, this, boost::phoenix::arg_names::arg1));
		playlist_removed_connection = get_playlist_removed_signal(playlists_).connect(boost::phoenix::bind(&self_t::playlist_removed, this, boost::phoenix::arg_names::arg1));

		update_entries_impl();

		BOOST_FOREACH(typename playlists_t::playlist_ptr_t playlist_, get_playlists(playlists_))
		{
			add_playlist_signal_connections(*playlist_);
		}
	}


	~filter_playlist()
	{
		playlist_added_connection.disconnect();
		playlist_removed_connection.disconnect();
	}


	void set_matching_function(matching_function_t const &new_matching_function)
	{
		matching_function = new_matching_function;
		update_entries();
	}


	virtual metadata_optional_t get_metadata_for(uri const &uri_) const
	{
		proxy_entries_by_uri_t const &proxy_entries_by_uri = proxy_entries.template get < uri_tag > ();
		typename proxy_entries_by_uri_t::const_iterator iter = proxy_entries_by_uri.find(uri_);
		if (iter != proxy_entries_by_uri.end())
			return ion::get_metadata_for(*(iter->playlist_), uri_);
		else
			return boost::none;
	}


	virtual uri_optional_t get_preceding_uri(uri const &uri_) const
	{
		proxy_entry_sequence_t const &proxy_entry_sequence = proxy_entries.template get < sequence_tag > ();
		typename proxy_entry_sequence_t::const_iterator seq_iter = proxy_entries.template project < sequence_tag > (get_uri_iterator_for(uri_));

		if (seq_iter == proxy_entry_sequence.end())
			return boost::none;
		else
		{
			if (seq_iter == proxy_entry_sequence.begin())
				return boost::none;
			else
			{
				--seq_iter;
				return seq_iter->uri_;
			}
		}
	}


	virtual uri_optional_t get_succeeding_uri(uri const &uri_) const
	{
		proxy_entry_sequence_t const &proxy_entry_sequence = proxy_entries.template get < sequence_tag > ();
		typename proxy_entry_sequence_t::const_iterator seq_iter = proxy_entries.template project < sequence_tag > (get_uri_iterator_for(uri_));

		if (seq_iter == proxy_entry_sequence.end())
			return boost::none;
		else
		{
			++seq_iter;
			if (seq_iter == proxy_entry_sequence.end())
				return boost::none;
			else
				return seq_iter->uri_;
		}
	}


	virtual void mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type)
	{
		proxy_entries_by_uri_t &proxy_entries_by_uri = proxy_entries.template get < uri_tag > ();
		typename proxy_entries_by_uri_t::const_iterator iter = proxy_entries_by_uri.find(uri_);
		if (iter != proxy_entries_by_uri.end())
			ion::mark_backend_resource_incompatibility(*(iter->playlist_), uri_, backend_type);
	}


	virtual num_entries_t get_num_entries() const
	{
		return proxy_entries.size();
	}


	virtual entry_t const * get_entry(index_t const index) const
	{
		if (index >= get_num_entries())
			return 0;

		proxy_entry_sequence_t const &proxy_entry_sequence = proxy_entries.template get < sequence_tag > ();
		typename proxy_entry_sequence_t::const_iterator iter = proxy_entry_sequence.begin();
		iter = iter + index;

		return iter->playlist_->get_entry(iter->uri_);
	}


	virtual entry_t const * get_entry(uri const &uri_) const
	{
		proxy_entries_by_uri_t const &proxy_entries_by_uri = proxy_entries.template get < uri_tag > ();
		typename proxy_entries_by_uri_t::const_iterator iter = proxy_entries_by_uri.find(uri_);
		if (iter == proxy_entries_by_uri.end())
			return 0;
		else
			return iter->playlist_->get_entry(iter->uri_);
	}


	virtual index_optional_t get_entry_index(uri const &uri_) const
	{
		proxy_entry_sequence_t const &proxy_entry_sequence = proxy_entries.template get < sequence_tag > ();
		typename proxy_entry_sequence_t::const_iterator seq_iter = proxy_entries.template project < sequence_tag > (get_uri_iterator_for(uri_));

		if (seq_iter == proxy_entry_sequence.end())
			return boost::none;

		return index_t(seq_iter - proxy_entry_sequence.begin());
	}


	proxy_entry_optional_t get_proxy_entry(index_t const index) const
	{
		if (index >= get_num_entries())
			return boost::none;

		proxy_entry_sequence_t const &proxy_entry_sequence = proxy_entries.template get < sequence_tag > ();
		typename proxy_entry_sequence_t::const_iterator iter = proxy_entry_sequence.begin();
		iter = iter + index;

		return *iter;
	}


	proxy_entries_t const & get_proxy_entries() const { return proxy_entries; }


	virtual bool is_mutable() const
	{
		return false;
	}


	virtual void add_entry(entry_t const &, bool const) {}
	virtual void remove_entry(entry_t const &, bool const) {}
	virtual void remove_entry(uri const &, bool const) {}
	virtual void remove_entries(uri_set_t const &, bool const) {}
	virtual void set_resource_metadata(uri const &, metadata_t const &) {}
	virtual void clear_entries(bool const) {}
	virtual void load_from(Json::Value const &) {}
	virtual void save_to(Json::Value &) const {}


	void update_entries()
	{
		all_resources_changed_signal(true);
		update_entries_impl();
		all_resources_changed_signal(false);
	}


protected:
	enum connection_type
	{
		added_connection = 0,
		removed_connection = 1,
		all_changed_connection = 2
	};

	typedef typename boost::array < boost::signals2::connection, 3 > playlist_connections_t;
	typedef boost::shared_ptr < playlist_connections_t > playlist_connections_ptr_t;
	typedef std::map < playlist*, playlist_connections_ptr_t > playlist_connections_map_t;




	void update_entries_impl()
	{
		proxy_entries.clear();

		BOOST_FOREACH(typename playlists_t::playlist_ptr_t playlist_, get_playlists(playlists_))
		{
			add_other_playlist(playlist_.get(), false);
		}
	}


	void get_playlist_uriset(playlist const &playlist_, uri_set_t &uris) const
	{
		for (index_t index_ = 0; index_ < ion::get_num_entries(playlist_); ++index_)
		{
			entry_t const *entry_ = ion::get_entry(playlist_, index_);
			if (entry_ != 0)
				uris.insert(boost::fusion::at_c < 0 > (*entry_));
		}
	}


	void add_other_playlist(playlist *playlist_, bool const emit_signal)
	{
		assert(playlist_ != 0);

		if (static_cast < void* > (playlist_) == static_cast < void* > (this))
			return;

		uri_set_t uris;

		if (emit_signal)
		{
			get_playlist_uriset(*playlist_, uris);
			resource_added_signal(uris, true);
		}

		for (index_t index_ = 0; index_ < ion::get_num_entries(*playlist_); ++index_)
			add_other_playlist_entry(playlist_, ion::get_entry(*playlist_, index_));

		if (emit_signal)
			resource_added_signal(uris, false);
	}


	void add_other_playlist_entry(playlist *playlist_, entry_t const *entry_)
	{
		assert(playlist_ != 0);

		if (entry_ == 0)
			return;

		bool matches = true;
		if (matching_function)
			matches = matching_function(*entry_);

		if (matches)
		{
			proxy_entry_sequence_t &proxy_entry_sequence = proxy_entries.template get < sequence_tag > ();

			typename proxy_entry_sequence_t::reverse_iterator riter = proxy_entry_sequence.rbegin();
			for (; riter != proxy_entry_sequence.rend(); ++riter)
			{
				if (riter->playlist_ == playlist_)
					break;
			}

			proxy_entry new_proxy_entry(
				boost::fusion::at_c < 0 > (*entry_),
				playlist_
			);

			if (riter == proxy_entry_sequence.rend())
				proxy_entry_sequence.push_back(new_proxy_entry);
			else
				proxy_entry_sequence.insert(riter.base(), new_proxy_entry);
		}
	}


	void remove_other_playlist_entry(playlist *playlist_, uri const &uri_)
	{
		assert(playlist_ != 0);

		proxy_entries_by_uri_t &proxy_entries_by_uri = proxy_entries.template get < uri_tag > ();
		typename proxy_entries_by_uri_t::iterator iter = proxy_entries_by_uri.find(uri_);
		if (iter != proxy_entries_by_uri.end())
			proxy_entries_by_uri.erase(iter);
	}


	void remove_other_playlist(playlist *playlist_, bool const emit_signal)
	{
		assert(playlist_ != 0);

		uri_set_t uris;
		if (emit_signal)
		{
			get_playlist_uriset(*playlist_, uris);
			resource_removed_signal(uris, true);
		}

		proxy_entries_by_playlist_t &proxy_entries_by_playlist = proxy_entries.template get < playlist_tag > ();
		typename proxy_entries_by_playlist_t::iterator begin = proxy_entries_by_playlist.lower_bound(playlist_);
		typename proxy_entries_by_playlist_t::iterator end = proxy_entries_by_playlist.upper_bound(playlist_);

		proxy_entries_by_playlist.erase(begin, end);

		if (emit_signal)
			resource_removed_signal(uris, false);
	}


	typename proxy_entries_by_uri_t::const_iterator get_uri_iterator_for(uri const &uri_) const
	{
		proxy_entries_by_uri_t const &proxy_entries_by_uri = proxy_entries.template get < uri_tag > ();
		typename proxy_entries_by_uri_t::const_iterator uri_tag_iter = proxy_entries_by_uri.find(uri_);

		return uri_tag_iter;
	}


	typename proxy_entries_by_uri_t::iterator get_uri_iterator_for(uri const &uri_)
	{
		proxy_entries_by_uri_t &proxy_entries_by_uri = proxy_entries.template get < uri_tag > ();
		typename proxy_entries_by_uri_t::iterator uri_tag_iter = proxy_entries_by_uri.find(uri_);

		return uri_tag_iter;
	}



	void add_playlist_signal_connections(playlist &playlist_)
	{
		playlist_connections_ptr_t connections(new playlist_connections_t);
		(*connections)[added_connection] = ion::get_resource_added_signal(playlist_).connect(boost::phoenix::bind(&self_t::resources_added, this, boost::ref(playlist_), boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));
		(*connections)[removed_connection] = ion::get_resource_removed_signal(playlist_).connect(boost::phoenix::bind(&self_t::resources_removed, this, boost::ref(playlist_), boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));
		(*connections)[all_changed_connection] = ion::get_all_resources_changed_signal(playlist_).connect(boost::phoenix::bind(&self_t::all_resources_changed, this, boost::ref(playlist_), boost::phoenix::arg_names::arg1));
		playlist_connections_map[&playlist_] = connections;
	}


	void playlist_added(playlist &playlist_)
	{
		add_other_playlist(&playlist_, true);
		add_playlist_signal_connections(playlist_);
	}


	void playlist_removed(playlist &playlist_)
	{
		typename playlist_connections_map_t::iterator connection_map_iter = playlist_connections_map.find(&playlist_);
		if (connection_map_iter != playlist_connections_map.end())
		{
			BOOST_FOREACH(boost::signals2::connection &connection_, *(connection_map_iter->second))
			{
				connection_.disconnect();
			}
			playlist_connections_map.erase(connection_map_iter);
		}

		remove_other_playlist(&playlist_, true);
	}


	void all_resources_changed(playlist &playlist_, bool const before)
	{
		if (ion::get_num_entries(playlist_) > 0)
		{
			if (before)
				remove_other_playlist(&playlist_, true);
			else
				add_other_playlist(&playlist_, true);
		}
	}


	void resources_added(playlist &playlist_, uri_set_t const &uris, bool const before)
	{
		if (!before)
		{
			BOOST_FOREACH(uri const &uri_, uris)
			{
				add_other_playlist_entry(&playlist_, ion::get_entry(playlist_, uri_));
			}
		}

		resource_added_signal(uris, before);
	}


	void resources_removed(playlist &playlist_, uri_set_t const &uris, bool const before)
	{
		if (!before)
		{
			BOOST_FOREACH(uri const &uri_, uris)
			{
				remove_other_playlist_entry(&playlist_, uri_);
			}
		}

		resource_removed_signal(uris, before);
	}



	playlists_t &playlists_;
	matching_function_t matching_function;
	proxy_entries_t proxy_entries;

	playlist_connections_map_t playlist_connections_map;
	typename boost::signals2::connection
		playlist_added_connection,
		playlist_removed_connection;
};


}


#endif

