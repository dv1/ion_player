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
#include <ion/command_line_tools.hpp>
#include <ion/metadata.hpp>
#include <ion/uri.hpp>
#include <ion/playlists_traits.hpp>


namespace ion
{


/*
Derived must contain:
- bool is_process_running() const
- void start_process(ion::uri const &uri_to_be_scanned)
- void add_entry_to_playlist(ion::uri const &new_uri, metadata_t const &new_metadata)
- void report_general_error(std::string const &error_string)
- void report_resource_error(std::string const &error_event, std::string const &uri)
*/


/*
TODO: add crash handling and multiple backend support.

Multiple backends:
Add get_backend_range() method for iterating through a list of backends. The range values are std::strings - filepaths to backend executables.

Crashes & scan failures:
1. Pick a backend for the URI. Typically, the first backend will be picked. A user preference can affect this step (for instance,
   when the user wants a specific backend to scan this resource).
2. Call the backend, asking for metadata for the given URI.
3. For each metadata event, store it in an internal metadata results list.
     For each unknown_resource event, store it in an internal URI scan failure list.
4. If the backend finished successfully, call add_entry_to_playlist() for each entry in the metadata results list. Then, clear that list.
     If the backend crashed, transfer all entries in the metadata results list to the URI scan failure list.
5. If the URI scan failure list is not empty, pick the next backend, and go to step 3.
6. If all backends have been tried, mark the URIs in the URI scan failure list as unreadable.
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


	explicit scanner_base(playlists_t &playlists_):
		current_playlist(0),
		playlists_(playlists_)
	{
		playlist_removed_connection = playlists_.get_playlist_removed_signal().connect(boost::phoenix::bind(&self_t::playlist_removed, this, boost::phoenix::arg_names::arg1));
	}


	~scanner_base()
	{
		playlist_removed_connection.disconnect();
	}


	void start_scan(playlist_t &playlist, ion::uri const &uri_to_be_scanned)
	{
		scan_queue.push_back(scan_entry_t(&playlist, uri_to_be_scanned));
		if (!static_cast < Derived* > (this)->is_process_running())
			init_scanning();
	}


	void cancel_scan()
	{
		scan_queue.clear();
	}


protected:
	typedef std::pair < playlist_t *, ion::uri > scan_entry_t;
	typedef std::deque < scan_entry_t > scan_queue_t;


	void init_scanning()
	{
		if (scan_queue.empty())
			return;

		scan_entry_t scan_entry = scan_queue.front();
		current_playlist = scan_entry.first;
		scan_queue.pop_front();

		static_cast < Derived* > (this)->start_process(scan_entry.second);
	}


	static bool is_from_playlist(playlist_t *playlist_, scan_entry_t const &scan_entry)
	{
		return scan_entry.first == playlist_;
	}


	void playlist_removed(playlist_t &playlist_)
	{
		typename scan_queue_t::iterator iter = std::remove_if(scan_queue.begin(), scan_queue.end(), boost::phoenix::bind(&is_from_playlist, &playlist_, boost::phoenix::arg_names::arg1));
		scan_queue.erase(iter, scan_queue.end());
	}


	void read_process_stdin_line(std::string const &line)
	{
		if (current_playlist == 0)
			return;

		if (!has_playlist(playlists_, *current_playlist))
			return;

		std::string event_command_name;
		params_t event_params;
		split_command_line(line, event_command_name, event_params);

		if ((event_command_name == "metadata") && (event_params.size() >= 2))
		{
			try
			{
				ion::uri uri_(event_params[0]);
				metadata_optional_t metadata_ = parse_metadata(event_params[1]);

				if (!metadata_)
				{
					metadata_ = empty_metadata();
					static_cast < Derived* > (this)->report_general_error(std::string("Metadata for URI \"") + uri_.get_full() + "\" is invalid!");
				}
				else
				{
					long num_sub_resources = 1;
					if (has_metadata_value(*metadata_, "num_sub_resources"))
					{
						num_sub_resources = get_metadata_value < long > (*metadata_, "num_sub_resources", 1);
						if (num_sub_resources < 1)
							num_sub_resources = 1;
					}

					long min_sub_resource_index = 0;
					if (has_metadata_value(*metadata_, "min_sub_resource_index"))
						min_sub_resource_index = get_metadata_value < long > (*metadata_, "min_sub_resource_index", 1);

					uri::options_t::const_iterator uri_resource_index_iter = uri_.get_options().find("sub_resource_index");
					bool has_resource_index = (uri_resource_index_iter != uri_.get_options().end());
					if ((num_sub_resources == 1) || has_resource_index)
					{
						if (has_metadata_value(*metadata_, "title") && has_resource_index)
						{
							std::string title = get_metadata_value < std::string > (*metadata_, "title", "");
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
							set_metadata_value(*metadata_, "title", sstr.str());
						}

						static_cast < Derived* > (this)->add_entry_to_playlist(uri_, *metadata_);
					}
					else
					{
						for (long sub_resource_nr = 0; sub_resource_nr < num_sub_resources; ++sub_resource_nr)
						{
							ion::uri temp_uri(uri_);
							temp_uri.get_options()["sub_resource_index"] = boost::lexical_cast < std::string > (sub_resource_nr + min_sub_resource_index);
							start_scan(*current_playlist, temp_uri);
						}
					}
				}
			}
			catch (ion::uri::invalid_uri const &invalid_uri_)
			{
				static_cast < Derived* > (this)->report_resource_error("invalid_uri", invalid_uri_.what());
			}
		}
		else if ((event_command_name == "unrecognized_resource")  && (event_params.size() >= 1))
			static_cast < Derived* > (this)->report_resource_error("unrecognized_resource", event_params[0]);
		else if ((event_command_name == "resource_not_found")  && (event_params.size() >= 1))
			static_cast < Derived* > (this)->report_resource_error("resource_not_found", event_params[0]);
		else if ((event_command_name == "resource_corrupted")  && (event_params.size() >= 1))
			static_cast < Derived* > (this)->report_resource_error("resource_corrupted", event_params[0]);
		else if (event_command_name == "error")
			static_cast < Derived* > (this)->report_general_error(line);
		else
			static_cast < Derived* > (this)->report_general_error(line);
	}


	void scanning_process_started()
	{
	}


	void scanning_process_terminated(bool const continue_scanning)
	{
		current_playlist = 0;
		if (continue_scanning)
			init_scanning();

		// TODO: implement crash handling (also mind multiple backend support!)
	}


	void scanning_process_finished(bool const continue_scanning)
	{
		current_playlist = 0;
		if (continue_scanning)
			init_scanning();
	}




	scan_queue_t scan_queue;
	playlist_t *current_playlist;
	playlists_t &playlists_;
	boost::signals2::connection playlist_removed_connection;
};


}


#endif

