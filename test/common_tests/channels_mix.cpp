#include "test.hpp"
#include "types.hpp"
#include "mix_channels.hpp"
#include <stdint.h>


int test_main(int, char **)
{
	using namespace ion::audio_common;

	// stereo -> mono, same sample type, with one extra sample in input
	{
		int const num_samples = 10;

		// the extra sample (the 32767 at the end) should be ignored, since it is not part of a sample pair
		// in other words, if the number of input samples is odd, the last sample must be ignored
		int16_t
			input_samples[num_samples * 2 + 1] = { 4, 4,  9, 3,  1, 7,  -20, 10,  7, 7,  189, 1,  1000, 0,  0, 0,  178, -180,  -32768, 0,    32767 },
			expected_output_samples[num_samples + 1] = { 4, 6, 4, -5, 7, 95, 500, 0, -1, -16384,  0 },
			output_samples[num_samples + 1];

		// to test the ignore sample part, an additional output sample value is added to the array, and set to a predefined value
		// if this value is changed by mix_channels(), then something went wrong
		output_samples[num_samples] = 0;

		mix_channels(input_samples, output_samples, num_samples, sample_s16, sample_s16, 2, 1);

		for (int i = 0; i < num_samples + 1; ++i)
			TEST_VALUE(output_samples[i], expected_output_samples[i]);
	}

	// mono -> stereo, same sample type
	{
		int const num_samples = 10;

		int16_t
			input_samples[num_samples] = { 0, 1, 32000, -31000, 498, 18, -1, 1, 99, -32700 },
			output_samples[num_samples * 2];

		mix_channels(input_samples, output_samples, num_samples, sample_s16, sample_s16, 1, 2);

		for (int i = 0; i < num_samples; ++i)
		{
			TEST_VALUE(output_samples[i * 2 + 0], input_samples[i]);
			TEST_VALUE(output_samples[i * 2 + 1], input_samples[i]);
		}
	}

	// mono -> mono, s16 -> s32
	{
		int const num_samples = 10;

		int16_t input_samples[num_samples] = { 0, 1, 32000, -31000, 498, 18, -1, 1, 99, -32700 };
		int32_t output_samples[num_samples];

		mix_channels(input_samples, output_samples, num_samples, sample_s16, sample_s32, 1, 1);

		for (int i = 0; i < num_samples; ++i)
			TEST_VALUE(output_samples[i], int32_t(input_samples[i]) << 16);
	}

	// mono -> mono, same sample type
	{
		int const num_samples = 10;

		int16_t input_samples[num_samples] = { 0, 1, 32000, -31000, 498, 18, -1, 1, 99, -32700 };
		int16_t output_samples[num_samples];

		mix_channels(input_samples, output_samples, num_samples, sample_s16, sample_s16, 1, 1);

		for (int i = 0; i < num_samples; ++i)
			TEST_VALUE(output_samples[i], input_samples[i]);
	}

	return 0;
}



INIT_TEST

