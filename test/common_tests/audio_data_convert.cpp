#include "test.hpp"
#include "types.hpp"
#include "convert.hpp"
#include <stdint.h>


using namespace ion::audio_common;



struct mock_resampler {};

unsigned long resample(
	mock_resampler &,
	void const *input_data, unsigned long const num_input_samples,
	void *output_data, unsigned long const num_output_samples,
	unsigned int const num_input_frequency, unsigned int const num_output_frequency,
	sample_type const input_type, sample_type const output_type,
	unsigned int const num_channels
)
{
	for (unsigned long i = 0; i < num_output_samples; ++i)
	{
		unsigned long j = i * num_input_samples/ num_output_samples;
		set_sample_value(
			output_data, i,
			convert_sample_value(
				get_sample_value(input_data, j, input_type),
				input_type, output_type
			),
			output_type
		);
	}


	return num_output_samples;
}


sample_type find_compatible_type(mock_resampler &, sample_type const type)
{
	return type;
}


sample_type find_compatible_type(mock_resampler &, sample_type const, sample_type const output_type)
{
	return output_type;
}




struct mock_audio_properties
{
	unsigned int num_channels, frequency;
	sample_type sample_type_;


	mock_audio_properties() {}

	mock_audio_properties(unsigned int const num_channels, unsigned int const frequency, sample_type const &sample_type_):
		num_channels(num_channels),
		frequency(frequency),
		sample_type_(sample_type_)
	{
	}
};


unsigned int get_num_channels(mock_audio_properties const &audio_properties) { return audio_properties.num_channels; }
unsigned int get_frequency(mock_audio_properties const &audio_properties) { return audio_properties.frequency; }
sample_type get_sample_type(mock_audio_properties const &audio_properties) { return audio_properties.sample_type_; }





struct mock_sample_source
{
	explicit mock_sample_source(bool const stereo):
		stereo(stereo),
		counter(0)
	{
	}


	long get_sample()
	{
		// using this, stereo output always delivers samples that are pair-wise identical, like: 3,3,  4,4,  5,5, ....
		long value = ((stereo ? (counter / 2) : counter) & 65535) - 32768;
		++counter;
		return value;
	}


	void reset()
	{
		counter = 0;
	}


	unsigned long get_count() const { return counter; }
	bool is_stereo() const { return stereo; }


protected:
	bool const stereo;
	unsigned long counter;
};


unsigned long retrieve_samples(mock_sample_source &sample_source, void *output, unsigned long const num_output_samples)
{
	for (unsigned long i = 0; i < num_output_samples * (sample_source.is_stereo() ? 2 : 1); ++i)
	{
		set_sample_value(output, i, sample_source.get_sample(), sample_s16);
	}


	return num_output_samples;
}








