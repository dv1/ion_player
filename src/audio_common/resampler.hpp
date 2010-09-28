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


#ifndef ION_AUDIO_COMMON_RESAMPLER_HPP
#define ION_AUDIO_COMMON_RESAMPLER_HPP

#include <stdint.h>
#include <vector>
#include "decoder.hpp"


namespace ion
{
namespace audio_common
{


class resampler
{
public:
	explicit resampler(unsigned int const initial_num_channels, unsigned int const initial_quality, unsigned int const initial_output_frequency);
	~resampler();


	void set_num_channels(unsigned int const new_num_channels);
	void set_quality(unsigned int const new_quality);
	void set_output_frequency(unsigned int const new_output_frequency);
	void reset();

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

