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


#include <assert.h>
#include <iostream>
#include <fstream>
#include "config.h"
#include <speex/speex_resampler.h>
#include "resampler.hpp"


namespace ion
{
namespace speex_resampler
{


/* TODO:
- handle bit depths other than 16
- test with decoders that change the frequency while playing
*/


struct speex_resampler::internal_data
{
	SpeexResamplerState *speex_resampler;
	unsigned int num_channels, quality, input_frequency, output_frequency;
	audio_common::sample_type sample_type_;

	internal_data():
		speex_resampler(0),
		num_channels(0),
		quality(0),
		input_frequency(0),
		output_frequency(0),
		sample_type_(audio_common::sample_unknown)
	{
	}

	bool are_parameters_valid() const
	{
		return
			(num_channels != 0) &&
			(input_frequency != 0) &&
			(output_frequency != 0) &&
			(sample_type_ != audio_common::sample_unknown)
			;
	}
};



speex_resampler::speex_resampler(unsigned int const quality):
	internal_data_(new internal_data)
{
	internal_data_->quality = quality;
}


speex_resampler::~speex_resampler()
{
	if (internal_data_->speex_resampler != 0)
		speex_resampler_destroy(internal_data_->speex_resampler);
	delete internal_data_;
}


void speex_resampler::reset()
{
	if (internal_data_->speex_resampler != 0)
	{
		speex_resampler_destroy(internal_data_->speex_resampler);
		internal_data_->speex_resampler = 0;
	}

	if (internal_data_->are_parameters_valid())
	{
		int err;
		internal_data_->speex_resampler = speex_resampler_init(internal_data_->num_channels, internal_data_->input_frequency, internal_data_->output_frequency, internal_data_->quality, &err);
	}

	output_buffer.clear();
}


bool speex_resampler::is_more_input_needed_for(unsigned long const num_output_samples) const
{
	if (internal_data_->speex_resampler == 0)
		return true;
	else
	{
		unsigned long sample_multiplier = internal_data_->num_channels * get_sample_size(internal_data_->sample_type_);
		return (output_buffer.size() < (sample_multiplier * num_output_samples));
	}
}


audio_common::sample_type speex_resampler::find_compatible_type(audio_common::sample_type const suggested_type)
{
	switch (suggested_type)
	{
		case audio_common::sample_s16:
			return audio_common::sample_s16;
		case audio_common::sample_s24:
		case audio_common::sample_s24_x8_lsb:
		case audio_common::sample_s24_x8_msb:
		case audio_common::sample_s32:
			return audio_common::sample_s32;
		default:
			return audio_common::sample_unknown;
	}
}


audio_common::sample_type speex_resampler::find_compatible_type(audio_common::sample_type const input_type, audio_common::sample_type const/* suggested_output_type*/)
{
	return find_compatible_type(input_type);
}


unsigned long speex_resampler::operator()(
	void const *input_data, unsigned long const num_input_samples,
	void *output_data, unsigned long const max_num_output_samples,
	unsigned int const input_frequency, unsigned int const output_frequency,
	audio_common::sample_type const input_type, audio_common::sample_type const output_type,
	unsigned int const num_channels
)
{
	if (num_input_samples == 0)
		return 0;

	assert(input_type == output_type);

	bool input_frequency_changed = (input_frequency != internal_data_->input_frequency);
	bool output_frequency_changed = (output_frequency != internal_data_->output_frequency);
	bool input_type_changed = (input_type != internal_data_->sample_type_);

	internal_data_->input_frequency = input_frequency;
	internal_data_->output_frequency = output_frequency;
	internal_data_->sample_type_ = input_type;
	internal_data_->num_channels = num_channels;

	if ((internal_data_->speex_resampler == 0) || input_type_changed)
		reset();
	else
	{
		if (input_frequency_changed || output_frequency_changed)
		{
			speex_resampler_set_rate(internal_data_->speex_resampler, input_frequency, output_frequency);
			// clear input buffer
		}
	}

	unsigned long sample_multiplier = num_channels * get_sample_size(input_type);
	unsigned long num_samples_to_output = 0;

	if (is_more_input_needed_for(max_num_output_samples))
	{
		unsigned long offset = output_buffer.size();
		unsigned long adjusted_max_num_output_samples = max_num_output_samples * output_frequency / input_frequency + 128;
		output_buffer.resize(offset + adjusted_max_num_output_samples * sample_multiplier);

		unsigned long num_written_samples = 0;

		switch (input_type)
		{
			case audio_common::sample_s16: num_written_samples = resample_16bit(input_data, num_input_samples, &output_buffer[offset], adjusted_max_num_output_samples); break;
			case audio_common::sample_s32: num_written_samples = resample_32bit(input_data, num_input_samples, &output_buffer[offset], adjusted_max_num_output_samples); break;
			default: assert(0); return 0;
		}

		output_buffer.resize(offset + num_written_samples * sample_multiplier);
		num_samples_to_output = std::min(max_num_output_samples, num_written_samples);
	}
	else
	{
	//	std::cerr << "no more input needed for resampler\n";
		num_samples_to_output = max_num_output_samples;
	}

	{
		unsigned long num_bytes_to_copy = num_samples_to_output * sample_multiplier;
		unsigned long num_remaining_bytes = output_buffer.size() - num_bytes_to_copy;
		std::memcpy(output_data, &output_buffer[0], num_bytes_to_copy);
		std::memmove(&output_buffer[0], &output_buffer[num_bytes_to_copy], num_remaining_bytes);
		output_buffer.resize(num_remaining_bytes);
	}

//	std::cerr << "num input samples: " << num_input_samples << "  samples  output buffer size: " << output_buffer.size() << " byte   send to output: " << num_samples_to_output << " samples\n";

	return num_samples_to_output;
}


unsigned long speex_resampler::resample_16bit(void const *input_data, unsigned long const num_input_samples, void *output_data, unsigned long const max_num_output_samples)
{
	spx_uint32_t in_length = num_input_samples;
	spx_uint32_t out_length = max_num_output_samples;

	spx_int16_t const *in_ptr = reinterpret_cast < spx_int16_t const * > (input_data);
	spx_int16_t *out_ptr = reinterpret_cast < spx_int16_t * > (output_data);

	int err;
	err = speex_resampler_process_interleaved_int(
		internal_data_->speex_resampler,
		in_ptr,
		&in_length,
		out_ptr,
		&out_length
	);

//	if (in_length != num_input_samples)
//		std::cerr << "WARNING: speex resampler used " << in_length << " out of " << num_input_samples << " input samples (all should be used)\n";

	return out_length;
}


unsigned long speex_resampler::resample_32bit(void const *input_data, unsigned long const num_input_samples, void *output_data, unsigned long const max_num_output_samples)
{
	spx_uint32_t in_length = num_input_samples;
	spx_uint32_t out_length = max_num_output_samples;

	float const *in_ptr = reinterpret_cast < float const * > (input_data);
	float *out_ptr = reinterpret_cast < float * > (output_data);

	int err;
	err = speex_resampler_process_interleaved_float(
		internal_data_->speex_resampler,
		in_ptr,
		&in_length,
		out_ptr,
		&out_length
	);

	return out_length;
}


}
}