int test_main(int, char **)
{
	// same frequency, same number of channels, same sample type
	{
		int const num_samples = 200;

		mock_sample_source sample_source(false);
		mock_resampler resampler;

		int16_t output_samples[num_samples];

		unsigned long num_converted = convert < mock_resampler > (resampler)(
			sample_source, mock_audio_properties(1, 10000, sample_s16),
			output_samples, num_samples, mock_audio_properties(1, 10000, sample_s16),
			255, 255
		);


		TEST_VALUE(num_converted, static_cast < unsigned long > (num_samples));


		sample_source.reset();
		for (int i = 0; i < num_samples; ++i)
			TEST_VALUE(output_samples[i], sample_source.get_sample());
	}


	// same frequency, same number of channels, s16 -> s32
	{
		int const num_samples = 200;

		mock_sample_source sample_source(false);
		mock_resampler resampler;

		int32_t output_samples[num_samples];

		unsigned long num_converted = convert < mock_resampler > (resampler)(
			sample_source, mock_audio_properties(1, 10000, sample_s16),
			output_samples, num_samples, mock_audio_properties(1, 10000, sample_s32),
			255, 255
		);


		TEST_VALUE(num_converted, static_cast < unsigned long > (num_samples));


		sample_source.reset();
		for (int i = 0; i < num_samples; ++i)
			TEST_VALUE(output_samples[i], sample_source.get_sample() << 16);
	}


	// same frequency, mono->stereo, s16 -> s32
	{
		int const num_samples = 200;

		mock_sample_source sample_source(false);
		mock_resampler resampler;

		int32_t output_samples[num_samples * 2];

		unsigned long num_converted = convert < mock_resampler > (resampler)(
			sample_source, mock_audio_properties(1, 10000, sample_s16),
			output_samples, num_samples, mock_audio_properties(2, 10000, sample_s32),
			255, 255
		);


		TEST_VALUE(sample_source.get_count(), static_cast < unsigned long > (num_samples));
		TEST_VALUE(num_converted, static_cast < unsigned long > (num_samples));


		sample_source.reset();
		for (int i = 0; i < num_samples; ++i)
		{
			int32_t expected_value = sample_source.get_sample() << 16;
			TEST_VALUE(output_samples[i * 2 + 0], expected_value);
			TEST_VALUE(output_samples[i * 2 + 1], expected_value);
		}
	}


	// same frequency, stereo->mono, s16 -> s32
	{
		int const num_samples = 200;

		mock_sample_source sample_source(true);
		mock_resampler resampler;

		int32_t output_samples[num_samples];

		unsigned long num_converted = convert < mock_resampler > (resampler)(
			sample_source, mock_audio_properties(2, 10000, sample_s16),
			output_samples, num_samples, mock_audio_properties(1, 10000, sample_s32),
			255, 255
		);


		TEST_VALUE(sample_source.get_count(), static_cast < unsigned long > (num_samples * 2));
		TEST_VALUE(num_converted, static_cast < unsigned long > (num_samples));


		sample_source.reset();
		for (int i = 0; i < num_samples; ++i)
		{
			int32_t expected_value = sample_source.get_sample() << 16;
			sample_source.get_sample(); // dummy call, necessary for stereo test
			TEST_VALUE(output_samples[i], expected_value);
		}
	}


	// 20kHz -> 10 kHz, same number of channels, same sample type
	{
		int const num_samples = 16384;

		mock_sample_source sample_source(false);
		mock_resampler resampler;

		int16_t output_samples[num_samples];

		unsigned long num_converted = convert < mock_resampler > (resampler)(
			sample_source, mock_audio_properties(1, 20000, sample_s16),
			output_samples, num_samples, mock_audio_properties(1, 10000, sample_s16),
			255, 255
		);


		TEST_VALUE(sample_source.get_count(), static_cast < unsigned long > (num_samples * 2));
		TEST_VALUE(num_converted, static_cast < unsigned long > (num_samples));


		sample_source.reset();
		for (int i = 0; i < num_samples; ++i)
		{
			long value = sample_source.get_sample();
			sample_source.get_sample(); // dummy call
			TEST_VALUE(output_samples[i], value);
		}
	}


	// 10kHz -> 20 kHz, same number of channels, same sample type
	{
		int const num_samples = 16384;

		mock_sample_source sample_source(false);
		mock_resampler resampler;

		int16_t output_samples[num_samples];

		unsigned long num_converted = convert < mock_resampler > (resampler)(
			sample_source, mock_audio_properties(1, 10000, sample_s16),
			output_samples, num_samples, mock_audio_properties(1, 20000, sample_s16),
			255, 255
		);


		TEST_VALUE(sample_source.get_count(), static_cast < unsigned long > (num_samples / 2));
		TEST_VALUE(num_converted, static_cast < unsigned long > (num_samples));


		sample_source.reset();
		for (int i = 0; i < num_samples / 2; ++i)
		{
			long value = sample_source.get_sample();
			TEST_VALUE(output_samples[i * 2 + 0], value);
			TEST_VALUE(output_samples[i * 2 + 1], value);
		}
	}


	// 20kHz->10kHz, stereo->mono, s16 -> s32
	{
		int const num_samples = 200;

		mock_sample_source sample_source(true);
		mock_resampler resampler;

		int32_t output_samples[num_samples];

		unsigned long num_converted = convert < mock_resampler > (resampler)(
			sample_source, mock_audio_properties(2, 20000, sample_s16),
			output_samples, num_samples, mock_audio_properties(1, 10000, sample_s32),
			255, 255
		);


		TEST_VALUE(sample_source.get_count(), static_cast < unsigned long > (num_samples * 4));
		TEST_VALUE(num_converted, static_cast < unsigned long > (num_samples));


		sample_source.reset();
		for (int i = 0; i < num_samples; ++i)
		{
			int32_t expected_value = sample_source.get_sample() << 16; // first channel of sample pair #1
			sample_source.get_sample(); // dummy call (second channel of sample pair #1)
			sample_source.get_sample(); // dummy call (first channel of sample pair #2)
			sample_source.get_sample(); // dummy call (second channel of sample pair #2)
			TEST_VALUE(output_samples[i], expected_value);
		}
	}


	return 0;
}



INIT_TEST

