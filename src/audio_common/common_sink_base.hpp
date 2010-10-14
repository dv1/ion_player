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


#ifndef ION_AUDIO_COMMON_COMMON_SINK_BASE_HPP
#define ION_AUDIO_COMMON_COMMON_SINK_BASE_HPP

#include <assert.h>
#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include "convert.hpp"
#include "resampler.hpp"
#include "sink.hpp"
#include "sink_creator.hpp"


namespace ion
{
namespace audio_common
{


/*
Base code for sinks, using CRTP. Most sinks should use this and implement only a few necessary member functions:

-bool is_initialized() const
Returns true if the audio device is initialized properly, false otherwise

-bool initialize_audio_device(unsigned int const playback_frequency)
Initializes the audio device, setting its playback frequency to the given one
The device is free to choose a different frequency if the given one cannot be used. Also, this function fills the playback_properties_ contents.
If the device is already initialized, this function does nothing.

-bool reinitialize_audio_device(unsigned int const new_playback_frequency)
Similar to initialize_audio_device(), it differs in that if the device is already initialized, it reinitializes it.

-void shutdown_audio_device()
Shuts down the device. If the device is shut down already, this function does nothing.

-bool render_samples(unsigned int const num_samples_to_render)
Plays the given number of samples. The number of channels does NOT affect this value. For example, at 48000 Hz playback frequency, 12000 samples
give data for 0.25 seconds of playback, regardless of whether or not this is mono or stereo playback.

-unsigned int get_default_playback_frequency() const
A device has a default playback frequency, which usually is 48 kHz. The default frequency
must be one the device can playback directly, meaning that passing this frequency to initialize_audio_device() or reinitialize_audio_device() would
never cause the device to choose a different one.

-std::size_t get_sample_buffer_size() const
Returns the size of the sample buffer, in bytes (NOT samples). This size *is* affected by number of channels and the sample format. For example,
a buffer that has room for 512 buffers, with stereo playback and 16bit samples, has 512 * 2 (stereo) * 2 (16bit) = 2048 bytes.
This value is not required to remain the same after initialize_audio_device(), reinitialize_audio_device(), or shutdown_audio_device() calls have been made.
Also, retrieving this value before the device was initialized is undefined behavior.

-uint8_t* get_sample_buffer()
Returns the pointer to the beginning of the sample buffer. This pointer is then used to fill data into said buffer.
This value is not required to remain the same after initialize_audio_device(), reinitialize_audio_device(), or shutdown_audio_device() calls have been made.
Also, retrieving this value before the device was initialized is undefined behavior.



IMPORTANT: initialize_audio_device(), reinitialize_audio_device(), and render_samples() must be synchronized by the derived class!


PAUSE/RESUME:

The device itself is not really paused, since many audio devices do not have this functionality (and not playing anything results in buffer underruns).
Instead, the sample buffer is filled with zeros and played.


SMART POINTER USAGE IN CODE:

This code makes extensive use of smart pointers. The reason for this is that the assignments current_song_decoder = next_song_decoder;
and next_song_decoder = null; then automatically deallocate songs that are no longer necessary. Of course this means that the decoder
must not spend a lot of time in its destructor.


ABOUT THE DIFFERENT FREQUENCIES:

The playback frequency is an issue that can get complicated. There are several frequencies involved: requested and actual playback frequency,
default sink frequency, and the frequency from the decoder's properties. In addition, it is preferable not to have to resample the audio data, since this needs extra processing power
and potentially degrades audio quality.

The policies for dealing with this situation are defined on whether or not reinitialization-on-demand is used.
With reinitialization-on-demand, the policy is:

1. In the device initialization (that is, a start() call after either stop() was called or the system just started up),
   try to initialize the device using the decoder's samplerate as playback frequency
2. The device's actual frequency (which is known after successful initialization) is stored in the playback_properties_ structure
3. During playback, resample if necessary
4. If a transition happens, check if a device reinitialization is necessary; if so, reinitialize the device with the next decoder's samplerate
   the check is performed this way:
   4.1. for each decoder, get its frequency
   4.2. compare these frequencies; if they match, no device reinitialization is necessary
   4.3. otherwise, a second check needs to be done: compare the next decoder's frequency (treating a rate of 0 like in 4.1) with the current playback frequency;
        if they do not match, a reinitialization is neccesary (using the next decoder's frequency), otherwise do not reinitialize
5. If a start() call is done, and playback is running, do a check similar to the one in 4.:
   4.1. Get the current decoder's playback frequency (treating a rate of 0 like in 4.1)
   4.2. Get the new decoder's (the one that is passed to start() as parameter) frequency (treating a rate of 0 like in 4.1)
   4.3. If these frequencies match, no reinitialization is necessary, otherwise perform one, using the new decoder's frequency for the reinitialization

(this one will be called the "reinitialization policy" from here on)

without reinitialization-on-demand, the policy is:

1. In the device initialization (that is, a start() call after either stop() was called or the system just started up),
   try to initialize the device using the sink's default playback frequency as playback frequency
2. The device's actual frequency (which is known after successful initialization) is stored in the playback_properties_ structure
3. During playback, resample if necessary

The second policy is clearly simpler and more robust, since the device has to be initialized only once (shutdown due to a stop() call or a playback finish have been
omitted, for sake of clarity). It also guarantees gapless playback regardless of decoder frequency. On the other hand, the reinitialization policy tries to get
the device to operate at the decoder's frequency as much as possible, thereby reducing the amount of unavoidable resampling work. Also, while gaps are introduced, they
do not happen between decoders with the same frequency. It can be argued that gapless playback is only really interesting for tracks of the same album, and these are
typically sampled with identical rates.
Decoders with no frequency of their own can adapt to any frequency, they just need to be told which one. These decoders return 0 for the frequency in the decoder
properties structure. The reinitialization policy uses this to further optimize playback; usually, the default sink playback frequency is one the audio device
directly supports, so when (re)initializing the device with this frequency, the actual playback frequency will match this one. Therefore, by choosing this frequency
for decoders with no frequency of their own, the probability for resampling to become necessary is greatly diminished.


POSSIBLE IMPROVEMENTS:

An improvement would be to check if a decoder will be set to null, and if so, stuff it in a "deallocation queue". So, before setting
current_song_decoder = next_song_decoder, check if next_song_decoder is null. If so, push current_song_decoder in a deallocation queue.
This queue will house smart pointers, so pushing it will keep the decoder alive. Then, a *different* thread can get a notification that
something was put into the queue, and pop it until its empty, dropping the smart pointers in the process. This avoids race conditions,
and makes it unnecessary to require fast decoder shutdowns. (The queue can be implemented with an STL deque for storage and a thread
condition variable for notification.)
*/

template < typename Derived >
class common_sink_base:
	public sink
{
public:
	typedef Derived derived_t;
	typedef common_sink_base < Derived > self_t;


	~common_sink_base()
	{
		// Stop is not called here; the backend does this already. Calling stop in the sink could lead to unexpected results.
		get_derived().shutdown_audio_device();
	}	


	virtual void start(decoder_ptr_t decoder_, decoder_ptr_t next_decoder_)
	{
		assert(decoder_); // No decoder -> error

		if (run_playback_loop)
		{
			// Playback is already running, the device does not have to be initialized - pause it
			pause(false);
		}
		else
		{
			// Playback is not running, the device must be initialized first; also, the resampler must be created
			if (!get_derived().initialize_audio_device(get_playback_frequency(*decoder_)))
			{
				send_event_callback("error", boost::assign::list_of("initializing the audio device failed -> not playing"));
				return;
			}
		}

		std::string current_decoder_uri_string, next_decoder_uri_string;

		// At this point, the device is initialized (and possibly in a paused state). This scope must still be synchronized, however,
		// since the playback loop thread may take a short while before it is paused. Without synchronization, this would cause a race condition.
		{
			boost::lock_guard < boost::mutex > lock(mutex);


			// Reset resampler, to flush any internal buffers it might have (otherwise short "clicks" may be heard)
			speex_resampler_.reset();

			// Reinitialization has been marked as necessary, and playback is running -> reinitialize device
			// (reinitializing makes no sense if no playback is running yet)
			if (reinitialize_on_demand && run_playback_loop)
			{
				/*
				In here, two tests are performed to see if reinitialization is *really* necessary
				Only if both tests are positive, a reinitialization is performed
				*/

				unsigned int new_frequency = get_playback_frequency(*decoder_);

				// First test - see if the new decoder's frequency differs from the current playback frequency
				bool do_reinitialize = (new_frequency != playback_properties_.frequency);

				if (do_reinitialize && current_decoder)
				{
					// If the first test resulted positive, move to the second test:
					// compare the current decoder's frequency with the new one's
					do_reinitialize = (get_playback_frequency(*current_decoder) != new_frequency);
				}

				if (do_reinitialize)
				{
					// Both tests were positive -> a reinitialization is necessary indeed

					if (!get_derived().reinitialize_audio_device(new_frequency))
					{
						// Error while reinitializeing - stop playback and exit
						run_playback_loop = false;	
						is_paused = false;
						send_event_callback("error", boost::assign::list_of("reinitializing audio device failed -> stopping playback"));
						return;
					}
				}
			}

			// Tell the decoder the playback properties. This has to be done here; it cannot happen at time of creation of the decoder,
			// since only from here on the properties are fully known
			decoder_->set_playback_properties(playback_properties_);

			if (decoder_->can_playback())
			{
				// playback can commence - set decoder_ as the new current one, and try to set the next decoder
				// also, get the current (and possibly the next) decoder uri as string here, BEFORE the mutex lock is released and playback unpaused
				// or the playback thread started
				// the reason for this is that inside the thread, these decoders may change while their URIs are retrieved -> race condition

				current_decoder = decoder_;
				current_decoder_uri_string = current_decoder->get_uri().get_full();

				if (next_decoder_)
				{
					next_decoder_->set_playback_properties(playback_properties_);
					if (next_decoder_->can_playback())
					{
						next_decoder = next_decoder_;
						next_decoder_uri_string = next_decoder->get_uri().get_full();
					}
					else
					{
						send_event_callback("error", boost::assign::list_of("given next decoder cannot playback"));
						next_decoder = decoder_ptr_t();
					}
				}
				else
					next_decoder = decoder_ptr_t();

				
			}
			else
			{
				// TODO: handle the case where the very first playback attempt fails because can_playback() returns false
				// (current_decoder will still be null, and the whole sink will be left in an undefined state)
				send_event_callback("error", boost::assign::list_of("given current decoder cannot playback"));
			}
		}

		// The decoder(s) is/are set; next step is to either resume playback, or start playback thread, depending on whether or not playback was running
		// when start() was called
		if (run_playback_loop)
			resume(false);
		else
		{
			is_paused = false;
			run_playback_loop = true;
			playback_thread = boost::thread(boost::phoenix::bind(&self_t::playback_loop, this));
		}

		// Notify about the start
		send_event_callback("started", boost::assign::list_of(current_decoder_uri_string)(next_decoder_uri_string));
	}


	virtual void stop(bool const do_notify)
	{
		if (!get_derived().is_initialized()) // Ignore calls when no playback is running
			return;

		{
			// Setting run_playback_loop to false causes the while loop in the playback thread to exit
			boost::lock_guard < boost::mutex > lock(mutex);
			run_playback_loop = false;
		}

		playback_thread.join(); // Wait for the playback thread to finish

		// At this point, the playback thread is shut down; race conditions are not a concern from here on
		// (This is why the mutex lock is not applied here)

		is_paused = false; // reset the pause state

		get_derived().shutdown_audio_device(); // perform device shutdown

		if (do_notify) // Send a stopped event if it is desired
			send_event_callback("stopped", boost::assign::list_of(current_decoder->get_uri().get_full()));

		// Finally, reset current and next decoder; any set decoder will be destroyed by shared_ptr
		current_decoder = decoder_ptr_t();
		next_decoder = decoder_ptr_t();
	}


	virtual void pause(bool const do_notify)
	{
		if (!get_derived().is_initialized() || !current_decoder) // No playback or no current decoder set? -> Do nothing, and exit
			return;

		// Set the pause flag - this needs to be synchronized, since the playback thread looks at this flag
		{
			boost::lock_guard < boost::mutex > lock(mutex);
			if (is_paused) // if the sink is already paused, do nothing and exit
				return;
			is_paused = true;

			// Send a notification if desired
			// Doing this inside the synchronized scope, to avoid secondary race conditions between this event and events sent by the playback thread
			if (do_notify)
				send_event_callback("paused", params_t());
		}

		// Tell the decoder it can pause any activity of its own
		current_decoder->pause();
	}


	virtual void resume(bool const do_notify)
	{
		if (!get_derived().is_initialized() || !current_decoder) // No playback or no current decoder set? -> Do nothing, and exit
			return;
			
		{
			boost::lock_guard < boost::mutex > lock(mutex);
			if (!is_paused) // if the sink is not paused, do nothing and exit
				return;
			is_paused = false;

			// Send a notification if desired
			// Doing this inside the synchronized scope, to avoid secondary race conditions between this event and events sent by the playback thread
			if (do_notify)
				send_event_callback("resumed", params_t());
		}

		// Tell the decoder it shall resume any previously paused activity
		current_decoder->resume();
	}


	virtual void clear_next_decoder()
	{
		if (!get_derived().is_initialized()) // No playback? -> Do nothing, and exit
			return;

		boost::lock_guard < boost::mutex > lock(mutex);
		// Clear the currently set next decoder
		// Using synchronization, since the playback thread accesses the next decoder
		next_decoder = decoder_ptr_t(); // TODO: see deferred shutdown note at the beginning of this file for details
	}


	virtual void set_next_decoder(decoder_ptr_t next_decoder_)
	{
		// If no playback is running, no current decoder is set, or the supplied next decoder is null, do nothing and exit
		if (!get_derived().is_initialized() || !current_decoder || !next_decoder_)
			return;

		// Tell the new next decoder the sink's playback properties
		next_decoder_->set_playback_properties(playback_properties_);

		if (!next_decoder_->can_playback()) // If the new next decoder cannot playback, it is useless for this sink -> report and error and exit
		{
			send_event_callback("error", boost::assign::list_of("given next decoder cannot playback"));
			return;
		}

		// New next decoder is OK, replace the currently set the next decoder with this one
		// Using synchronization, since the playback thread accesses the next decoder
		boost::lock_guard < boost::mutex > lock(mutex);
		next_decoder = next_decoder_;
	}


protected:
	// Convenience CRTP related calls
	derived_t& get_derived() { return *(static_cast < derived_t* > (this)); }
	derived_t const & get_derived() const { return *(static_cast < derived_t const * > (this)); }


	/**
	* Constructor. Accepts a send event callback and a boolean flag determining whether or not to use reinitialization (see above for an explanation).
	*
	* @param send_event_callback The send event callback to use; must be valid and non-null
	* @param initialize_on_demand true if the reinitialization policy shall be used, otherwise false (the other will be used then)
	* @pre The callback must be valid
	* @post The common sink base class will be initialized with the reinitialization policy in effect or not, depending on initialize_on_demand
	*/
	explicit common_sink_base(send_event_callback_t const &send_event_callback, bool const initialize_on_demand):
		sink(send_event_callback),
		run_playback_loop(false),
		is_paused(false),
		reinitialize_on_demand(initialize_on_demand),
		speex_resampler_(5), // The 5 is a quality setting; 0 is worst, 10 is best; better quality requires more computations at run-time
		convert_(speex_resampler_)
	{
	}


	/**
	* Helper function to get a frequency for the given decoder. If reinitialize on demand is used, the decoder's frequency is returned,
	* otherwise, the sink's default playback frequency will be used.
	*
	* @param decoder_ The decoder whose frequency is to be used (if its nonzero)
	* @return A non-zero frequency for playback; if the decoder frequency is nonzero, this will be the frequency, otherwise it will be the sink's
	* default playback one
	*/
	unsigned int get_playback_frequency(decoder &decoder_) const
	{
		unsigned int frequency = 0;

		if (reinitialize_on_demand) // first, get the decoder's frequency
			frequency = decoder_.get_decoder_properties().frequency;

		if (frequency == 0) // if the frequency is zero, it means the decoder can adapt to any frequency -> use the sink's default playback one
			frequency = get_derived().get_default_playback_frequency();

		return frequency;
	}


	// Playback thread loop function.
	void playback_loop()
	{
		bool do_shutdown = false;
		bool restart_device = false;

		// actual loop
		while (true)
		{
			unsigned int num_samples_written;

			// First part of the loop: prepare data to be played
			// This part is synchronized, since outside operations may affect its behavior, introducing race conditions otherwise
			{
				boost::lock_guard < boost::mutex > lock(mutex);

				if (do_shutdown)
				{	
					// An earlier loop iteration requested a shutdown -> shutdown audio device and exit loop
					run_playback_loop = false;
					is_paused = false;
					get_derived().shutdown_audio_device();
					return;
				}

				if (restart_device)
				{
					// An earlier iteration of the loop requested a device restart
					// If a current decoder is set, do the restart (= reinitialize the device), otherwise just set restart_device to false
					if (current_decoder)
					{
						if (!get_derived().reinitialize_audio_device(get_playback_frequency(*current_decoder)))
						{
							// Reinitialization failed -> shut down playback loop
							run_playback_loop = false;
							is_paused = false;
							send_event_callback("error", boost::assign::list_of("reinitializing audio device failed -> stopping playback"));
							return; // reinitialization failed -> shut down playback
						}

						// tell the current decoder about the new playback properties
						current_decoder->set_playback_properties(playback_properties_);
					}

					restart_device = false;
				}

				// Either something inside this loop or a call outside the playback thread requested for the playback loop to exit
				// (therefore ending the playback thread) -> do so
				if (!run_playback_loop)
					return;

				unsigned int num_samples_to_write = playback_properties_.num_buffer_samples;


				// This scope fills the sample buffer, which will be played back
				// If the sink is paused, do not fill the sample buffer with decoder data
				if (!is_paused && current_decoder)
				{
					// Multiplier is used for getting byte counts out of sample counts
					unsigned int multiplier = playback_properties_.num_channels * get_sample_size(playback_properties_.sample_type_);
					// This catches miscalculations that would otherwise lead to buffer overflows
					assert(num_samples_to_write * multiplier <= get_derived().get_sample_buffer_size());

					// Retrieve, and if necessary, convert/resample data from the current decoder. The output is written into the sink's sample buffer.
					// The converter returns the number of actually written samples.
					num_samples_written = convert_(
						*current_decoder, current_decoder->get_decoder_properties(),
						&(get_derived().get_sample_buffer()[0]), num_samples_to_write, playback_properties_,
						255, 255
					);

					// No samples were written -> move to the next song, so that the next loop iteration uses that next one
					// If no next song exists, it means we are done playing, shut down in this case
					// (A count of zero means by definition "this decoder is done decoding, and will not write any more samples")
					if (num_samples_written == 0)
					{
						std::string current_uri = current_decoder->get_uri().get_full();
						std::string next_uri;

						// There is a next decoder set -> get its URI (for sending a transition/resource_finished event), and determine if the next playback
						// loop iteration should restart the device
						if (next_decoder)
						{
							next_uri = next_decoder->get_uri().get_full();

							if (reinitialize_on_demand)
							{
								unsigned int current_actual_frequency = get_playback_frequency(*current_decoder);
								unsigned int next_actual_frequency = get_playback_frequency(*next_decoder);
								restart_device =
									(current_actual_frequency != next_actual_frequency) &&
									(playback_properties_.frequency != next_actual_frequency);
							}
						}

						// if (next_song_decoder == null) deallocation_queue.push(current_song_decoder); // TODO: see deferred shutdown note at the beginning of this file for details

						// Do the next->current handover; current decoder is destroyed, next decoder becomes current one
						current_decoder = next_decoder;
						next_decoder = decoder_ptr_t();

						// Call the resource_finished_callback if one was set before playback started
						if (resource_finished_callback)
							resource_finished_callback();

						// Send a notification; if a next decoder was set, send "transition", otherwise send "resource_finished"
						// (This way, frontends know whether or not playback stopped)
						if (!next_uri.empty())
							send_event_callback("transition", boost::assign::list_of(current_uri)(next_uri));
						else
							send_event_callback("resource_finished", boost::assign::list_of(current_uri));

						// If current_decoder is null, it means the next decoder wasn't set, and the handover above essentially set both decoder shared pointers to null
						// -> there is nothing more to play; set do_shutdown to true so the next playback loop iteration performs a shutdown and exits
						if (!current_decoder)
							do_shutdown = true;
					}

					assert(num_samples_written <= playback_properties_.num_buffer_samples);
				}

				if (is_paused)
					std::memset(&(get_derived().get_sample_buffer()[0]), 0, get_derived().get_sample_buffer_size());
			}

			// Second part of the loop: playback the prepared data (if there is any)
			if (num_samples_written > 0)
			{
				if (!get_derived().render_samples(num_samples_written))
				{
					// if a fatal error happened, end the playback loop
					send_event_callback("error", boost::assign::list_of("error while rendering samples -> stopping playback"));
					do_shutdown = true;
				}
			}
		}
	}



	decoder_ptr_t current_decoder, next_decoder;
	playback_properties playback_properties_;
	bool run_playback_loop, is_paused, reinitialize_on_demand;
	boost::thread playback_thread;
	boost::mutex mutex;
	speex_resampler::speex_resampler speex_resampler_;
	convert < speex_resampler::speex_resampler > convert_;
};


}
}


#endif

