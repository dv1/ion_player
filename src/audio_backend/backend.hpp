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


#ifndef ION_AUDIO_BACKEND_BACKEND_HPP
#define ION_AUDIO_BACKEND_BACKEND_HPP

#include <boost/thread/mutex.hpp>

#include <ion/uri.hpp>
#include <ion/metadata.hpp>

#include "types.hpp"
#include "source_creator.hpp"
#include "decoder_creator.hpp"
#include "sink_creator.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


class backend
{
public:
	typedef component_creators < decoder_creator > decoder_creators_t;
	typedef component_creators < sink_creator >    sink_creators_t;
	typedef component_creators < source_creator >  source_creators_t;


	explicit backend();
	explicit backend(send_event_callback_t const &send_event_callback);
	~backend();


	void set_send_event_callback(send_event_callback_t const &new_send_event_callback);


	// Retrieves a type identifier for this backend, this identifier being "ion_audio".
	// pre: nothing.
	// post: does not affect backend states.
	std::string get_type() const { return "ion_audio"; }

	// Retrieves metadata for the given resource.
	// pre: the given uri must be valid. It does not matter if the same resource is currently being played.
	// post: does not affect backend states.
	metadata_t get_metadata(std::string const &uri_str);

	// Convenience function to retrieve metadata in string form.
	// pre: the given uri must be valid. It does not matter if the same resource is currently being played.
	// post: does not affect backend states.
	std::string get_metadata_as_string(std::string const &uri_str);

	void exec_command(std::string const &command, params_t const &params, std::string &response_command, params_t &response_params);


	decoder_creators_t::creators_t & get_decoder_creators() { return decoder_creators; }
	sink_creators_t::creators_t    & get_sink_creators()    { return sink_creators; }
	source_creators_t::creators_t  & get_source_creators()  { return source_creators; }


	// Starts playback with the given params. See the play command in audio.txt for details.
	// Internally, a decoder is created for the given uri. (If a next uri is given, it will also get a decoder; this facilitates preload and subsequently gapless playback.)
	// pre: Params must be valid. Invalid params cause an exception to be thrown. A valid sink must have been set before.
	// post: If the given first uri is valid, its playback will commence immediately. Any current playback will cease.
	// Note that no notification will be sent in this case.
	// If the first uri is invalid, this function does not alter any internal states.
	// If a second uri is given, and it is found valid, it will be the new next uri. If it is found invalid, or if it is not given,
	// any previously set next resource will be removed.
	void start_playback(params_t const &params);

	// Sets the next resource, replacing any previously set resource. See the set_next_resource command in audio.txt for details.
	// pre: Params must be valid. Invalid params cause an exception to be thrown. A valid sink must have been set before.
	// post: if nothing is playing at time of this call, this function will call start_playback(), passing its params, and then exit.
	// (Note: paused playback does not count as "no playback".)
	// If something is already playing, it will set a new next song if the given uri is found to be valid. If the given uri is invalid,
	// this function does not set anything.
	void set_next_resource(params_t const &params);

	// Clears the next resource. If any playback is happening, its ending will not trigger a transition anymore; instead, resource_finished will be triggered,
	// just as if the play command were invoked without a next uri set.
	// pre: A valid sink must have been set before.
	// post: if a next resource was set, it will be no more.
	void clear_next_resource();

	// Stops any current playback. See the stop command in audio.txt for details.
	// pre: A valid sink must have been set before.
	// post: If nothing is playing, this function does nothing. (Note: paused playback does not count as "no playback".)
	// If something is playing, playback will cease, and the current decoder is uninitialized. If a next resource is set, its decoder too will be uninitialized.
	void stop_playback();

	// Generates a list of modules, putting the response in the two arguments.
	// pre: nothing.
	// post: does not affect backend states.
	void generate_modules_list(std::string &response_command_out, params_t &response_params);


	void create_sink(std::string const &type);


protected:
	void set_next_decoder(std::string const &uri_str, std::string const &decoder_type, metadata_t const &metadata);
	source_ptr_t create_new_source(ion::uri const &uri_);
	decoder_ptr_t create_new_decoder(ion::uri const &uri_, std::string const &decoder_type, metadata_t const &metadata);
	void resource_finished_callback();
	metadata_t update_metadata(ion::uri const &uri_, metadata_t const &metadata_updates);
	metadata_t update_metadata_impl(decoder &decoder_, metadata_t const &metadata_updates);

	metadata_t checked_parse_metadata(std::string const &metadata_str, std::string const &error_msg);


	send_event_callback_t send_event_callback;

	decoder_creators_t::creators_t decoder_creators;
	sink_creators_t::creators_t    sink_creators;
	source_creators_t::creators_t  source_creators;

	decoder_ptr_t current_decoder, next_decoder;
	sink_ptr_t current_sink;

	boost::mutex decoder_mutex;
};



void set_send_event_callback(backend &backend_, send_event_callback_t const &new_send_event_callback);
std::string get_backend_type(backend const &backend_);
void execute_command(backend &backend_, std::string const &command, params_t const &parameters, std::string &response_command, params_t &response_parameters);


}
}


#endif

