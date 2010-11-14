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


#ifndef ION_FRONTEND_BASE_HPP
#define ION_FRONTEND_BASE_HPP

#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/function.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>

#include <ion/uri.hpp>

#include "metadata.hpp"
#include "command_line_tools.hpp"


namespace ion
{


// TODO: install exception handlers for invalid_uri exceptions


// See Playlist concept in docs/concepts.txt
template < typename Playlist >
class frontend_base:
	private boost::noncopyable
{
public:
	typedef Playlist playlist_t;
	typedef boost::function < void(std::string const &line) > send_line_to_backend_callback_t;
	typedef boost::signals2::signal < void(uri_optional_t const &new_current_uri) > current_uri_changed_signal_t;
	typedef boost::signals2::signal < void(metadata_optional_t const &new_metadata) > current_metadata_changed_signal_t;
	typedef boost::signals2::signal < void(uri const &uri_, metadata_t const &uri_metadata) > new_metadata_signal_t;
	typedef frontend_base < Playlist > self_t;


	// Constructor; the parameter is a callback that sends a line to the backend process' stdin.
	explicit frontend_base(send_line_to_backend_callback_t const &send_line_to_backend_callback):
		send_line_to_backend_callback(send_line_to_backend_callback),
		current_playlist(0),
		crash_count(0),
		num_allowed_crashes(0),
		backend_broken(false)
	{
	}

	virtual ~frontend_base()
	{
		resource_added_signal_connection.disconnect();
		resource_removed_signal_connection.disconnect();
	}


	/*
	This function is called when the backend process' stdout sent a complete line.
	These lines are events sent by the backend to the frontend. This function splits the line into event command name and parameters,
	and then parses these using parse_command().

	@pre The line must be a valid command line.
	@post If parsing is successful (and the command is known), the respective handler is invoked, otherwise nothing happens.
	*/
	void parse_incoming_line(std::string const &line)
	{
		std::string event_command_name;
		params_t event_params;
		split_command_line(line, event_command_name, event_params);
		parse_command(event_command_name, event_params);
	}


	/*
	Sets the current playlist. The current playlist is the one that is queried for URIs. For instance, when a transition happens, the transition() event handler in this class
	queries this playlist for a next URI.
	Also, the current playlist's signals are connected to handlers from this class. This is necessary to handle special cases when a resource is added/removed.
	If another playlist was previously set as the current playlist, it is replaced, its signals get disconnected.
	This function does nothing if the same playlist is already set as the current one.

	@param new_current_playlist The new current playlist to use. If this is zero, and another playlist was previously set, its signals get disconnected, and the current playlist is set to null.
	Otherwise, this function does nothing when this parameter is zero.
	@post If the same playlist was previously set, this function does nothing (this includes duplicate calls with nullpointers). Otherwise, the given playlist becomes the current one, and its
	resource_added/removed signals are connected to the resource_added()/removed() handlers. Any previously set playlist is replaced, its signals disconnected.
	*/
	void set_current_playlist(playlist_t *new_current_playlist)
	{
		if (current_playlist == new_current_playlist)
			return;

		resource_added_signal_connection.disconnect();
		resource_removed_signal_connection.disconnect();

		current_playlist = new_current_playlist;

		if (current_playlist != 0)
		{
			resource_added_signal_connection = get_resource_added_signal(*current_playlist).connect(boost::phoenix::bind(&self_t::resource_added, this, boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));
			resource_removed_signal_connection = get_resource_removed_signal(*current_playlist).connect(boost::phoenix::bind(&self_t::resource_removed, this, boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));
		}
	}


	playlist_t* get_current_playlist() { return current_playlist; }




	// Actions

	void play(uri const &uri_)
	{
		if (backend_broken)
			return;

		params_t play_params = boost::assign::list_of(uri_.get_full());

		{
			metadata_optional_t metadata_ = get_metadata_for(*current_playlist, uri_);
			if (metadata_)
				play_params.push_back(get_metadata_string(*metadata_));
			else
				play_params.push_back(empty_metadata_string());
		}

		{
			uri_optional_t new_next_uri = get_succeeding_uri(*current_playlist, uri_);
			if (new_next_uri)
			{
				play_params.push_back(new_next_uri->get_full());
				metadata_optional_t metadata_ = get_metadata_for(*current_playlist, *new_next_uri);
				if (metadata_)
					play_params.push_back(get_metadata_string(*metadata_));
			}
		}

		send_line_to_backend_callback(recombine_command_line("play", play_params));
	}


	void stop()
	{
		if (backend_broken)
			return;

		send_line_to_backend_callback("stop");
	}


	void reacquire_next_resource()
	{
		if (current_playlist == 0)
			return;
		if (!current_uri)
			return;

		uri_optional_t new_next_uri = get_succeeding_uri(*current_playlist, *current_uri);

		// some checks to catch redundant calls

		// if there wasnt a new uri before, exit
		if (!new_next_uri && !next_uri)
			return;

		// if there was a next value before, a new next value exists, and both are identical, exit
		if (new_next_uri && next_uri)
		{
			if (*new_next_uri == *next_uri)
				return;
		}

		// the next uri actually changed -> store the new next uri and send command to backend
		next_uri = new_next_uri;

		if (next_uri)
			send_line_to_backend_callback(recombine_command_line("set_next_resource", boost::assign::list_of(new_next_uri->get_full())));
		else
			send_line_to_backend_callback("clear_next_resource");
	}


	void move_to_previous_resource()
	{
		if (current_playlist == 0)
			return;
		if (!current_uri)
			return;

		uri_optional_t uri_ = get_preceding_uri(*current_playlist, *current_uri);
		if (uri_)
			play(*uri_);
	}


	void move_to_next_resource()
	{
		if (current_playlist == 0)
			return;
		if (!current_uri)
			return;

		uri_optional_t uri_ = get_succeeding_uri(*current_playlist, *current_uri);
		if (uri_)
			play(*uri_);
	}


	current_uri_changed_signal_t & get_current_uri_changed_signal() { return current_uri_changed_signal; }
	current_metadata_changed_signal_t & get_current_metadata_changed_signal() { return current_metadata_changed_signal; }
	new_metadata_signal_t & get_new_metadata_signal() { return new_metadata_signal; }


	uri_optional_t const & get_current_uri() const { return current_uri; }
	metadata_optional_t const & get_current_metadata() const { return current_metadata; }

	void set_current_metadata(metadata_optional_t const &metadata)
	{
		current_metadata = metadata;
		current_metadata_changed_signal(current_metadata);
	}


	// Backend start/termination event handlers

	// Backend just started; depending on the number of crashes, either restart playback, or mark the resource as incompatible with the backend and move
	// to the next one.
	void backend_started(std::string const &backend_type)
	{
		if (backend_type != current_backend_type)
		{
			crash_count = 0;
			backend_broken = false;
			current_backend_type = backend_type;
		}

		if (current_uri)
		{
			if (crash_count < num_allowed_crashes)
			{
				uri current_uri_ = *current_uri;
				play(current_uri_);
			}
			else
			{
				crash_count = 0;
				if (current_playlist != 0)
					mark_backend_resource_incompatibility(*current_playlist, *current_uri, backend_type);
				move_to_next_resource();
			}
		}
	}


	// not to be called if the backend exists normally
	// returnvalue true => restart backend, false => do not restart backend
	// If playback is running (= current_uri is valid), just return true. Otherwise, if there were 5 crashes or more, mark the backend as broken (since it kept crashing while idling), and
	// tell the caller not to restart this backend. Otherwise, tell the caller to restart it.
	bool backend_terminated()
	{
		return true;
		// TODO: the code below must not be in effect if only one backend is present, since otherwise nothing will ever happen,
		// and the logfile will never report any more backend crashes, which is bad for troubleshooting
#if 0
		++crash_count;

		if (current_uri)
		{
			return true;
		}
		else
		{
			if (crash_count >= 5)
			{
				backend_broken = true;
				return false;
			}
			else
				return true;
		}
#endif
	}




protected:
	virtual void parse_command(std::string const &event_command_name, params_t const &event_params)
	{
		if ((event_command_name == "transition") && (event_params.size() >= 2))
			transition(event_params[0], event_params[1]);
		else if (event_command_name == "metadata")
		{
			metadata_optional_t metadata_ = parse_metadata(event_params[1]);
			if (metadata_)
				new_metadata_signal(uri(event_params[0]), *metadata_);
		}
		else if (event_command_name == "started")
		{
			if (event_params.size() >= 2 && !event_params[1].empty())
				started(uri(event_params[0]), uri(event_params[1]));
			else if (event_params.size() >= 1)
				started(uri(event_params[0]), boost::none);
		}
		else if ((event_command_name == "stopped") && (event_params.size() >= 1))
			stopped(event_params[0]);
		else if ((event_command_name == "resource_finished") && (event_params.size() >= 1))
			resource_finished(event_params[0]);
	}



	// Playlist event handlers

	void resource_added(uri_set_t const &added_uris, bool const before)
	{
		if (current_playlist == 0)
			return;
		if (!current_uri)
			return;
		if (before)
			return;

		uri_optional_t actual_next_uri = get_succeeding_uri(*current_playlist, *current_uri);

		/*
		The added URIs may be located between the one contained in current_uri and the one contained in next_uri.
		For example: in the playlist, there are URIs 1 and 2:   ... 1 2 ...
		1 is the current one, 2 is the next one.
		After adding, there are new URIs between the two:  .... 1 3 4 5 2 ...
		Now, 2 is no longer the next URI, but next_uri still contains "2".
		The code below rectifies this by testing if the *actual* next URI (retrieved by using get_succeeding_uri()) is one of the newly added ones.
		If so, update next_uri, and notify the backend with the set_next_resource command that the next URI changed.
		*/
		if (actual_next_uri)
		{
			if (added_uris.find(*actual_next_uri) != added_uris.end())
			{
				next_uri = *actual_next_uri;
				send_line_to_backend_callback(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
			}
		}
	}


	void resource_removed(uri_set_t const &removed_uris, bool const before)
	{
		if (current_playlist == 0)
			return;
		if (!current_uri)
			return;
		if (before)
			return;

		/*
		When URIs are removed, the currently playing one as well as the next one might have been removed as well. This requires extra handling.
		This code tests for four possible cases:
		- current URI removed, next URI removed: stop playback (it is unclear what URI to playback at this point; TODO: maybe this can be clarified by using the "before" mode)
		- current URI removed, next URI not removed: set current URI = next URI if there is a next one, otherwise stop playback
		- current URI not removed, next URI removed: determine the new next URI
		- current URI not removed, next URI not removed: nothing needs to be done
		*/

		bool cur_removed = false, next_removed = false;
		
		cur_removed = removed_uris.find(*current_uri) != removed_uris.end();
		if (next_uri)
			next_removed = removed_uris.find(*next_uri) != removed_uris.end();

		if (cur_removed)
		{
			if (next_removed)
				stop();
			else
			{
				if (next_uri)
					play(*next_uri);
				else
					stop();
			}
		}
		else
		{
			if (next_removed)
			{
				next_uri = get_succeeding_uri(*current_playlist, *current_uri);
				if (next_uri)
					send_line_to_backend_callback(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
				else
					send_line_to_backend_callback("clear_next_resource");
			}
		}
	}


	void transition(uri const &old_uri, uri const &new_uri)
	{
		current_uri = new_uri;
		if (current_playlist != 0)
		{
			next_uri = get_succeeding_uri(*current_playlist, new_uri);
			if (next_uri)
				send_line_to_backend_callback(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
			current_metadata = get_metadata_for(*current_playlist, new_uri);
			current_uri_changed_signal(current_uri);
			current_metadata_changed_signal(current_metadata);
		}
		else
			stopped(old_uri);
	}


	void started(uri const &current_uri_, uri_optional_t const &next_uri_)
	{
		if (current_playlist == 0)
			return;

		// Reset the crash count if a different URI is to be played. New resource means a fresh try. Previous crashes of previous URIs must not affect playback.
		if (current_uri)
		{
			if (*current_uri != current_uri_)
			{
				crash_count = 0;
			}
		}

		current_uri = current_uri_;

		/*
		There is the possibility that set_next_resource is sent right when a resource finished playing.
		In chronological order:
		1. set_next_resource is sent.
		2. current resource finishes playing -before- set_next_resource arrives -> resource_finished is sent back.
		3. frontend receives resource_finished, assumes playback stopped.
		4. backend receives set_next_resource, which at this point behaves just like play, because the playback finished just before.
		5. backend sends back a started event, with current_uri = the next_uri from that set_next_resource command, and next_uri set to null.
		6. when this playback ends, no transition will happen, even though there might be a next resource in the playlist.

		For example, one might have added five songs just when the current song finished playing. The case described above could happen then.
		The first of the five songs would start playing just like in step (4), and the second one will not be started properly, because no next uri
		is set in the backend.

		The solution is: when next uri is not set, check if there is _really_ no next resource. If there is in fact one, use it as the next uri and
		send set_next_resource.
		*/
		if (!next_uri_)
		{
			next_uri = get_succeeding_uri(*current_playlist, current_uri_);
			if (next_uri)
				send_line_to_backend_callback(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
		}
		else
			next_uri = next_uri_;

		current_metadata = get_metadata_for(*current_playlist, current_uri_);
		current_uri_changed_signal(current_uri);
		current_metadata_changed_signal(current_metadata);
	}


	void stopped(uri const &/*uri_*/)
	{
		current_uri = boost::none;
		next_uri = boost::none;
		current_metadata = boost::none;
		current_uri_changed_signal(current_uri);
		current_metadata_changed_signal(current_metadata);
	}


	void resource_finished(uri const &uri_)
	{
		current_uri = boost::none;
		next_uri = boost::none;
		current_metadata = boost::none;
		current_uri_changed_signal(current_uri);
		current_metadata_changed_signal(current_metadata);
	}





	// Callback & signals
	send_line_to_backend_callback_t send_line_to_backend_callback;
	current_uri_changed_signal_t current_uri_changed_signal;
	current_metadata_changed_signal_t current_metadata_changed_signal;
	new_metadata_signal_t new_metadata_signal;
	boost::signals2::connection resource_added_signal_connection, resource_removed_signal_connection;

	// URIs
	uri_optional_t current_uri, next_uri;

	// The playlist
	playlist_t *current_playlist;

	// Misc
	metadata_optional_t current_metadata;
	unsigned int crash_count;
	unsigned int const num_allowed_crashes;
	bool backend_broken;
	std::string current_backend_type;
};


}


#endif

