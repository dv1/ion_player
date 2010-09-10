#ifndef ION_AUDIO_BACKEND_BACKEND_ALSA_SINK_HPP
#define ION_AUDIO_BACKEND_BACKEND_ALSA_SINK_HPP

#include <stdint.h>
#include <vector>
#include <alsa/asoundlib.h>
#include "common_sink_base.hpp"


namespace ion
{
namespace audio_backend
{


class alsa_sink:
	public common_sink_base < alsa_sink >
{
public:
	explicit alsa_sink(send_command_callback_t const &send_command_callback);


	bool is_initialized() const;
	bool initialize_audio_device();
	void shutdown_audio_device();
	bool render_samples(unsigned int const num_samples_to_render);

	inline std::size_t get_sample_buffer_size() const { return sample_buffer.size(); }
	inline uint8_t* get_sample_buffer() { return &sample_buffer[0]; }


protected:
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *hw_params;

	typedef std::vector < uint8_t > sample_buffer_t;
	sample_buffer_t sample_buffer;
};




class alsa_sink_creator:
	public sink_creator
{
public:
	virtual sink_ptr_t create(send_command_callback_t const &send_command_callback);
	virtual std::string get_type() const { return "alsa"; }
};


}
}


#endif

