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


#ifndef ION_AUDIO_COMMON_AUDIO_FRONTEND_HPP
#define ION_AUDIO_COMMON_AUDIO_FRONTEND_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/signals2/signal.hpp>

#include <json/value.h>

#include <ion/frontend_base.hpp>
#include <ion/playlist.hpp>


namespace ion
{
namespace audio_common
{


/*
Frontend code for communicating with the audio backend.
*/
class audio_frontend:
	public frontend_base < playlist >
{
public:
	typedef frontend_base < playlist > base_t;

	struct module_entry
	{
		std::string type, id;
		std::string html_code;
		Json::Value ui_properties;

		explicit module_entry();
		explicit module_entry(std::string const &type, std::string const &id):
			type(type),
			id(id)
		{
		}
	};

	struct sequence_tag {};
	struct type_tag {};
	struct id_tag {};

	typedef boost::multi_index::multi_index_container <
		module_entry,
		boost::multi_index::indexed_by <
			boost::multi_index::random_access < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_non_unique <
				boost::multi_index::tag < type_tag >,
				boost::multi_index::member < module_entry, std::string, &module_entry::type >
			>,
			boost::multi_index::ordered_unique <
				boost::multi_index::tag < id_tag >,
				boost::multi_index::member < module_entry, std::string, &module_entry::id >
			>
		>
	> module_entries_t;

	typedef module_entries_t::index < sequence_tag > ::type module_entry_sequence_t;
	typedef module_entries_t::index < type_tag > ::type module_entries_by_type_t;
	typedef module_entries_t::index < id_tag > ::type module_entries_by_id_t;

	typedef boost::signals2::signal < void() > module_entries_updated_signal_t;


	explicit audio_frontend(send_line_to_backend_callback_t const &send_line_to_backend_callback);

	void pause(bool const set);
	bool is_paused() const;

	void set_current_volume(long const new_volume);
	void set_current_position(unsigned int const new_position);
	void issue_get_position_command();
	unsigned int get_current_position() const;

	void play(uri const &uri_);
	void stop();

	void update_module_entries();

	module_entries_t const & get_module_entries() const { return module_entries; }
	module_entries_updated_signal_t & get_module_entries_updated_signal() { return module_entries_updated_signal; }

	void update_module_properties(std::string const &module_id, Json::Value const &ui_properties);


protected:
	void parse_command(std::string const &event_command_name, params_t const &event_params);
	void parse_modules_list(params_t const &event_params);
	void read_module_ui(params_t const &event_params);


	bool paused;
	unsigned int current_position;
	module_entries_t module_entries;
	module_entries_updated_signal_t module_entries_updated_signal;
};


}
}


#endif

