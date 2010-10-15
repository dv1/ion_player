/****************************************************************************

Copyright (c) 2010 Carlos Rafael Giani

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

****************************************************************************/


#ifndef ION_AUDIO_COMMON_BACKEND_SINK_HPP
#define ION_AUDIO_COMMON_BACKEND_SINK_HPP

#include <assert.h>
#include <iostream>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "send_event_callback.hpp"
#include "decoder.hpp"


namespace ion
{
namespace audio_common
{


class sink:
	private boost::noncopyable
{
public:
	typedef boost::function < void() > resource_finished_callback_t;


	// Minimum and maximum volume constants; deliberately not using integers here, to help platforms where floating point is expensive
	inline static long min_volume() { return 0; }
	inline static long max_volume() { return 16777215; }


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
	* @param do_notify If set to true, this function will use the send event callback to send a response that notifies about the stop
	* @pre the sink must be operational
	* @post current and next decoder will be reset (e.g. set to decoder_ptr_t(), which equals a null pointer). This may trigger a decoder shutdown if no one else had a shared pointer
	* to these decoders. If no playback was running, this function does nothing. Paused playback will be stopped as well.
	*/
	virtual void stop(bool const do_notify = true) = 0;

	/**
	* Pauses playback. Repeated calls will be ignored. This function does NOT have to uninitialize anything internally, and is in fact preferred to not do so.
	*
	* @param do_notify If set to true, this function will use the send event callback to send a response that notifies about the pause
	* @pre Playback must be present (e.g. start() must have been called earlier) and unpaused; the sink must be operational
	* @post The playback will be paused if the preconditions were met, otherwise this function does nothing
	*/
	virtual void pause(bool const do_notify = true) = 0;

	/**
	* Resumes playback. Repeated calls will be ignored. This function does NOT have to reinitialize anything internally, and is in fact preferred to not do so.
	*
	* @param do_notify If set to true, this function will use the send event callback to send a response that notifies about the resume
	* @pre Playback must be present (e.g. start() must have been called earlier) and paused; the sink must be operational
	* @post The playback will be resumed if the preconditions were met, otherwise this function does nothing
	*/
	virtual void resume(bool const do_notify = true) = 0;

	/**
	* Sets the current volume to the one specified. Valid range is 0 (silence) to 16777215 (full volume). The sink does _not_ have to guarantee that the current volume
	* will exactly match the given one after this call is done (this may not be possible, depending on the sink implementation, sometimes one can only set the volume with coarse granularity).
	* The return value will be the new current volume, which as described may differ from new_volume.
	*
	* @param new_volume The new volume; valid range = 0 (silence) to 16777215 (full volume)
	* @return The new current volume
	* @pre new_volume must be within the valid range; the sink must be correctly initialized
	* @post The current volume will have been changed if the precondition was met; if not, this call will be ignored
	*/
	virtual long set_current_volume(long const new_volume) = 0;

	/**
	* Returns the current volume. The valid range goes from 0 (silence) to 16777215 (full volume).
	* @return The current volume
	*/
	virtual long get_current_volume() const = 0;

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

	/**
	* Clears any set next decoder. If no next decoder is set, or no playback is running, this function does nothing.
	* @pre The sink must be operational
	* @post The next decoder will be cleared. If no playback is running, or no next decoder was previously set, this function does nothing.
	* @
	*/
	virtual void clear_next_decoder() = 0;

	/**
	* Sets the song finished callback.
	* CAUTION: do NOT set this during playback, otherwise race conditions may occur!
	* It is valid to pass on an invalid callback (that is, the value of resource_finished_callback_t(), which is an "empty" callback). This tells the sink to not trigger this callback.
	*
	* @param new_resource_finished_callback The callback to be used
	* @pre Playback must not not running
	* @post The song finished callback will be new_resource_finished_callback
	*/
	inline void set_resource_finished_callback(resource_finished_callback_t const &new_resource_finished_callback)
	{
		resource_finished_callback = new_resource_finished_callback;
	}


protected:
	// Base constructor. Accepts a send event callback that is used to send events back to the frontend.
	// @param send_event_callback The send event callback to use; must be valid and non-null
	// @pre The callback must be valid
	// @post The sink base class will be initialized
	explicit sink(send_event_callback_t const &send_event_callback):
		send_event_callback(send_event_callback)
	{
		assert(send_event_callback);
	}


	send_event_callback_t send_event_callback;
	resource_finished_callback_t resource_finished_callback;
};


typedef boost::shared_ptr < sink > sink_ptr_t;


}
}


#endif

