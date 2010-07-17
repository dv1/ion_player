#ifndef ION_BACKEND_BACKEND_COMMON_SINK_BASE_HPP
#define ION_BACKEND_BACKEND_COMMON_SINK_BASE_HPP

#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include "sink.hpp"
#include "sink_creator.hpp"


namespace ion
{
namespace backend
{


template < typename Derived >
class common_sink_base:
	public sink
{
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
			pause(false);
		else
		{
			if (!get_derived().initialize_audio_device())
			{
				message_callback("error", boost::assign::list_of("initializing the audio device failed -> not playing"));
				return;
			}
		}

		{
			boost::lock_guard < boost::mutex > lock(mutex);

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
						message_callback("error", boost::assign::list_of("given next decoder cannot playback"));
						next_decoder = decoder_ptr_t();
					}
				}
				else
					next_decoder = decoder_ptr_t();
			}
			else
				message_callback("error", boost::assign::list_of("given current decoder cannot playback"));
		}

		if (run_playback_loop)
			resume(false);
		else
		{
			is_paused = false;
			run_playback_loop = true;
			playback_thread = boost::thread(boost::lambda::bind(&self_t::playback_loop, this));
			send_message(started, boost::assign::list_of(current_decoder->get_uri().get_full()));
		}
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
				send_message(paused);
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
				send_message(resumed);
		}

		current_decoder->resume();
	}


	virtual void set_next_decoder(decoder_ptr_t next_decoder_)
	{
		if (!get_derived().is_initialized() || !current_decoder || !next_decoder_)
			return;

		next_decoder_->set_playback_properties(playback_properties_);

		if (!next_decoder_->can_playback())
		{
			message_callback("error", boost::assign::list_of("given next decoder cannot playback"));
			return;
		}

		boost::lock_guard < boost::mutex > lock(mutex);
		next_decoder = next_decoder_;
	}


protected:
	derived_t& get_derived() { return *(static_cast < derived_t* > (this)); }
	derived_t const & get_derived() const { return *(static_cast < derived_t const * > (this)); }


	explicit common_sink_base(message_callback_t const &message_callback):
		sink(message_callback),
		run_playback_loop(false),
		is_paused(false)
	{
	}


	virtual void stop_impl(bool const do_notify, message_type const message_type_)
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
			send_message(message_type_, boost::assign::list_of(current_decoder->get_uri().get_full()));

		current_decoder = decoder_ptr_t();
		next_decoder = decoder_ptr_t();
	}


	void playback_loop()
	{
		bool do_shutdown = false;

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
					unsigned int num_samples_written = current_decoder->update(&(get_derived().get_sample_buffer()[sample_offset  * multiplier]), num_samples_to_write);
					if (num_samples_written < num_samples_to_write) // less samples were written than expected -> adjust offset and num samples to write, and try the next song
					{
						num_samples_to_write -= num_samples_written;
						sample_offset += num_samples_written;

						if (next_decoder)
							send_message(transition,
								boost::assign::list_of
									(current_decoder->get_uri().get_full())
									(next_decoder->get_uri().get_full())
							);
						else
							send_message(resource_finished, boost::assign::list_of(current_decoder->get_uri().get_full()));

						// if (next_song_decoder == null) deallocation_queue.push(current_song_decoder); // TODO: see audio.txt docs for details
						current_decoder = next_decoder;
						next_decoder = decoder_ptr_t();

						if (!current_decoder)
							do_shutdown = true;

						if (resource_finished_callback)
							resource_finished_callback();
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
	bool run_playback_loop, is_paused;
	boost::thread playback_thread;
	boost::mutex mutex;
};


}
}


#endif

