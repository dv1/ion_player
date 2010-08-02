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
#include "playlist.hpp"


namespace ion
{


/*

This class is used for communicating with a backend. More specifically, it sends commands, receives events, and reacts to the latter.
It does -not- directly start a backend; instead, it abstracts away I/O with the backend by using a function and a callback.
- The parse_incoming_line() function is to be called whenever the backend process sent a line
- The send_line_to_backend callback will be used whenever this class wants to send the backend a command

frontend_io is used as a part of a system. Said system performs the actual backend startup, and controls its I/O streams (typically stdin
and stdout).


The frontend io's logic can be grouped in three categories:

- actions: tellng the backend to play, stop, ..
- backend event handlers: when the backend sends an event, these react accordingly; example: transition event handler
- playlist event handlers: these are called by the playlist, to inform the frontend io about playlist changes that are relevant to the current playback
  for example, when the currently playing resource gets deleted from the playlist, the backend should immediately transition to the next resource



=== actions:
These are simple. They just send a command to the backend, nothing more.

* play: gets an uri as parameter. It then asks the playlist for a uri that succeeds the given one, provided one exists.
  The playlist is also asked to provide metadata for the uri(s).
  Then the play command is sent to the backend. If a succeeding uri was retrieved, it is put in the play command's parameter.
  As for the metadata: for each uri the playlist didn't have metadata for, a get_metadata command is sent as well.

* stop: just sends the stop command.



=== backend events:

* metadata: information about a resource's metadata. The handler tells the playlist about said metadata.

* transition: has uris for the previous and the next resource. The next resource is about to start playing, the previous one
  is being phased out. This handler looks up a succeeding uri for the the next resource. If one is found, it asks the playlist
  for metadata for it, and sends a set_next_resource command, with the found uri and its metadata as parameter. frontend_io's
  current_uri and next_uri values are changed as well; current_uri is set to next_uri's value, and next_uri is set to the succeeding
  uri. If no succeeding uri was found, then next_uri is nulled. In any way, listeners are notified about the change in current_uri's
  value.

* started: informs about playback start. This is not sent by the backend when a transition happens. It has one or two parameters,
  the currently playing resource's uri, and the uri of the next resource. The latter is not present if no next resource was
  specified to the backend.
  Upon receiving this event, the handler sets frontend_io's internal current_uri and next_uri values, and notifies listeners
  that frontend_io's current uri just got changed.

* stopped: sent when playback was stopped by the user. NOT sent when a transition happens (see the transition event) or the resource
  finished with no next resource set (see the resource_finished event). The current_uri and next_uri values get nulled, and listeners
  are notified about current_uri's value change.

* resource_finished: very similar to stopped, except that this is sent by the backend when the resource finished playing with no next
  resource specified. It is NOT set when the user stopped playback (see the stopped event) or when a transition happens (see the
  transition event).



=== playlist event handlers

* resource added: a resource just got added to the playlist. The handler checks if the new resource has been inserted -before- the
  one that was set as the next resource. If this is the case, then the next resource is set to this new one.

* resource removed: a resource just got removed from the playlist. Two cases need to be handled:
  - the removed resource might be the one that was previously set as the next resource. In this case, the next resource needs to be
    determined again by asking the playlist for the uri that succeeds the currently playing resource's. set_next_resource is then
    sent to the backend, either with the uri the playlist returned, or with an empty string as parameter if no such uri was found.
  - The removed resource might be the currently playling resource's. In this case, tell the backend to immediately initiate a
    transition. The rest is just regular transition logic.



Additional actions and event handlers are front/backend specific (= defined in derived frontend_ios). Examples are pause, resume,
set/get volume, set/get position, set/get current subsong, get settings ui, ....



=== Handling crashing backends

If the backend crashes during playback, the frontend_io will get notified, that is, its backend_terminated() function will get called.
The frontend_io keeps a termination counter, which starts at zero. Each backend_terminated() call increases this counter. If the counter reaches
3, the current resource will be skipped (like a transition, but without the communication to the backend), and the playlist will be told
that this backend cannot handle this resource. The playlist can then try another backend, and if all backends crash or are otherwise unable
to handle the resource, mark the resource as broken. After the backend was restarted, backend_started() gets called. The frontend_io then
retries to play the current song.


*/


template < typename Playlist >
class frontend_io:
	private boost::noncopyable
{
public:
	typedef Playlist playlist_t;
	typedef boost::function < void(std::string const &line) > send_line_to_backend_t;
	typedef boost::signals2::signal < void(uri_optional_t const &new_current_uri) > current_uri_changed_signal_t;
	typedef frontend_io < Playlist > self_t;



	explicit frontend_io(send_line_to_backend_t const &send_line_to_backend):
		current_playlist(0),
		send_line_to_backend(send_line_to_backend)
	{
	}


	virtual ~frontend_io()
	{
	}


	void parse_incoming_line(std::string const &line)
	{
		std::string event_command_name;
		params_t event_params;

		split_command_line(line, event_command_name, event_params);

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


	// TODO: install exception handlers for invalid_uri exceptions


	// Actions

	void play(uri const &uri_)
	{
		params_t play_params = boost::assign::list_of(uri_.get_full());

		{
			metadata_optional_t current_uri_metadata = current_playlist->get_metadata_for(uri_);
			if (current_uri_metadata)
				play_params.push_back(get_metadata_string(*current_uri_metadata));
			else
				play_params.push_back("{}");
		}

		{
			uri_optional_t new_next_uri = current_playlist->get_succeeding_uri(uri_);
			if (new_next_uri)
			{
				play_params.push_back(new_next_uri->get_full());
				metadata_optional_t next_uri_metadata = current_playlist->get_metadata_for(*new_next_uri);
				if (next_uri_metadata)
					play_params.push_back(get_metadata_string(*next_uri_metadata));
			}
		}

		send_line_to_backend(recombine_command_line("play", play_params));
	}


	void stop()
	{
		send_line_to_backend("stop");
	}


	// Playlist event handlers

	void resource_added(uri const &added_uri)
	{
		if (current_playlist == 0)
			return;
		if (!current_uri)
			return;

		uri_optional_t uri_ = current_playlist->get_succeeding_uri(*current_uri);
		if (uri_ == added_uri) // the added uri is right after the current uri, meaning that it just became the next uri
		{
			next_uri = added_uri;
			send_line_to_backend(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
		}
	}


	void resource_removed(uri const &removed_uri)
	{
		if (current_playlist == 0)
			return;

		// the current uri was removed -> trigger a transition to the next resource
		if (removed_uri == current_uri)
		{
			send_line_to_backend("trigger_transition");
			return;
		}

		// the *next* uri was removed -> get a new next resource
		if (current_uri && next_uri && (removed_uri == *next_uri))
		{
			next_uri = current_playlist->get_succeeding_uri(*current_uri);
			send_line_to_backend(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
		}
	}


	// Backend start/termination event handlers

	virtual void backend_started(std::string const &backend_type)
	{
	}


	virtual void backend_terminated() // not to be called if the backend exists normally
	{
	}


	void set_current_playlist(playlist_t &new_current_playlist)
	{
		stop();
		current_playlist = &new_current_playlist;
		current_uri_changed_signal_connection = current_uri_changed_signal.connect(boost::lambda::bind(&playlist_t::current_uri_changed, current_playlist, boost::lambda::_1));
		resource_added_signal_connection = current_playlist->get_resource_added_signal().connect(boost::lambda::bind(&self_t::resource_added, this, boost::lambda::_1));
		resource_removed_signal_connection = current_playlist->get_resource_removed_signal().connect(boost::lambda::bind(&self_t::resource_removed, this, boost::lambda::_1));
	}


protected:
	void transition(uri const &old_uri, uri const &new_uri)
	{
		current_uri = new_uri;
		if (current_playlist != 0)
		{
			next_uri = current_playlist->get_succeeding_uri(new_uri);
			if (next_uri)
				send_line_to_backend(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
		}
		current_uri_changed_signal(current_uri);
	}


	void started(uri const &current_uri_, uri_optional_t const &next_uri_)
	{
		current_uri = current_uri_;
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




	bool termination_counter;
	uri_optional_t current_uri, next_uri;
	playlist_t *current_playlist;
	send_line_to_backend_t send_line_to_backend;
	current_uri_changed_signal_t current_uri_changed_signal;
	boost::signals2::connection current_uri_changed_signal_connection;
	typename playlist_t::connection_t resource_added_signal_connection, resource_removed_signal_connection;
};


}


#endif

