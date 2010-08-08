#ifndef ION_FRONTEND_IO_HPP
#define ION_FRONTEND_IO_HPP

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


/*

Playlist concept:

<unspecified function object type (uri_optional_t)> get_current_uri_changed_callback(Playlist const &playlist)
<boost signals2 compatible type> get_resource_added_signal(Playlist const &playlist)
<boost signals2 compatible type> get_resource_removed_signal(Playlist const &playlist)
metadata_optional_t get_metadata_for(Playlist const &playlist, uri const &uri_)
uri_optional_t get_succeeding_uri(Playlist const &playlist, uri const &uri_)
void mark_backend_resource_incompatibility(Playlist const &playlist, uri const &uri_, std::string const &backend_type)


*/


template < typename Playlist >
class frontend_io_base:
	private boost::noncopyable
{
public:
	typedef Playlist playlist_t;
	typedef boost::function < void(std::string const &line) > send_line_to_backend_callback_t;
	typedef boost::signals2::signal < void(uri_optional_t const &new_current_uri) > current_uri_changed_signal_t;
	typedef frontend_io_base < Playlist > self_t;



	explicit frontend_io_base(send_line_to_backend_callback_t const &send_line_to_backend_callback):
		send_line_to_backend_callback(send_line_to_backend_callback),
		current_playlist(0)
	{
	}


	void parse_incoming_line(std::string const &line)
	{
		std::string event_command_name;
		params_t event_params;
		split_command_line(line, event_command_name, event_params);
		parse_command(event_command_name, event_params);
	}


	void set_current_playlist(playlist_t *new_current_playlist)
	{
		if (current_playlist == new_current_playlist)
			return;

		//stop(); // TODO: i do not think this call is really necessary. Verify.
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
			metadata_optional_t current_uri_metadata = get_metadata_for(*current_playlist, uri_);
			if (current_uri_metadata)
				play_params.push_back(get_metadata_string(*current_uri_metadata));
			else
				play_params.push_back("{}");
		}

		{
			uri_optional_t new_next_uri = get_succeeding_uri(*current_playlist, uri_);
			if (new_next_uri)
			{
				play_params.push_back(new_next_uri->get_full());
				metadata_optional_t next_uri_metadata = get_metadata_for(*current_playlist, *new_next_uri);
				if (next_uri_metadata)
					play_params.push_back(get_metadata_string(*next_uri_metadata));
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


	current_uri_changed_signal_t & get_current_uri_changed_signal()
	{
		return current_uri_changed_signal;
	}




protected:
	void parse_command(std::string const &event_command_name, params_t const &event_params)
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

		current_uri_changed_signal(current_uri);
	}


	void stopped(uri const &uri_)
	{
		current_uri = boost::none;
		next_uri = boost::none;
		current_uri_changed_signal(current_uri);
	}


	void resource_finished(uri const &uri_)
	{
		current_uri = boost::none;
		next_uri = boost::none;
		current_uri_changed_signal(current_uri);
	}





	// Callback & signals
	send_line_to_backend_callback_t send_line_to_backend_callback;
	current_uri_changed_signal_t current_uri_changed_signal;
	boost::signals2::connection resource_added_signal_connection, resource_removed_signal_connection;

	// URIs
	uri_optional_t current_uri, next_uri;

	// The playlist
	playlist_t *current_playlist;
};


}


#endif

