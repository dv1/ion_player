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


#include <assert.h>
#include <fstream>
#include <stdint.h>
#include <vector>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "uade_decoder.hpp"

extern "C"
{

#include "uadeconstants.h"
#include "uadecontrol.h"
#include "uadeconfig.h"
#include "uadeconf.h"
#include "uadestate.h"
#include "amigafilter.h"
#include "songdb.h"
#include "songinfo.h"

}


// TODO: review & cleanup this code; especially check for correct behavior in S and R states


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


struct uade_decoder::internal_data
{
	struct uade_state state;
	char uadeconf_name[PATH_MAX];
	std::string config_name, player_name, score_name, uade_name;
	std::string module_name, song_name;
	std::string title;
	bool uadeconf_loaded, spawned, file_loaded, subsong_ended, subsong_info_read, manual_songend;

	struct uade_ipc *ipc;
	struct uade_song *us;
	struct uade_effect *ue;
	struct uade_config *uc;

	uint8_t sample_data[UADE_MAX_MESSAGE_SIZE];
	uint8_t space[UADE_MAX_MESSAGE_SIZE];
	struct uade_msg *um;

	enum uade_control_state control_state;
	int const framesize;
	uint16_t *sm;
	int left;
	int what_was_left;
	int tailbytes;
	int playbytes;
	int64_t subsong_bytes;

	typedef std::vector < int16_t > sample_buffer_t;
	sample_buffer_t sample_buffer;



	explicit internal_data():
		spawned(false),
		file_loaded(false),
		control_state(UADE_S_STATE),
		framesize(UADE_BYTES_PER_SAMPLE * UADE_CHANNELS),
		left(0),
		what_was_left(0),
		tailbytes(0),
		subsong_bytes(0)
	{
		playbytes = 0;
		manual_songend = false;
		subsong_ended = false;
		subsong_info_read = false;
		ipc = &state.ipc;
		ue = &state.effects;
		uc = &state.config;
		um = (struct uade_msg*)space;	
	}

	~internal_data()
	{
		cleanup();
	}


	void spawn_core(unsigned int const frequency)
	{
		cleanup();

		std::memset(&state, 0, sizeof(state));

		uadeconf_loaded = (uade_load_initial_config(uadeconf_name, sizeof(uadeconf_name), &state.config, 0) != 0);
		config_name = std::string(state.config.basedir.name) + "/uaerc";
		score_name = std::string(state.config.basedir.name) + "/score";
		uade_name = UADE_CONFIG_UADE_CORE;

		std::cerr << "Score name: " << score_name << std::endl;
		std::cerr << "UADE name: " << uade_name << std::endl;
		std::cerr << "Config name: " << config_name << std::endl;
		state.config.filter_type = FILTER_MODEL_A500;
		state.config.filter_type_set = 1;
		state.config.no_filter = 1;
		state.config.no_filter_set = 1;
		state.config.frequency = frequency;
		state.config.frequency_set = 1;
		/*state.config.headphones = 1;
		state.config.headphones_set = 1;
		state.config.headphones2 = 1;
		state.config.headphones2_set = 1;*/

		uade_spawn(&state, uade_name.c_str(), config_name.c_str());

		spawned = true;
	}


	void unalloc_song()
	{
		if (file_loaded)
		{
			if (control_state == UADE_R_STATE)
			{
				do
				{
					uade_receive_message(um, sizeof(space), ipc);
				}
				while (um->msgtype != UADE_COMMAND_TOKEN);
			}

			uade_unalloc_song(&state);
			file_loaded = false;
		}
	}


	void cleanup()
	{
		if (spawned)
		{
			unalloc_song();
			spawned = false;
		}

		control_state = UADE_S_STATE;
		sample_buffer.clear();

		if ((state.pid != 0) && (state.pid != -1))
		{
			pid_t pid = state.pid;
			kill(pid, SIGTERM);
			waitpid(pid, 0, 0);
		}

		//um->size = 0;
		//uade_send_message(um, ipc);
	}


