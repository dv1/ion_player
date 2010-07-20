#include <boost/assign/list_of.hpp>
#include <ion/command_line_tools.hpp>
#include "backend_handler.hpp"


namespace ion
{
namespace frontend
{


backend_handler::backend_handler(send_to_backend_function_t const &send_to_backend_function):
	send_to_backend_function(send_to_backend_function),
	playlist_(0)
{
}


backend_handler::~backend_handler()
{
}


void backend_handler::set_playlist(playlist const &new_playlist)
{
	stop();
	playlist_ = &new_playlist;
}


void backend_handler::play(uri const &uri_)
{
	uri_optional_t new_next_uri = playlist_->get_succeeding_uri(uri_);

	if (new_next_uri)
		send_to_backend_function(recombine_command_line("play", boost::assign::list_of(uri_.get_full())(new_next_uri->get_full())));
	else
		send_to_backend_function(recombine_command_line("play", boost::assign::list_of(uri_.get_full())));

	// TODO: move this code into started() (give the started command a next_uri parameter first)
	current_uri = uri_;
	next_uri = new_next_uri;
	current_uri_changed_signal(current_uri);
}


void backend_handler::stop()
{
	send_to_backend_function("stop");
}


backend_handler::uri_optional_t backend_handler::get_current_uri() const
{
	return current_uri;
}


void backend_handler::transition(uri const &old_uri, uri const &new_uri)
{
	current_uri = new_uri;
	if (playlist_ != 0)
	{
		next_uri = playlist_->get_succeeding_uri(new_uri);
		if (next_uri)
			send_to_backend_function(recombine_command_line("set_next_resource", boost::assign::list_of(next_uri->get_full())));
	}
	current_uri_changed_signal(current_uri);
}


void backend_handler::started(uri const &uri_)
{
}


void backend_handler::stopped(uri const &uri_)
{
	current_uri = boost::none;
	next_uri = boost::none;
	current_uri_changed_signal(current_uri);
}


void backend_handler::song_finished(uri const &uri_)
{
	current_uri = boost::none;
	next_uri = boost::none;
	current_uri_changed_signal(current_uri);
}


void backend_handler::resource_added(uri const &uri_)
{
}


void backend_handler::resource_removed(uri const &uri_)
{
}


}
}

