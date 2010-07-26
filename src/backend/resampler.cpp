#include <assert.h>
#include <fstream>
#include <speex/speex_resampler.h>
#include "resampler.hpp"


namespace ion
{
namespace backend
{


/* TODO:
- testing; check if the byte <-> sample roundings in the () operator work properly
- handle bit depths other than 16
*/


struct resampler::internal_data
{
	SpeexResamplerState *speex_resampler;
	unsigned int num_channels, quality, input_frequency, output_frequency;
};


resampler::resampler(unsigned int const initial_num_channels, unsigned int const initial_quality, unsigned int const initial_output_frequency):
	internal_data_(new internal_data)
{
	int err;
	internal_data_->speex_resampler = speex_resampler_init(initial_num_channels, initial_output_frequency, initial_output_frequency, initial_quality, &err);
	internal_data_->num_channels = initial_num_channels;
	internal_data_->quality = initial_quality;
	internal_data_->input_frequency = initial_output_frequency;
	internal_data_->output_frequency = initial_output_frequency;
	remaining_input_data = 0;
}


resampler::~resampler()
{
	speex_resampler_destroy(internal_data_->speex_resampler);
	delete internal_data_;
}


void resampler::set_num_channels(unsigned int const new_num_channels)
{
	if (internal_data_->num_channels == new_num_channels)
		return;

	speex_resampler_destroy(internal_data_->speex_resampler);
	internal_data_->num_channels = new_num_channels;
	int err;
	internal_data_->speex_resampler = speex_resampler_init(internal_data_->num_channels, internal_data_->input_frequency, internal_data_->output_frequency, internal_data_->quality, &err);
}


void resampler::set_quality(unsigned int const new_quality)
{
	if (internal_data_->quality != new_quality)
	{
		internal_data_->quality = new_quality;
		speex_resampler_set_quality(internal_data_->speex_resampler, new_quality);
	}
}


void resampler::set_input_frequency(unsigned int const new_input_frequency)
{
	if (internal_data_->input_frequency != new_input_frequency)
	{
		internal_data_->input_frequency = new_input_frequency;
		speex_resampler_set_rate(internal_data_->speex_resampler, new_input_frequency, internal_data_->output_frequency);
	}
}


void resampler::set_output_frequency(unsigned int const new_output_frequency)
{
	if (internal_data_->output_frequency != new_output_frequency)
	{
		internal_data_->output_frequency = new_output_frequency;
		speex_resampler_set_rate(internal_data_->speex_resampler, internal_data_->input_frequency, new_output_frequency);
	}
}


unsigned int resampler::operator()(void *dest, unsigned int const num_samples_to_write, decoder &decoder_)
{
	{
		unsigned int decoder_samplerate = decoder_.get_decoder_samplerate();
		if (decoder_samplerate == 0)
			decoder_samplerate = internal_data_->output_frequency;
		set_input_frequency(decoder_samplerate);
	}


	if (internal_data_->input_frequency == internal_data_->output_frequency)
	{
		unsigned int num_samples_written = decoder_.update(dest, num_samples_to_write);
		return num_samples_written;
	}


#if 1
	unsigned channel_sample_factor = internal_data_->num_channels * sizeof(spx_int16_t);
	unsigned int num_samples_to_decode = static_cast < unsigned int > (float(num_samples_to_write) * float(internal_data_->input_frequency) / float(internal_data_->output_frequency) + 0.5f);

	unsigned int num_samples_decoded;

	bool loop = true;
	while (loop)
	{
		unsigned int num_undecoded_samples;

		{
			unsigned int previous_inputbuffer_size = input_buffer.size();

			num_undecoded_samples = (num_samples_to_decode * channel_sample_factor - previous_inputbuffer_size) / channel_sample_factor;
			input_buffer.resize(previous_inputbuffer_size + num_undecoded_samples * channel_sample_factor);
			num_samples_decoded = decoder_.update(&input_buffer[previous_inputbuffer_size], num_undecoded_samples);
			input_buffer.resize(previous_inputbuffer_size + num_samples_decoded * channel_sample_factor);
		}

		if (num_samples_decoded < num_undecoded_samples)
			loop = false;

		if (input_buffer.size() == 0)
			return 0;

		spx_uint32_t in_length = input_buffer.size() / channel_sample_factor;
		spx_uint32_t out_length = num_samples_to_write;

		{
			unsigned int previous_outputbuffer_size = output_buffer.size();
			output_buffer.resize(previous_outputbuffer_size + out_length * channel_sample_factor);

			spx_int16_t const *src_ptr = reinterpret_cast < spx_int16_t const * > (&input_buffer[0]);
			spx_int16_t *dst_ptr = reinterpret_cast < spx_int16_t * > (&output_buffer[previous_outputbuffer_size]);

			int err;
			err = speex_resampler_process_interleaved_int(
				internal_data_->speex_resampler,
				src_ptr,
				&in_length,
				dst_ptr,
				&out_length
			);


			/*static std::ofstream dumpinf("dump_in.raw", std::ios::binary);
			dumpinf.write((char const*)(src_ptr), channel_sample_factor * in_length);

			static std::ofstream dumpoutf("dump_out.raw", std::ios::binary);
			dumpoutf.write((char const*)(dst_ptr), channel_sample_factor * out_length);*/


			unsigned int remaining_in = input_buffer.size() - in_length * channel_sample_factor;
			//std::cout << input_buffer.size() << " " << remaining_in << std::endl;

			if (remaining_in > 0)
				std::memmove(&input_buffer[0], &input_buffer[in_length * channel_sample_factor], remaining_in);
			input_buffer.resize(remaining_in);

			output_buffer.resize(previous_outputbuffer_size + out_length * channel_sample_factor);
		}

		if (output_buffer.size() >= (channel_sample_factor * num_samples_to_write))
		{
			/*static std::ofstream dumpf("dump3.raw", std::ios::binary);
			dumpf.write((char const*)(&output_buffer[0]), channel_sample_factor * num_samples_to_write);*/

			std::memcpy(dest, &output_buffer[0], channel_sample_factor * num_samples_to_write);
			unsigned int remaining_out = output_buffer.size() - channel_sample_factor * num_samples_to_write;

			if (remaining_out > 0)
				std::memmove(&output_buffer[0], &output_buffer[channel_sample_factor * num_samples_to_write], remaining_out);
			output_buffer.resize(remaining_out);

			return num_samples_to_write;
		}
	}


	return 0;
#else

	unsigned channel_sample_factor = internal_data_->num_channels * sizeof(spx_int16_t);
	unsigned int num_samples_to_decode = static_cast < unsigned int > (float(num_samples_to_write) * float(internal_data_->input_frequency) / float(internal_data_->output_frequency) + 0.5f);
	input_buffer.resize(channel_sample_factor * num_samples_to_decode);

	assert(
		(remaining_input_data + (num_samples_to_decode - ((remaining_input_data + sizeof(spx_int16_t) / 2) / sizeof(spx_int16_t))))
		<=
		input_buffer.size()
	);

	unsigned int num_samples_decoded = decoder_.update(
		&input_buffer[remaining_input_data],
		num_samples_to_decode - ((remaining_input_data + sizeof(spx_int16_t) / 2) / sizeof(spx_int16_t)) // the  + sizeof(spx_int16_t) / 2   is a trick to round up the number in case a fractional value would come out
	);
	remaining_input_data += num_samples_decoded * channel_sample_factor;

	assert(remaining_input_data <= input_buffer.size());

	if (remaining_input_data == 0)
		return 0;

	spx_uint32_t in_length = remaining_input_data / channel_sample_factor;
	spx_uint32_t out_length = num_samples_to_write;

	int err;
	err = speex_resampler_process_interleaved_int(internal_data_->speex_resampler, reinterpret_cast < spx_int16_t const * > (&input_buffer[0]), &in_length, reinterpret_cast < spx_int16_t * > (dest), &out_length);
	std::cout << input_buffer.size() << " " << in_length << " " << (num_samples_to_write * channel_sample_factor) << " " << out_length << std::endl;

//	remaining_input_data -= in_length * channel_sample_factor;
	remaining_input_data = 0;
	if (remaining_input_data > 0)
		std::memcpy(&input_buffer[0], &input_buffer[input_buffer.size() - remaining_input_data], remaining_input_data);

	return num_samples_to_write * channel_sample_factor;
#endif
}


}
}

