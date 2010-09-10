#ifndef ION_AUDIO_BACKEND_RESAMPLER_HPP
#define ION_AUDIO_BACKEND_RESAMPLER_HPP

#include <stdint.h>
#include <vector>
#include "decoder.hpp"


namespace ion
{
namespace audio_backend
{


class resampler
{
public:
	explicit resampler(unsigned int const initial_num_channels, unsigned int const initial_quality, unsigned int const initial_output_frequency);
	~resampler();


	void set_num_channels(unsigned int const new_num_channels);
	void set_quality(unsigned int const new_quality);
	void set_output_frequency(unsigned int const new_output_frequency);

	unsigned int operator()(void *dest, unsigned int const num_samples_to_write, decoder &decoder_);


protected:
	void set_input_frequency(unsigned int const new_input_frequency);


	struct internal_data;
	internal_data *internal_data_;

	typedef std::vector < uint8_t > input_buffer_t;
	input_buffer_t input_buffer, output_buffer;
	unsigned int remaining_input_data;
	void *resample_handler;
};


}
}


#endif

