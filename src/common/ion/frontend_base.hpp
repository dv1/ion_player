#ifndef ION_FRONTEND_BASE_HPP
#define ION_FRONTEND_BASE_HPP

#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/function.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

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
	typedef frontend_base < Playlist > self_t;


	// Constructor; the parameter is a callback that sends a line to the backend process' stdin.
	explicit frontend_base(send_line_to_backend_callback_t const &send_line_to_backend_callback):
		send_line_to_backend_callback(send_line_to_backend_callback),
		current_playlist(0)
	{
	}

	virtual ~frontend_base()
	{
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
			resource_added_signal_connection = get_resource_added_signal(*current_playlist).connect(boost::lambda::bind(&self_t::resource_added, this, boost::lambda::_1));
			resource_removed_signal_connection = get_resource_removed_signal(*current_playlist).connect(boost::lambda::bind(&self_t::resource_removed, this, boost::lambda::_1));
		}
	}




	// Actions

	void play(uri const &uri_)
	{
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
		send_line_to_backend_callback("stop");
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


	uri_optional_t const & get_current_uri() const { return current_uri; }
	metadata_optional_t const & get_current_metadata() const { return current_metadata; }

	void set_current_metadata(metadata_optional_t const &metadata)
	{
		current_metadata = metadata;
		current_metadata_changed_signal(current_metadata);
	}




protected:
	virtual void parse_command(std::string const &event_command_name, params_t const &event_params)
	{
		if ((event_command_name == "transition") && (event_params.size() >= 2))
			transition(event_params[0], event_params[1]);
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

	void resource_added(uri const &added_uri)
	{
		if (current_playlist == 0)
			return;
		if (!current_uri)
			return;

		uri_optional_t uri_ = get_succeeding_uri(*current_playlist, *current_uri);
		if (uri_ == added_uri) // the added uri is right after the current uri, meaning that it just became the next uri
		{
			next_uri = added_uri;
			send_line_to_backend_callback(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
		}
	}


	void resource_removed(uri const &removed_uri)
	{
		if (current_playlist == 0)
			return;

		// the current uri was removed -> playback next resource
		if (removed_uri == current_uri)
		{
			if (next_uri)
				play(*next_uri);
			else
				stop();
			return;
		}

		// the *next* uri was removed -> get a new next resource
		if (current_uri && next_uri && (removed_uri == *next_uri))
		{
			next_uri = get_succeeding_uri(*current_playlist, *current_uri);
			if (next_uri)
				send_line_to_backend_callback(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
			else
				send_line_to_backend_callback("clear_next_resource");
		}
	}


	// Backend start/termination event handlers

	void backend_started(std::string const &backend_type)
	{
	}


	void backend_terminated() // not to be called if the backend exists normally
	{
	}


	void transition(uri const &old_uri, uri const &new_uri)
	{
		current_uri = new_uri;
		if (current_playlist != 0)
		{
			next_uri = get_succeeding_uri(*current_playlist, new_uri);
			if (next_uri)
				send_line_to_backend_callback(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
		}
		current_uri_changed_signal(current_uri);
	}


	void started(uri const &current_uri_, uri_optional_t const &next_uri_)
	{
		if (current_playlist == 0)
			return;

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


	void stopped(uri const &uri_)
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
	boost::signals2::connection resource_added_signal_connection, resource_removed_signal_connection;

	// URIs
	uri_optional_t current_uri, next_uri;

	// The playlist
	playlist_t *current_playlist;

	// Misc
	metadata_optional_t current_metadata;
};


}


#endif

