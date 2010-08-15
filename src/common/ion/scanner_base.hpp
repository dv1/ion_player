#ifndef ION_SCANNER_BASE_HPP
#define ION_SCANNER_BASE_HPP

#include <deque>
#include <iostream>
#include <boost/noncopyable.hpp>
#include <ion/command_line_tools.hpp>
#include <ion/metadata.hpp>
#include <ion/uri.hpp>


namespace ion
{


/*
Derived must contain:
- bool is_process_running() const
- void start_process(ion::uri const &uri_to_be_scanned)
- void add_entry_to_playlist(ion::uri const &new_uri, metadata_t const &new_metadata)
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


template < typename Derived, typename Playlist >
class scanner_base:
	private boost::noncopyable
{
public:
	typedef Derived derived_t;
	typedef Playlist playlist_t;


	explicit scanner_base():
		current_playlist(0)
	{
	}


	~scanner_base()
	{
	}


	void start_scan(playlist_t &playlist, ion::uri const &uri_to_be_scanned)
	{
		scan_queue.push_back(scan_entry_t(&playlist, uri_to_be_scanned));
		if (!static_cast < Derived* > (this)->is_process_running())
			init_scanning();
	}


protected:
	void init_scanning()
	{
		if (scan_queue.empty())
			return;

		scan_entry_t scan_entry = scan_queue.front();
		current_playlist = scan_entry.first;
		scan_queue.pop_front();

		static_cast < Derived* > (this)->start_process(scan_entry.second);
	}


	void read_process_stdin_line(std::string const &line)
	{
		if (current_playlist == 0)
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
					std::cerr << "Metadata for URI \"" << uri_.get_full() << "\" is invalid!" << std::endl;
				}

				static_cast < Derived* > (this)->add_entry_to_playlist(uri_, *metadata_);
			}
			catch (ion::uri::invalid_uri const &invalid_uri_)
			{
				std::cerr << "Caught invalid URI \"" << invalid_uri_.what() << '"' << std::endl;
			}
		}
	}


	void scanning_process_started()
	{
	}


	void scanning_process_terminated()
	{
		// TODO: implement crash handling (also mind multiple backend support!)
	}


	void scanning_process_finished(bool const continue_scanning)
	{
		current_playlist = 0;
		if (continue_scanning)
			init_scanning();
	}




	typedef std::pair < playlist_t *, ion::uri > scan_entry_t;
	typedef std::deque < scan_entry_t > scan_queue_t;
	scan_queue_t scan_queue;
	playlist_t *current_playlist;
};


}


#endif

