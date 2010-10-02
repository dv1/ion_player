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


#include "alsa_sink.hpp"


namespace ion
{
namespace audio_backend
{


using namespace audio_common;


alsa_sink::alsa_sink(send_command_callback_t const &send_command_callback):
	common_sink_base < alsa_sink > (send_command_callback),
	pcm_handle(0),
	hw_params(0)
{
}


bool alsa_sink::is_initialized() const
{
	return (pcm_handle != 0);
}


namespace
{

template < typename T >
inline void checked_alsa_call(T const &t, std::string const &error_msg)
{
	if (t < 0)
		throw std::runtime_error(error_msg + ": " + snd_strerror(t));
}

}


bool alsa_sink::initialize_audio_device(unsigned int const playback_frequency)
{
	boost::lock_guard < boost::mutex > lock(alsa_mutex);
	return initialize_audio_device_impl(playback_frequency);
}


bool alsa_sink::initialize_audio_device_impl(unsigned int const playback_frequency)
{
	if (pcm_handle != 0)
		return true;


	playback_properties new_playback_properties(playback_frequency, 2048, 2, sample_s16);


	checked_alsa_call(snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0), "unable to open alsa pcm");

	checked_alsa_call(snd_pcm_hw_params_malloc(&hw_params), "unable to allocate hw params structure");
	checked_alsa_call(snd_pcm_hw_params_any(pcm_handle, hw_params), "could not fill hw params structure with full configuration space");
#if SND_LIB_VERSION >= 0x010009
	checked_alsa_call(snd_pcm_hw_params_set_rate_resample(pcm_handle, hw_params, 0), "disabling ALSA resampling failed"); // turns off alsa resampling
#endif
	checked_alsa_call(snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED), "could not set pcm access");
	checked_alsa_call(snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE), "could not set pcm format");
	checked_alsa_call(snd_pcm_hw_params_set_channels(pcm_handle, hw_params, new_playback_properties.num_channels), "could not number of channels");
	checked_alsa_call(snd_pcm_hw_params_set_periods(pcm_handle, hw_params, 4, 0), "could not set number of periods");

	unsigned int freq = new_playback_properties.frequency;
	checked_alsa_call(snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &freq, 0), "setting near sample rate failed");
	checked_alsa_call(snd_pcm_hw_params_get_rate(hw_params, &freq, 0), "retrieving actual sample rate failed");
	new_playback_properties.frequency = freq;
	std::cout << "ALSA: playback frequency: " << freq << " Hz" << std::endl;

	snd_pcm_uframes_t frames = new_playback_properties.num_buffer_samples;
	checked_alsa_call(snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &frames, 0), "setting near period size failed");
	checked_alsa_call(snd_pcm_hw_params_get_period_size(hw_params, &frames, 0), "retrieving actual period size failed");
	new_playback_properties.num_buffer_samples = frames;
	std::cout << "ALSA: playback buffer size: " << frames << " samples" << std::endl;

	checked_alsa_call(snd_pcm_hw_params(pcm_handle, hw_params), "setting hw params failed");

	snd_pcm_uframes_t buffer_size[2], cur_buffer_size = 0;
	checked_alsa_call(snd_pcm_hw_params_get_buffer_size_min(hw_params, &buffer_size[0]), "getting minimum buffer size failed");
	checked_alsa_call(snd_pcm_hw_params_get_buffer_size_max(hw_params, &buffer_size[1]), "getting maximum buffer size failed");
	checked_alsa_call(snd_pcm_hw_params_get_buffer_size(hw_params, &cur_buffer_size), "getting current buffer size failed");
	std::cout << "ALSA: buffer size range: [" << buffer_size[0] << ", " << buffer_size[1] << "]  current buffer size: " << cur_buffer_size << std::endl;

	float approx_num_periods = (float(cur_buffer_size) / float(frames));
	std::cout << "ALSA: " << frames << " samples per period -> approx. " << approx_num_periods << " periods (approx. latency: " << ((approx_num_periods - 1) * float(frames) / float(freq) * 1000.0f) << " ms)" << std::endl;

	unsigned int num_periods[2] = { 0, 0 }, cur_num_periods = 0;
	checked_alsa_call(snd_pcm_hw_params_get_periods_min(hw_params, &num_periods[0], 0), "getting minimum number of periods failed");
	checked_alsa_call(snd_pcm_hw_params_get_periods_max(hw_params, &num_periods[1], 0), "getting maximum number of periods failed");
	checked_alsa_call(snd_pcm_hw_params_get_periods(hw_params, &cur_num_periods, 0), "getting current number of periods failed");
	std::cout << "ALSA: period count range: [" << num_periods[0] << ", " << num_periods[1] << "]  current count: " << cur_num_periods << std::endl;


	playback_properties_ = new_playback_properties;


	unsigned int new_buffer_size = playback_properties_.num_buffer_samples * playback_properties_.num_channels * get_sample_size(playback_properties_.sample_type_);
	if (sample_buffer.size() != new_buffer_size)
		sample_buffer.resize(new_buffer_size);


	std::cout << "ALSA: initialization complete." << std::endl;


	return true;
}


bool alsa_sink::reinitialize_audio_device(unsigned int const playback_frequency)
{
	boost::lock_guard < boost::mutex > lock(alsa_mutex);

	shutdown_audio_device();
	return initialize_audio_device_impl(playback_frequency);
}


void alsa_sink::shutdown_audio_device()
{
	// Even though both the playback thread and stop_impl() call shutdown_audio_device, there is no need to synchronize this function
	// the playback thread calls this when a song finished and no next song is there, while stop_impl() calls this only -after- the playback thread was join()ed

	if (pcm_handle == 0)
		return;

	if (hw_params != 0)
		snd_pcm_hw_params_free(hw_params);
	if (pcm_handle != 0)
	{
		snd_pcm_drain(pcm_handle);
		snd_pcm_close(pcm_handle);
	}

	std::cout << "ALSA: shutdown complete." << std::endl;

	pcm_handle = 0;
	hw_params = 0;
}


bool alsa_sink::render_samples(unsigned int const num_samples_to_render)
{
	boost::lock_guard < boost::mutex > lock(alsa_mutex);

	int ret = snd_pcm_writei(pcm_handle, &sample_buffer[0], num_samples_to_render);
	if (ret < 0)
	{
		switch (-ret)
		{
			case EINTR:
			case EPIPE:
			case ESTRPIPE:
			{
				std::cerr << "ALSA: ";

				switch (-ret)
				{
					case EINTR: std::cerr << "interrupted"; break;
					case EPIPE: std::cerr << "underrun detected"; break;
					case ESTRPIPE: std::cerr << "device suspended"; break;
					default: std::cerr << "??unknown error?? (report an error if you see this message)"; break;
				}

				std::cerr << std::endl;
				ret = snd_pcm_recover(pcm_handle, ret, 1);
				if (ret < 0)
				{
					std::cerr << "ALSA: fatal: error while recovering: " << snd_strerror(ret) << " -> stopping" << std::endl;
					return false;
				}
				break;
			}

			default:
				// unknown error -> fatal
				std::cerr << "ALSA: fatal: unknown error while sending PCM data: " << snd_strerror(ret) << " -> stopping" << std::endl;
				return false;
		}
	}

	return true;
}




sink_ptr_t alsa_sink_creator::create(send_command_callback_t const &send_command_callback)
{
	return sink_ptr_t(new alsa_sink(send_command_callback));
}


}
}

