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


/*void audio_frontend_io::get_metadata_for(uri const &uri_)
{
	send_line_to_backend_callback(recombine_command_line("get_metadata", boost::assign::list_of(uri_.get_full())));
}*/


void audio_frontend_io::parse_command(std::string const &event_command_name, params_t const &event_params)
{
/*	if ((event_command_name == "metadata") && (event_params.size() >= 2) && (current_playlist != 0))
	{
		try
		{
			ion::uri uri_(event_params[0]);
			metadata_t metadata_(event_params[1]);
			if (metadata_.isObject()) // TODO: remove the jsoncpp specific calls, make this more generic, like is_valid(metadata_)
			{
				// TODO: instead of using the current playlist, do some lookup; the metadata events were caused by get_metadata calls, which may not be directed to the current playlist
				// for instance, the user might have wanted to scan a very large directory while playing stuff from another playlist
				// TODO: improve the "playlist_t::entry" part; maybe something like playlist_traits < playlist_t > ::entry, or just get rid of the explicit entry type entirely
				// add_entry(current_playlist, uri_, metadata_) sounds better
				current_playlist->add_entry(playlist_t::entry(uri_, metadata_));
			}
			else
				std::cerr << "Metadata for URI \"" << uri_.get_full() << "\" is invalid!" << std::endl;
		}
		catch (ion::uri::invalid_uri const &)
		{
		}
	}
	else */if (event_command_name == "paused")
		paused = true;
	else if ((event_command_name == "resumed") || (event_command_name == "started") || (event_command_name == "stopped") || (event_command_name == "resource_finished"))
		paused = false;

	base_t::parse_command(event_command_name, event_params);
}


}
}

