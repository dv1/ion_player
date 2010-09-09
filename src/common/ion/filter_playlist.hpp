#ifndef ION_FILTER_PLAYLIST_HPP
#define ION_FILTER_PLAYLIST_HPP

#include <boost/function.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

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


	virtual bool is_mutable() const
	{
		return false;
	}


	virtual void add_entry(entry_t const &, bool const)
	{
	}


	virtual void remove_entry(entry_t const &, bool const)
	{
	}


	virtual void remove_entry(uri const &, bool const)
	{
	}


	virtual void remove_entries(uri_set_t const &, bool const)
	{
	}


	virtual void set_resource_metadata(uri const &, metadata_t const &)
	{
	}


	virtual void clear_entries(bool const)
	{
	}


	virtual void load_from(Json::Value const &)
	{
	}


	virtual void save_to(Json::Value &) const
	{
	}


	void update_entries()
	{
		proxy_entries.clear();

		BOOST_FOREACH(typename playlists_t::playlist_ptr_t playlist_, get_playlists(playlists_))
		{
			if (static_cast < void* > (playlist_.get()) == static_cast < void* > (this))
				continue;

			for (index_t index_ = 0; index_ < ion::get_num_entries(*playlist_); ++index_)
			{
				entry_t const *entry_ = ion::get_entry(*playlist_, index_);
				if (entry_ == 0)
					continue;

				bool matches = true;
				if (matching_function)
					matches = matching_function(*entry_);

				if (matches)
				{
					proxy_entries.push_back(
						proxy_entry(
							boost::fusion::at_c < 0 > (*entry_),
							playlist_.get()
						)
					);
				}
			}
		}
	}


protected:
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



	void playlist_added(playlist &playlist)
	{
	}


	void playlist_removed(playlist &playlist)
	{
	}



	playlists_t &playlists_;
	matching_function_t matching_function;
	proxy_entries_t proxy_entries;
};


}


#endif