	void try_load_file(std::string const &filename, int const cur_subsong)
	{
		assert(spawned);
		unalloc_song();

		state.song = 0;
		state.ep = 0;

		if (!uade_is_our_file(filename.c_str(), 0, &state))
			return;

		module_name = filename;

		if (std::string(state.ep->playername) == "custom")
		{
			player_name = module_name;
			module_name = std::string();
		}
		else
		{
			player_name = std::string(state.config.basedir.name) + "/players/" + state.ep->playername;
		}

		if (player_name.empty())
			return;

		// If no modulename given, try the playername as it can be a custom song
		song_name = module_name.empty() ? player_name : module_name;

		if (!uade_alloc_song(&state, song_name.c_str()))
			return;

		/* The order of parameter processing is important:
		 * 0. set uade.conf options (done before this)
		 * 1. set eagleplayer attributes
		 * 2. set song attributes
		 * 3. set command line options
		 */

		if (state.ep != 0)
			uade_set_ep_attributes(&state);

		/* Now we have the final configuration in "uc". */
		uade_set_effects(&state);

		{
			std::ifstream f(player_name.c_str());
			if (!f.good())
				return;
		}
		
		int ret = uade_song_initialization(score_name.c_str(), player_name.c_str(), module_name.c_str(), &state);

		switch (ret)
		{
			case UADECORE_INIT_OK:
				break;

			case UADECORE_INIT_ERROR:
				uade_unalloc_song(&state);
				return;

			case UADECORE_CANT_PLAY:
				uade_unalloc_song(&state);
				return;

			default:
				return;
		}

		us = state.song;

		uade_effect_reset_internals();
		if (cur_subsong != -1)
		{
			us->cur_subsong = cur_subsong;
			uade_change_subsong(&state);
		}

		{
			char buf[256];
			uade_generate_song_title(buf, sizeof(buf), &state);
			title = buf;
		}

		while (!subsong_ended && !subsong_info_read)
		{
			if (!iterate())
			{
				uade_unalloc_song(&state);
				return;
			}
		}

		file_loaded = true;
	}


	bool iterate()
	{
		bool retval = iterate_impl();

		if (!retval && (control_state == UADE_S_STATE))
		{
			if (uade_send_short_message(UADE_COMMAND_REBOOT, ipc))
			{
				std::cerr << "Cannot send reboot" << std::endl;
				return false;
			}
		}

		return retval;
	}


	bool iterate_impl()
	{
		switch (control_state)
		{
			case UADE_S_STATE:
			{
				if (manual_songend)
					return false;

				left = uade_read_request(ipc);
				if (uade_send_short_message(UADE_COMMAND_TOKEN, ipc))
				{
					std::cerr << "Cannot send token" << std::endl;
					return false;
				}

				control_state = UADE_R_STATE;

				if (what_was_left > 0)
				{
					if (subsong_ended)
					{
						playbytes = tailbytes;
						tailbytes = 0;
					}
					else
						playbytes = what_was_left;

					us->out_bytes += playbytes;

					uade_effect_run(ue, (int16_t *)sample_data, playbytes / framesize);
					unsigned long offset = sample_buffer.size();
					sample_buffer.resize(offset + playbytes / 2);
					std::memcpy(&sample_buffer[offset], sample_data, playbytes);

					// TODO: timeout
					//if ((us->out_bytes / (UADE_BYTES_PER_FRAME * state.config.frequency)) >= 4)
					//	subsong_ended = true;

				}

				if (subsong_ended)
					return false;

				break;
			}

			case UADE_R_STATE:
				if (uade_receive_message(um, sizeof(space), ipc) <= 0)
				{
					std::cerr << "Cannot receive messages from core" << std::endl;
					return false;
				}

				switch (um->msgtype)
				{
					case UADE_COMMAND_TOKEN:
						control_state = UADE_S_STATE;
						break;

					case UADE_REPLY_DATA:
					{
						sm = (uint16_t*)(um->data);
						for (unsigned int i = 0; i < um->size; i += 2)
						{
							*sm = ntohs(*sm);
							++sm;
						}

						assert(left == int(um->size));
						assert(sizeof(sample_data) >= um->size);
						std::memcpy(sample_data, um->data, um->size);

						what_was_left = left;
						left = 0;

						break;
					}

					case UADE_REPLY_FORMATNAME:
					{
							uade_check_fix_string(um, 128);
							std::cerr << "Format name: " << ((char*)(um->data)) << std::endl;
							break;
					}

					case UADE_REPLY_MODULENAME:
					{
							uade_check_fix_string(um, 128);
							std::cerr << "Module name: " << ((char*)(um->data)) << std::endl;
							break;
					}

					case UADE_REPLY_MSG:
					{
							uade_check_fix_string(um, 128);
							std::cerr << "Message: " << ((char*)(um->data)) << std::endl;
							break;
					}
					case UADE_REPLY_PLAYERNAME:
					{
							uade_check_fix_string(um, 128);
							std::cerr << "Player name: " << ((char*)(um->data)) << std::endl;
							break;
					}

					case UADE_REPLY_SONG_END:
					{
						subsong_ended = true;
						break;
					}

					case UADE_REPLY_SUBSONG_INFO:
					{
						uint32_t *u32ptr = (uint32_t*)(um->data);
						us->min_subsong = ntohl(u32ptr[0]);
						us->max_subsong = ntohl(u32ptr[1]);
						us->cur_subsong = ntohl(u32ptr[2]);
						std::cerr << "Min subsong: " << us->min_subsong << std::endl;
						std::cerr << "Max subsong: " << us->max_subsong << std::endl;
						std::cerr << "Cur subsong: " << us->cur_subsong << std::endl;
						subsong_info_read = true;
						break;
					}
				}
				break;

			default:
				break;
		}

		return true;
	}
};


