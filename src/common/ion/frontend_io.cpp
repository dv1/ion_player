#include <boost/assign/list_of.hpp>
#include "metadata.hpp"
#include "command_line_tools.hpp"
#include "playlist.hpp"
#include "frontend_io.hpp"


namespace ion
{


frontend_io::frontend_io(send_line_to_backend_t const &send_line_to_backend):
	current_playlist(0),
	send_line_to_backend(send_line_to_backend)
{
}


frontend_io::~frontend_io()
{
}


void frontend_io::parse_incoming_line(std::string const &line)
{
	std::string event_command_name;
	params_t event_params;

	split_command_line(line, event_command_name, event_params);

	if ((event_command_name == "transition") && (event_params.size() >= 2))
		transition(event_params[0], event_params[1]);
	else if (event_command_name == "started")
	{
		if (event_params.size() >= 2)
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


void frontend_io::play(uri const &uri_)
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


void frontend_io::stop()
{
	send_line_to_backend("stop");
}


void frontend_io::resource_added(uri const &added_uri)
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


void frontend_io::resource_removed(uri const &removed_uri)
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


void frontend_io::backend_started(std::string const &backend_type)
{
}


void frontend_io::backend_terminated() // not to be called if the backend exists normally
{
}


void frontend_io::transition(uri const &, uri const &new_uri)
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


void frontend_io::started(uri const &current_uri_, uri_optional_t const &next_uri_)
{
	current_uri = current_uri_;
	next_uri = next_uri_;
	current_uri_changed_signal(current_uri);
}


void frontend_io::stopped(uri const &uri_)
{
	current_uri = boost::none;
	next_uri = boost::none;
	current_uri_changed_signal(current_uri);
}


void frontend_io::resource_finished(uri const &uri_)
{
	current_uri = boost::none;
	next_uri = boost::none;
	current_uri_changed_signal(current_uri);
}


void frontend_io::set_current_playlist(playlist &new_current_playlist)
{
	stop();
	current_playlist = &new_current_playlist;
}


}

