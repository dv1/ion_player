#include "audio_frontend.hpp"


namespace ion
{
namespace frontend
{


audio_frontend::audio_frontend(send_line_to_backend_callback_t const &send_line_to_backend_callback):
	base_t(send_line_to_backend_callback),
	paused(false),
	current_position(0)
{
}


void audio_frontend::pause(bool const set)
{
	if (paused == set)
		return;

	paused = set;

	send_line_to_backend_callback(paused ? "pause" : "resume");
}


bool audio_frontend::is_paused() const
{
	return paused;
}


void audio_frontend::set_current_volume(long const new_volume)
{
	send_line_to_backend_callback(recombine_command_line("set_current_volume", boost::assign::list_of(boost::lexical_cast < std::string > (new_volume))));
}


void audio_frontend::set_current_position(unsigned int const new_position)
{
	current_position = new_position;
	send_line_to_backend_callback(recombine_command_line("set_current_position", boost::assign::list_of(boost::lexical_cast < std::string > (new_position))));
}


void audio_frontend::issue_get_position_command()
{
	send_line_to_backend_callback("get_current_position");
}


unsigned int audio_frontend::get_current_position() const
{
	return current_position;
}


void audio_frontend::play(uri const &uri_)
{
	base_t::play(uri_);
}


void audio_frontend::stop()
{
	base_t::stop();
}


void audio_frontend::parse_command(std::string const &event_command_name, params_t const &event_params)
{
	if (event_command_name == "paused")
		paused = true;
	else if ((event_command_name == "current_position") && (event_params.size() >= 1))
	{
		try
		{
			current_position = boost::lexical_cast < unsigned int > (event_params[0]);
		}
		catch (boost::bad_lexical_cast const &)
		{
		}
	}
	else if (event_command_name == "resumed")
		paused = false;
	else if ((event_command_name == "started") || (event_command_name == "stopped") || (event_command_name == "resource_finished"))
	{
		paused = false;
		current_position = 0;
	}

	base_t::parse_command(event_command_name, event_params);
}


}
}

