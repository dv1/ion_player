#ifndef ION_AUDIO_BACKEND_BACKEND_SINK_HPP
#define ION_AUDIO_BACKEND_BACKEND_SINK_HPP

#include <iostream>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <ion/send_command_callback.hpp>

#include "decoder.hpp"


namespace ion
{
namespace audio_backend
{


class sink:
	private boost::noncopyable
{
public:
	enum command_type
	{
		resource_finished, // sent when the decoder signalizes and end-of-data via a "false" returnvalue from decoder::update(); the only way to get playback again is to call start()
		stopped, // sent when the playback is stopped by other means (by calling stop() for instance); the only way to get playback again is to call start()
		started, // start() was called, playback started
		paused, // pause() was called, playback pauses
		resumed, // resume() was called, playback resumes
		transition // like resource_finished, except that playback doesn't stop, since there is a next song that automatically gets promoted to the current song
	};

	typedef boost::function < void() > resource_finished_callback_t;


	virtual ~sink()
	{
	}


	/**
	* Starts playback. decoder_ becomes the current decoder, next_decoder_ the next decoder. The current decoder will be used for playback immedatiely. If a next decoder is given, it
	* will be used for playback once the current decoder reports end of resource (= update() starts returning 0). The current decoder will then be nulled, and the next decoder then becomes
	* the current one. The sink guarantees a gapless transition.
	* This function implicitely resumes playback; if pause() was called before start(), it is not necessary to call resume() afterwards.
	* If start() was already called earlier, current and next decoder will be set to the new values, and playback will continue. It is not necessarily to call stop() before calling start()
	* again; in fact, it is recommended not to do so. Gapless transitions between start() calls are not guaranteed, but invoking stop() between them is likely to enlarge this gap.
	* Furthermore, stop() uninitializes the sink (that is, any internal handles/devices), while subsequent start() calls can reuse previous initializations.
	*
	* If a decoder signals a song end during playback, and no next decoder is around to take over, the sink calls the song finished callback.
	*
	* @param decoder_ the decoder to be used for immediate playback; will become the current decoder
	* @param next_decoder_ the decoder to be used after the current one finished; will become the next decoder; it can be set to a null pointer, no next decoder will be set then
	* @pre decoder_ must be valid; the sink must be operational
	* @post if successful, and if the sink was stopped before the start() call (or if this is the first start() call), playback will commence, decoder_ becomes the current decoder, next_decoder_ becomes the next one.
	  If successful, and start() was called before (= playback is already running), current and next decoder will be set, playback will continue.
	  (NOTE: a paused playback still counts as a running one. Running does not imply unpaused.)
	  If this call fails, nothing gets changed; no playback is started, and any previously started playback will be left undisturbed.
	*/
	virtual void start(decoder_ptr_t decoder_, decoder_ptr_t next_decoder_) = 0;

	/**
	* Stops any running playback. If nothing is being played, this call does nothing.
	* (NOTE: a paused playback still counts as a running one. Running does not imply unpaused.)
	* This call does _not_ affect the decoder in any way; it does not change position, volume, it does not even pause it.
	* The backend requires this for internal sink handovers, in case the user wants to change the sink while playback is running.
	*
	* @param do_notify If set to true, this function will use the send command callback to send a response that notifies about the stop
	* @pre the sink must be operational
	* @post current and next decoder will be reset (e.g. set to decoder_ptr_t(), which equals a null pointer). This may trigger a decoder shutdown if no one else had a shared pointer
	* to these decoders. If no playback was running, this function does nothing. Paused playback will be stopped as well.
	*/
	virtual void stop(bool const do_notify = true) = 0;

	/**
	* Pauses playback. Repeated calls will be ignored. This function does NOT have to uninitialize anything internally, and is in fact preferred to not do so.
	*
	* @param do_notify If set to true, this function will use the send command callback to send a response that notifies about the pause
	* @pre Playback must be present (e.g. start() must have been called earlier) and unpaused; the sink must be operational
	* @post The playback will be paused if the preconditions were met, otherwise this function does nothing
	*/
	virtual void pause(bool const do_notify = true) = 0;

	/**
	* Resumes playback. Repeated calls will be ignored. This function does NOT have to reinitialize anything internally, and is in fact preferred to not do so.
	*
	* @param do_notify If set to true, this function will use the send command callback to send a response that notifies about the resume
	* @pre Playback must be present (e.g. start() must have been called earlier) and paused; the sink must be operational
	* @post The playback will be resumed if the preconditions were met, otherwise this function does nothing
	*/
	virtual void resume(bool const do_notify = true) = 0;

	/**
	* Sets the next decoder. The next decoder will be used once the current one reports a song finish (= decoder::update() starts returning 0). If a next decoder was already set, it will be replaced with the given one.
	* If no playback is running, this function does nothing.
	*
	* This function is typically used in the song finished callback, to set the next decoder after a next->current transition occurred.
	*
	* @param next_decoder_ The decoder to be set as the new next decoder
	* @pre next_decoder_ must be valid; the sink must be operational
	* @post The next decoder will be set to the given one. If no playback is running, this function does nothing.
	*/
	virtual void set_next_decoder(decoder_ptr_t next_decoder_) = 0;

	virtual void clear_next_decoder() = 0;

	/**
	* Sets the song finished callback.
	* CAUTION: do NOT set this during playback, otherwise race conditions may occur!
	* It is valid to pass on an invalid callback (that is, the value of resource_finished_callback_t(), which is an "empty" callback). This tells the sink to not trigger this callback.
	*
	* @param new_resource_finished_callback The callback to be used
	* @post The song finished callback will be new_resource_finished_callback
	*/
	inline void set_resource_finished_callback(resource_finished_callback_t const &new_resource_finished_callback)
	{
		resource_finished_callback = new_resource_finished_callback;
	}


protected:
	explicit sink(send_command_callback_t const &send_command_callback):
		send_command_callback(send_command_callback)
	{
	}


	// Convenience function to define and send common response event commands
	void send_event_command(command_type const command, params_t const &custom_params = params_t())
	{
		if (!send_command_callback)
		{
			std::cerr << "Trying to send command \"" << command << "\" without a callback" << std::endl;
			return;
		}

		switch (command)
		{
			case resource_finished: send_command_callback("resource_finished", custom_params); break;
			case stopped: send_command_callback("stopped", custom_params); break;
			case started: send_command_callback("started", custom_params); break;
			case paused: send_command_callback("paused", custom_params); break;
			case resumed: send_command_callback("resumed", custom_params); break;
			case transition: send_command_callback("transition", custom_params); break;
			default: break;
		}
	}


	send_command_callback_t send_command_callback;
	resource_finished_callback_t resource_finished_callback;
};


typedef boost::shared_ptr < sink > sink_ptr_t;


}
}


#endif

