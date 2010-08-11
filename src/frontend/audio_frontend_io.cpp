#include "audio_frontend_io.hpp"


namespace ion
{
namespace frontend
{


audio_frontend_io::audio_frontend_io(send_line_to_backend_callback_t const &send_line_to_backend_callback):
	base_t(send_line_to_backend_callback),
	paused(false)
{
}


void audio_frontend_io::pause(bool const set)
{
	if (paused == set)
		return;

	paused = set;

	send_line_to_backend_callback(paused ? "pause" : "resume");
}


bool audio_frontend_io::is_paused() const
{
	return paused;
}


void audio_frontend_io::set_current_volume(long const new_volume)
{
	send_line_to_backend_callback(recombine_command_line("set_current_volume", boost::assign::list_of(boost::lexical_cast < std::string > (new_volume))));
}


void audio_frontend_io::play(uri const &uri_)
{
	paused = false;
	base_t::play(uri_);
}


void audio_frontend_io::stop()
{
	paused = false;
	base_t::stop();
}


void audio_frontend_io::parse_command(std::string const &event_command_name, params_t const &event_params)
{
	if (event_command_name == "paused")
		paused = true;
	else if ((event_command_name == "resumed") || (event_command_name == "started") || (event_command_name == "stopped") || (event_command_name == "resource_finished"))
		paused = false;

	base_t::parse_command(event_command_name, event_params);
}


}
}