uade_decoder::uade_decoder(send_event_callback_t const send_event_callback, source_ptr_t source_, metadata_t const &initial_metadata):
	decoder(send_event_callback),
	source_(source_),
	subsong_nr(-1),
	current_position(0),
	internal_data_(0)
{
	internal_data_ = new internal_data;
	if (internal_data_ == 0)
		return;

	try
	{
		// For some reason, making options_ a reference causes segfaults in the find() call below TODO: check why this happens
		uri::options_t const options_ = source_->get_uri().get_options();
		uri::options_t::const_iterator iter = options_.find("sub_resource_index");
		if (iter != options_.end())
			subsong_nr = boost::lexical_cast < long > (iter->second);
	}
	catch (boost::bad_lexical_cast const &)
	{
	}

	internal_data_->spawn_core(48000);
	internal_data_->try_load_file(source_->get_uri().get_path(), subsong_nr);
}


uade_decoder::~uade_decoder()
{
	delete internal_data_;
}


bool uade_decoder::is_initialized() const
{
	return internal_data_->spawned && internal_data_->file_loaded;
}


bool uade_decoder::can_playback() const
{
	return is_initialized();
}


long uade_decoder::set_current_position(long const)
{
	return current_position;
}


long uade_decoder::get_current_position() const
{
	return current_position;
}


metadata_t uade_decoder::get_metadata() const
{
	metadata_t metadata_ = empty_metadata();
	if (!is_initialized())
		return metadata_;

	if (!internal_data_->title.empty())
		set_metadata_value(metadata_, "title", internal_data_->title);

	int num_subsongs = internal_data_->us->max_subsong - internal_data_->us->min_subsong + 1;
	if (num_subsongs > 1)
	{
		set_metadata_value(metadata_, "min_sub_resource_index", internal_data_->us->min_subsong);
		set_metadata_value(metadata_, "max_sub_resource_index", internal_data_->us->max_subsong);
		set_metadata_value(metadata_, "num_sub_resources", num_subsongs);
	}

	return metadata_;
}


std::string uade_decoder::get_type() const
{
	return "uade";
}


uri uade_decoder::get_uri() const
{
	return (source_) ? source_->get_uri() : uri();
}


long uade_decoder::get_num_ticks() const
{
	return 0;
}


long uade_decoder::get_num_ticks_per_second() const
{
	return 50000;
}


void uade_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	if (!is_initialized())
		return;

	boost::lock_guard < boost::mutex > lock(mutex_);

	playback_properties_ = new_playback_properties;
	internal_data_->spawn_core(playback_properties_.frequency);
	internal_data_->try_load_file(source_->get_uri().get_path(), subsong_nr);
}


decoder_properties uade_decoder::get_decoder_properties() const
{
	return decoder_properties(0, 2, audio_common::sample_s16);
}


unsigned int uade_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	boost::lock_guard < boost::mutex > lock(mutex_);

	while ((num_samples_to_write * 2) > internal_data_->sample_buffer.size())
	{
		if (!internal_data_->iterate())
		{
			internal_data_->cleanup();
			return 0;
		}
	}

	long num_samples_to_return = std::min(
		long(internal_data_->sample_buffer.size() / playback_properties_.num_channels),
		long(num_samples_to_write)
	);

	long remaining_size = long(internal_data_->sample_buffer.size()) - long(num_samples_to_return * playback_properties_.num_channels);
	std::memcpy(dest, &internal_data_->sample_buffer[0], num_samples_to_return * playback_properties_.num_channels * get_sample_size(playback_properties_.sample_type_));
	if (remaining_size > 0)
	{
		std::memmove(
			&internal_data_->sample_buffer[0],
			&internal_data_->sample_buffer[num_samples_to_write * playback_properties_.num_channels],
			remaining_size * get_sample_size(playback_properties_.sample_type_)
		);

		internal_data_->sample_buffer.resize(remaining_size);
	}
	else
		internal_data_->sample_buffer.resize(0);

	current_position += num_samples_to_return * 50000 / playback_properties_.frequency;
	return num_samples_to_return;
}




uade_decoder_creator::uade_decoder_creator()
{
}


decoder_ptr_t uade_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_event_callback_t const &send_event_callback)
{
	// UADE cannot handle any I/O other than local files
	if (source_->get_uri().get_type() != "file")
		return decoder_ptr_t();

	uade_decoder *uade_decoder_ = new uade_decoder(send_event_callback, source_, metadata);
	if (!uade_decoder_->is_initialized())
	{
		delete uade_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(uade_decoder_);
}


}
}

