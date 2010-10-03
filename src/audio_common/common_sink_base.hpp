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
#include <boost/shared_ptr.hpp>
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
A device has a default playback frequency, which usually is 48 kHz. For decoders that have no sample rate, this value is used. The default frequency
must be one the device can playback directly, meaning that passing this frequency to initialize_audio_device() or reinitialize_audio_device() would
never cause the device to choose a different one.

-std::size_t get_sample_buffer_size() const
Returns the size of the sample buffer, in bytes (NOT samples). This size *is* affected by number of channels and the sample format. For example,
a buffer that has room for 512 buffers, with stereo playback and 16bit samples, has 512 * 2 (stereo) * 2 (16bit) = 2048 bytes.
This value is not required to remain the same after initialize_audio_device(), reinitialize_audio_device(), or shutdown_audio_device() calls have been made.
Also, retrieving this value before the device was initialized is undefined behavior.

-uint8_t* get_sample_buffer()
Returns the pointer to the beginning of the sample buffer.
This value is not required to remain the same after initialize_audio_device(), reinitialize_audio_device(), or shutdown_audio_device() calls have been made.
Also, retrieving this value before the device was initialized is undefined behavior.



IMPORTANT: initialize_audio_device(), reinitialize_audio_device(), and render_samples() must be synchronized by the derived class!


This code makes extensive use of smart pointers. The reason for this is that the assignments current_song_decoder = next_song_decoder;
and next_song_decoder = null; then automatically deallocate songs that are no longer necessary. Of course this means that the decoder
must not spend a lot of time in its destructor.

An improvement would be to check if a decoder will be set to null, and if so, stuff it in a "deallocation queue". So, before setting
current_song_decoder = next_song_decoder, check if next_song_decoder is null. If so, push current_song_decoder in a deallocation queue.
This queue will house smart pointers, so pushing it will keep the decoder alive. Then, a *different* thread can get a notification that
something was put into the queue, and pop it until its empty, dropping the smart pointers in the process. This avoids race conditions,
and makes it unnecessary to require fast decoder shutdowns. (The queue can be implemented with an STL deque for storage and a thread
condition variable for notification.)


The sink base can run with reinitialization-on-demand enabled. This affects the way differing decoder samplerates are handled.
If reinitialization on demand is enabled, the sink will be reinitialized if the sink's current sample rate differs from a decoder's one.
This is checked in start(), and in the playback loop, when transitions happen. If it is disabled, then the playback frequency is set
once (when initializing the device), and never changed. Decoders with differing sample rates will be resampled instead.
*/

