#ifndef ION_BACKEND_BACKEND_HPP
#define ION_BACKEND_BACKEND_HPP

#include <boost/thread/mutex.hpp>

#include <ion/uri.hpp>
#include <ion/backend_base.hpp>
#include <ion/metadata.hpp>

#include "types.hpp"
#include "source_creator.hpp"
#include "decoder_creator.hpp"
#include "sink_creator.hpp"


namespace ion
{
namespace backend
{


class backend:
	public backend_base
{
public:
	typedef decoder_creator::creators_t decoder_creators_t;
	typedef sink_creator::creators_t    sink_creators_t;
	typedef source_creator::creators_t  source_creators_t;


	explicit backend(message_callback_t const &message_callback);
	virtual ~backend();


	// Retrieves a type identifier for this backend, this identifier being "ion_audio".
	// pre: nothing.
	// post: does not affect backend states.
	virtual std::string get_type() const { return "ion_audio"; }

	// Retrieves metadata for the given resource.
	// pre: the given uri must be valid. It does not matter if the same resource is currently being played.
	// post: does not affect backend states.
	std::string get_metadata(std::string const &uri_str);

	virtual void exec_command(std::string const &command, params_t const &params, std::string &response_command, params_t &response_params);


	decoder_creators_t & get_decoder_creators() { return decoder_creators; }
	sink_creators_t    & get_sink_creators()    { return sink_creators; }
	source_creators_t  & get_source_creators()  { return source_creators; }


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

	// Stops any current playback. See the stop command in audio.txt for details.
	// pre: A valid sink must have been set before.
	// post: If nothing is playing, this function does nothing. (Note: paused playback does not count as "no playback".)
	// If something is playing, playback will cease, and the current decoder is uninitialized. If a next resource is set, its decoder too will be uninitialized.
	void stop_playback();

	void trigger_transition();

	// Sets the loop mode, putting the response in the two response_* arguments. See the set_loop_mode command for details.
	// pre: nothing.
	// post: if an integer was given, the loop mode will be set accordingly. Otherwise, nothing is changed.
	void set_loop_mode(params_t const &params, std::string &response_command_out, params_t &response_params);

	// Generates a list of modules, putting the response in the two arguments.
	// pre: nothing.
	// post: does not affect backend states.
	void generate_modules_list(std::string &response_command_out, params_t &response_params);


	void create_sink(std::string const &type);


protected:
	void set_next_decoder(std::string const &uri_str, std::string const &decoder_type, metadata_t const &metadata);
	source_ptr_t create_new_source(ion::uri const &uri_);
	decoder_ptr_t create_new_decoder(std::string const &uri_str, std::string const &decoder_type, metadata_t const &metadata);
	void resource_finished_callback();

	metadata_t checked_parse_metadata(std::string const &metadata_str, std::string const &error_msg);


	message_callback_t message_callback;

	decoder_creators_t decoder_creators;
	sink_creators_t    sink_creators;
	source_creators_t  source_creators;

	decoder_ptr_t current_decoder, next_decoder;
	sink_ptr_t current_sink;

	boost::mutex decoder_mutex;

	int loop_count;
};


}
}


#endif

