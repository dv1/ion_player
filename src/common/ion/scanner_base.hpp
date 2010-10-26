/****************************************************************************

Copyright (c) 2010 Carlos Rafael Giani

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

****************************************************************************/


#ifndef ION_SCANNER_BASE_HPP
#define ION_SCANNER_BASE_HPP

#include <algorithm>
#include <deque>
#include <iostream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/member.hpp>
#include <ion/command_line_tools.hpp>
#include <ion/metadata.hpp>
#include <ion/uri.hpp>
#include <ion/playlists_traits.hpp>


namespace ion
{


/*
functions the derived class must implement:

void send_to_backend(std::string const &command_line)
void restart_watchdog_timer()
void stop_watchdog_timer()
void restart_backend()
void adding_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before)
void removing_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before)
void scanning_in_progress(bool const state)
void resource_successfully_scanned(ion::uri const &uri_, playlist_t &playlist_)
void unrecognized_resource(ion::uri const &uri_, playlist_t &playlist_)
void resource_corrupted(ion::uri const &uri_, playlist_t &playlist_)
void scanning_failed(ion::uri const &uri_, playlist_t &playlist_)


functions from this base class the derived class can/should use:
	void issue_scan_request(ion::uri const &uri_, playlist_t &playlist_)
	void backend_crashed()
	void cancel_scan()
	void parse_backend_event(std::string const &line)
*/


template < typename Derived, typename Playlists >
class scanner_base:
	private boost::noncopyable
{
public:
	typedef scanner_base < Derived, Playlists > self_t;
	typedef Derived derived_t;
	typedef Playlists playlists_t;
	typedef typename playlists_traits < Playlists > ::playlist_t playlist_t;

	struct sequence_tag {};
	struct uri_tag {};
	struct playlist_tag {};

	struct entry
	{
		ion::uri uri_;
		playlist_t *playlist_;

		entry(): playlist_(0) {}
		entry(ion::uri const &uri_, playlist_t *playlist_):
			uri_(uri_),
			playlist_(playlist_)
		{
		}
	};

	typedef boost::multi_index::multi_index_container <
		entry,
		boost::multi_index::indexed_by <
			boost::multi_index::random_access < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_unique <
				boost::multi_index::tag < uri_tag >,
				boost::multi_index::member < entry, ion::uri, &entry::uri_ >
			>,
			boost::multi_index::ordered_non_unique <
				boost::multi_index::tag < playlist_tag >,
				boost::multi_index::member < entry, playlist_t*, &entry::playlist_ >
			>
		>
	> queue_t;

	typedef typename queue_t::template index < sequence_tag > ::type  queue_sequence_t;
	typedef typename queue_t::template index < uri_tag > ::type       queue_by_uri_t;
	typedef typename queue_t::template index < playlist_tag > ::type  queue_by_playlist_t;


	explicit scanner_base(playlists_t &playlists_):
		playlists_(playlists_)
	{
		playlist_removed_connection = playlists_.get_playlist_removed_signal().connect(boost::phoenix::bind(&self_t::playlist_removed, this, boost::phoenix::arg_names::arg1));
	}


	~scanner_base()
	{
		if (!queue.empty())
			cancel_scan();
		playlist_removed_connection.disconnect();
	}


	void issue_scan_request(ion::uri const &uri_, playlist_t &playlist_)
	{
		bool queue_was_empty = queue.empty();
		get_derived().adding_queue_entry(uri_, playlist_, true);
		queue.push_back(entry(uri_, &playlist_));
		get_derived().adding_queue_entry(uri_, playlist_, false);
		if (queue_was_empty)
		{
			get_derived().scanning_in_progress(true);
			get_derived().restart_watchdog_timer();
			request_next_metadata();
		}
	}


	queue_t const & get_queue() const { return queue; }


protected:
	derived_t& get_derived()
	{
		return *(static_cast < derived_t* > (this));
	}


	void playlist_removed(playlist_t &playlist_)
	{
		// TODO: call removing_queue_entry() for each entry before and after removing them
		queue_by_playlist_t &queue_by_playlist = queue.template get < playlist_tag > ();
		queue_by_playlist.erase(queue_by_playlist.lower_bound(&playlist_), queue_by_playlist.upper_bound(&playlist_));

		if (queue.empty())
		{
			get_derived().stop_watchdog_timer();
			get_derived().scanning_in_progress(false);
		}
	}


	void request_next_metadata()
	{
		if (queue.empty())
			return;

		entry front_entry = queue.front();
		get_derived().send_to_backend(std::string("get_metadata \"") + front_entry.uri_.get_full() + '"');
	}


	void cancel_scan()
	{
		queue.clear();
		get_derived().stop_watchdog_timer();
		get_derived().scanning_in_progress(false);
	}


	// called by the derived class when the backend dies unexpectedly
	void backend_crashed()
	{
		if (!queue.empty())
		{
			entry front_entry = queue.front();
			queue.pop_front();
			get_derived().scanning_failed(front_entry.uri_, *(front_entry.playlist_));
		}

		get_derived().restart_backend();

		if (!queue.empty())
		{
			get_derived().restart_watchdog_timer();
			request_next_metadata();
		}
		else
		{
			get_derived().stop_watchdog_timer();
			get_derived().scanning_in_progress(false);
		}
	}


	void parse_backend_event(std::string const &line)
	{
		std::string command;
		params_t params;
		split_command_line(line, command, params);

		queue_by_uri_t &queue_by_uri = queue.template get < uri_tag > ();
		typename queue_by_uri_t::iterator resource_by_uri_iter = queue_by_uri.end();

		if (params.size() > 0)
			resource_by_uri_iter = queue_by_uri.find(params[0]);

		if (resource_by_uri_iter != queue_by_uri.end())
		{
			uri uri_ = resource_by_uri_iter->uri_;
			playlist_t *playlist_ = resource_by_uri_iter->playlist_;

			bool playlist_exists = has_playlist(playlists_, *playlist_);

			if (playlist_exists)
			{
				if ((command == "metadata") && (params.size() >= 2))
				{
					metadata_optional_t new_metadata = parse_metadata(params[1]);

					if (new_metadata)
					{
						long num_sub_resources = get_metadata_value < long > (*new_metadata, "num_sub_resources", 0);
						long min_sub_resource_index = get_metadata_value < long > (*new_metadata, "min_sub_resource_index", 0);
						uri::options_t::const_iterator uri_resource_index_iter = uri_.get_options().find("sub_resource_index");
						bool has_resource_index = (uri_resource_index_iter != uri_.get_options().end());

						if ((num_sub_resources > 1) && !has_resource_index)
						{
							for (int i = 0; i < num_sub_resources; ++i)
							{
								unsigned int sub_resource_index = min_sub_resource_index + i;
								uri sub_uri = uri_;
								sub_uri.get_options()["sub_resource_index"] = boost::lexical_cast < std::string > (sub_resource_index);

								//queue_sequence_t &queue_sequence = queue.template get < sequence_tag > ();
								typename queue_sequence_t::iterator seq_iter = queue.template project < sequence_tag > (resource_by_uri_iter);
								queue.insert(seq_iter, entry(sub_uri, playlist_));
							}
						}
						else
						{
							if (has_metadata_value(*new_metadata, "title") && has_resource_index)
							{
								std::string title = get_metadata_value < std::string > (*new_metadata, "title", "");
								std::stringstream sstr;
								std::string resource_index_str = uri_resource_index_iter->second;

								try
								{
									// This compensates for the 0-starting indices
									int resource_index = boost::lexical_cast < int > (resource_index_str);
									resource_index_str = boost::lexical_cast < std::string > (resource_index + 1 - min_sub_resource_index);
								}
								catch (boost::bad_lexical_cast const &)
								{
								}

								sstr << title << " (" << resource_index_str << "/" << num_sub_resources << ")";
								set_metadata_value(*new_metadata, "title", sstr.str());
							}

							get_derived().resource_successfully_scanned(uri_, *playlist_, *new_metadata);
						}
					}
					else
						get_derived().unrecognized_resource(uri_, *playlist_);
				}
				else if (command == "resource_corrupted")
				{
					get_derived().resource_corrupted(uri_, *playlist_);
				}
				else if (command == "unrecognized_resource")
				{
					get_derived().unrecognized_resource(uri_, *playlist_);
				}
				/*else if (command == "error")
				{
					get_derived().resource_scan_error(uri_, *playlist_, (params.size() >= 2) ? params[1] : boost::none);
				}*/
			}
			else
				std::cerr << "uri " << uri_.get_full() << " playlist is from a playlist that does not exist\n";

			{
				if (playlist_exists)
					get_derived().removing_queue_entry(uri_, *playlist_, true);

				queue_by_uri.erase(resource_by_uri_iter);

				if (playlist_exists)
					get_derived().removing_queue_entry(uri_, *playlist_, false);
			}

			if (queue.empty())
			{
				get_derived().stop_watchdog_timer();
				get_derived().scanning_in_progress(false);
			}
			else
			{
				get_derived().restart_watchdog_timer();
				request_next_metadata();
			}
		}
	}


	queue_t queue;
	playlists_t &playlists_;
	boost::signals2::connection playlist_removed_connection;
};


}


#endif