template < typename Derived >
class common_sink_base:
	public sink
{
	typedef boost::shared_ptr < speex_resampler::resampler > resampler_ptr_t;
public:
	typedef Derived derived_t;
	typedef common_sink_base < Derived > self_t;


	~common_sink_base()
	{
		// stop is not called here; the backend does this already. Calling stop in the sink could lead to unexpected results.
		get_derived().shutdown_audio_device();
	}	


	virtual void start(decoder_ptr_t decoder_, decoder_ptr_t next_decoder_)
	{
		if (!decoder_)
			return;

		if (run_playback_loop)
		{
			pause(false);
			resampler_->reset();
		}
		else
		{
			if (!get_derived().initialize_audio_device(get_playback_frequency(*decoder_)))
			{
				send_command_callback("error", boost::assign::list_of("initializing the audio device failed -> not playing"));
				return;
			}

			resampler_ = resampler_ptr_t(new speex_resampler::resampler(playback_properties_.num_channels, 5, playback_properties_.frequency));
		}

		{
			boost::lock_guard < boost::mutex > lock(mutex);

			if (reinitialize_on_demand && run_playback_loop)
			{
				unsigned int new_frequency = get_playback_frequency(*decoder_);
				bool do_restart = (new_frequency != playback_properties_.frequency);
				if (do_restart && current_decoder)
					do_restart = (get_playback_frequency(*current_decoder) != new_frequency);
				if (do_restart)
				{
					if (!get_derived().reinitialize_audio_device(new_frequency))
					{
						run_playback_loop = false;	
						is_paused = false;
						return;
					}

					resampler_->set_output_frequency(playback_properties_.frequency);
				}
			}

			decoder_->set_playback_properties(playback_properties_);

			if (decoder_->can_playback())
			{
				current_decoder = decoder_;

				if (next_decoder_)
				{
					next_decoder_->set_playback_properties(playback_properties_);
					if (next_decoder_->can_playback())
						next_decoder = next_decoder_;
					else
					{
						send_command_callback("error", boost::assign::list_of("given next decoder cannot playback"));
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
				send_command_callback("error", boost::assign::list_of("given current decoder cannot playback"));
			}
		}

		std::string current_uri = current_decoder->get_uri().get_full();
		std::string next_uri;			
		if (next_decoder)
			next_uri = next_decoder->get_uri().get_full();

		if (run_playback_loop)
			resume(false);
		else
		{
			is_paused = false;
			run_playback_loop = true;
			playback_thread = boost::thread(boost::phoenix::bind(&self_t::playback_loop, this));
		}

		send_event_command(started, boost::assign::list_of(current_uri)(next_uri));
	}


	virtual void stop(bool const do_notify)
	{
		stop_impl(do_notify, stopped);
	}


	virtual void pause(bool const do_notify)
	{
		if (!get_derived().is_initialized() || !current_decoder)
			return;

		{
			boost::lock_guard < boost::mutex > lock(mutex);
			if (is_paused)
				return;
			is_paused = true;
			if (do_notify)
				send_event_command(paused);
		}

		current_decoder->pause();
	}


	virtual void resume(bool const do_notify)
	{
		if (!get_derived().is_initialized() || !current_decoder)
			return;
			
		{
			boost::lock_guard < boost::mutex > lock(mutex);
			if (!is_paused)
				return;
			is_paused = false;
			if (do_notify)
				send_event_command(resumed);
		}

		current_decoder->resume();
	}


	virtual void clear_next_decoder()
	{
		if (!get_derived().is_initialized())
			return;

		boost::lock_guard < boost::mutex > lock(mutex);
		next_decoder = decoder_ptr_t(); // TODO: see deferred shutdown note at the beginning of this file for details
	}


	virtual void set_next_decoder(decoder_ptr_t next_decoder_)
	{
		if (!get_derived().is_initialized() || !current_decoder || !next_decoder_)
			return;

		next_decoder_->set_playback_properties(playback_properties_);

		if (!next_decoder_->can_playback())
		{
			send_command_callback("error", boost::assign::list_of("given next decoder cannot playback"));
			return;
		}

		boost::lock_guard < boost::mutex > lock(mutex);
		next_decoder = next_decoder_;
	}


protected:
	derived_t& get_derived() { return *(static_cast < derived_t* > (this)); }
	derived_t const & get_derived() const { return *(static_cast < derived_t const * > (this)); }


	explicit common_sink_base(send_command_callback_t const &send_command_callback, bool const initialize_on_demand):
		sink(send_command_callback),
		run_playback_loop(false),
		is_paused(false),
		reinitialize_on_demand(initialize_on_demand)
	{
	}


	unsigned int get_playback_frequency(decoder &decoder_) const
	{
		unsigned int frequency = 0;
		if (reinitialize_on_demand)
			frequency = decoder_.get_decoder_samplerate();
		if (frequency == 0)
			frequency = get_derived().get_default_playback_frequency();
		return frequency;
	}


	virtual void stop_impl(bool const do_notify, command_type const command_type_)
	{
		if (!get_derived().is_initialized())
			return;

		{
			boost::lock_guard < boost::mutex > lock(mutex);
			run_playback_loop = false;
		}

		playback_thread.join();
		run_playback_loop = false;
		is_paused = false;

		get_derived().shutdown_audio_device();

		if (do_notify)
			send_event_command(command_type_, boost::assign::list_of(current_decoder->get_uri().get_full()));

		current_decoder = decoder_ptr_t();
		next_decoder = decoder_ptr_t();
	}


	void playback_loop()
	{
		bool do_shutdown = false;
		bool restart_device = false;

		while (true)
		{
			unsigned int num_buffer_samples;

			{
				boost::lock_guard < boost::mutex > lock(mutex);

				if (do_shutdown)
				{
					run_playback_loop = false;
					is_paused = false;
					get_derived().shutdown_audio_device();
					return;
				}

				if (restart_device && current_decoder)
				{
					restart_device = false;
					if (!get_derived().reinitialize_audio_device(get_playback_frequency(*current_decoder)))
						return;
					resampler_->set_output_frequency(playback_properties_.frequency);
					current_decoder->set_playback_properties(playback_properties_);
				}

				if (!run_playback_loop)
					return;

				unsigned int
					num_samples_to_write = playback_properties_.num_buffer_samples,
					sample_offset = 0;

				num_buffer_samples = playback_properties_.num_buffer_samples;


				while (!is_paused && current_decoder)
				{
					unsigned int multiplier = playback_properties_.num_channels * get_sample_size(playback_properties_.sample_type_);
					assert((sample_offset + num_samples_to_write) * multiplier <= get_derived().get_sample_buffer_size());
					unsigned int num_samples_written = (*resampler_)(&(get_derived().get_sample_buffer()[sample_offset  * multiplier]), num_samples_to_write, *current_decoder);

					if (num_samples_written == 0) // no samples were written -> try the next song
					{
						std::string current_uri = current_decoder->get_uri().get_full();
						std::string next_uri;
						
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
						current_decoder = next_decoder;
						next_decoder = decoder_ptr_t();

						if (resource_finished_callback)
							resource_finished_callback();

						if (!next_uri.empty())
						{
							send_event_command(transition,
								boost::assign::list_of
									(current_uri)
									(next_uri)
							);
						}
						else
							send_event_command(resource_finished, boost::assign::list_of(current_uri));

						if (!current_decoder)
							do_shutdown = true;

						if (restart_device)
							break;
					}
					else if (num_samples_written < num_samples_to_write) // less samples were written than expected -> adjust offset and num samples to write
					{
						num_samples_to_write -= num_samples_written;
						sample_offset += num_samples_written;
					}
					else // rest of buffer fully filled -> exit loop, we are done
					{
						num_samples_to_write = 0;
						sample_offset = 0;
						break;
					}
				}


				if (num_samples_to_write > 0)
				{
					// if the current decoder is null, and not the entire buffer was filled, set the remaining samples to zero
					// (also, if the output is paused, num_samples_to_write will always equal the total amount of samples)
					unsigned int multiplier = playback_properties_.num_channels * get_sample_size(playback_properties_.sample_type_);
					assert((sample_offset + num_samples_to_write) * multiplier <= get_derived().get_sample_buffer_size());
					std::memset(&(get_derived().get_sample_buffer()[sample_offset * multiplier]), 0, num_samples_to_write * multiplier);
				}
			}

			if (!get_derived().render_samples(num_buffer_samples))
			{
				// if a fatal error happened, end the playback loop
				do_shutdown = true;
			}
		}
	}


	decoder_ptr_t current_decoder, next_decoder;
	playback_properties playback_properties_;
	bool run_playback_loop, is_paused, reinitialize_on_demand;
	boost::thread playback_thread;
	boost::mutex mutex;
	resampler_ptr_t resampler_;
};


}
}


#endif

