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


#ifndef ION_SPEEX_RESAMPLER_HPP
#define ION_SPEEX_RESAMPLER_HPP

#include <stdint.h>
#include <vector>
#include "decoder.hpp"
#include "types.hpp"


namespace ion
{
namespace speex_resampler
{


class speex_resampler
{
public:
	explicit speex_resampler(unsigned int const quality);
	~speex_resampler();

	audio_common::sample_type find_compatible_type(audio_common::sample_type const suggested_type);
	audio_common::sample_type find_compatible_type(audio_common::sample_type const input_type, audio_common::sample_type const suggested_output_type);

	unsigned long operator()(
		void const *input_data, unsigned long const num_input_samples,
		void *output_data, unsigned long const max_num_output_samples,
		unsigned int const input_frequency, unsigned int const output_frequency,
		audio_common::sample_type const input_type, audio_common::sample_type const output_type,
		unsigned int const num_channels
	);


	void reset();
	bool is_more_input_needed_for(unsigned long const num_output_samples) const;


protected:
	unsigned long resample_16bit(void const *input_data, unsigned long const num_input_samples, void *output_data, unsigned long const max_num_output_samples);
	unsigned long resample_32bit(void const *input_data, unsigned long const num_input_samples, void *output_data, unsigned long const max_num_output_samples);


	struct internal_data;
	internal_data *internal_data_;

	typedef std::vector < uint8_t > buffer_t;
	buffer_t output_buffer;
};



inline audio_common::sample_type find_compatible_type(speex_resampler &resampler_, audio_common::sample_type const type)
{
	return resampler_.find_compatible_type(type);
}

inline audio_common::sample_type find_compatible_type(speex_resampler &resampler_, audio_common::sample_type const input_type, audio_common::sample_type const output_type)
{
	return resampler_.find_compatible_type(input_type, output_type);
}

inline bool is_more_input_needed_for(speex_resampler const &resampler_, unsigned long const num_output_samples)
{
	return resampler_.is_more_input_needed_for(num_output_samples);
}

inline unsigned long resample(
	speex_resampler &resampler_,
	void const *input_data, unsigned long const num_input_samples,
	void *output_data, unsigned long const max_num_output_samples,
	unsigned int const input_frequency, unsigned int const output_frequency,
	audio_common::sample_type const input_type, audio_common::sample_type const output_type,
	unsigned int const num_channels
)
{
	return resampler_(
		input_data, num_input_samples,
		output_data, max_num_output_samples,
		input_frequency, output_frequency,
		input_type, output_type,
		num_channels
	);
}


}
}


#endif

