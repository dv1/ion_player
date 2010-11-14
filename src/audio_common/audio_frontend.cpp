/**************************************************************************

    Copyright (C) 2010  Carlos Rafael Giani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**************************************************************************/


#include <json/reader.h>
#include <json/writer.h>
#include "audio_frontend.hpp"


namespace ion
{
namespace audio_common
{


audio_frontend::audio_frontend(send_line_to_backend_callback_t const &send_line_to_backend_callback, pong_callback_t const &pong_callback):
	base_t(send_line_to_backend_callback, pong_callback),
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


void audio_frontend::update_module_entries()
{
	send_line_to_backend_callback("get_modules");
}


void audio_frontend::parse_command(std::string const &event_command_name, params_t const &event_params)
{
	if (event_command_name == "paused")
		paused = true;
	else if (event_command_name == "modules")
		parse_modules_list(event_params);
	else if (event_command_name == "module_ui")
		read_module_ui(event_params);
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
	else if (event_command_name == "transition")
		current_position = 0;
	else if ((event_command_name == "started") || (event_command_name == "stopped") || (event_command_name == "resource_finished"))
	{
		paused = false;
		current_position = 0;
	}

	base_t::parse_command(event_command_name, event_params);
}


void audio_frontend::parse_modules_list(params_t const &event_params)
{
	bool first_from_pair = true;
	std::string type, id;

	module_entries.clear();

	BOOST_FOREACH(param_t const &param, event_params)
	{
		if (first_from_pair)
		{
			type = param;
			first_from_pair = false;
		}
		else
		{
			if ((type == "decoder") || (type == "sink"))
			{
				id = param;			
				module_entries.push_back(module_entry(type, id));
				send_line_to_backend_callback(recombine_command_line("get_module_ui", boost::assign::list_of(id)));
			}
			first_from_pair = true;
		}
	}

	module_entries_updated_signal();
}


void audio_frontend::update_module_properties(std::string const &module_id, Json::Value const &ui_properties)
{
	if (!ui_properties.isObject())
		return;

	module_entries_by_id_t &module_entries_by_id = module_entries.get < id_tag > ();
	module_entries_by_id_t::iterator iter = module_entries_by_id.find(module_id);
	if (iter == module_entries_by_id.end())
		return;

	module_entry entry = *iter;
	entry.ui_properties = ui_properties;
	module_entries_by_id.replace(iter, entry);

	std::string json_string = Json::FastWriter().write(ui_properties);
	send_line_to_backend_callback(recombine_command_line("set_module_properties", boost::assign::list_of(module_id)(json_string)));
}


void audio_frontend::set_loop_mode(uri const &uri_, int const loop_mode)
{
	metadata_t loopmode_metadata = empty_metadata();
	set_metadata_value(loopmode_metadata, "loop_mode", loop_mode);
	send_line_to_backend_callback(recombine_command_line("update_metadata", boost::assign::list_of(uri_.get_full())(get_metadata_string(loopmode_metadata))));
}


void audio_frontend::read_module_ui(params_t const &event_params)
{
	if (event_params.size() < 3)
		return; // TODO: report the error

	module_entries_by_id_t &module_entries_by_id = module_entries.get < id_tag > ();
	module_entries_by_id_t::iterator iter = module_entries_by_id.find(event_params[0]);
	if (iter == module_entries_by_id.end())
		return;

	Json::Reader json_reader;
	Json::Value json_properties_value;
	if (!json_reader.parse(event_params[2], json_properties_value))
		return;

	module_entry entry = *iter;
	entry.ui_properties = json_properties_value;
#if 1
	entry.html_code = event_params[1];
#else
	entry.html_code = "<html><head> \n\
		<script type=\"text/javascript\"> \n\
			setTimeout(\"alert(uiProperties[\\\"somekey\\\"]);\", 1000); \n\
		</script></head> \n\
		<body>No user interface for this module available.</body></html>";
	entry.ui_properties["somekey"] = "somevalue";
#endif

	module_entries_by_id.replace(iter, entry);
}


}
}

