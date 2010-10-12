#include "test.hpp"
#include "types.hpp"
#include <stdint.h>


int test_main(int, char **)
{
	using namespace ion::audio_common;


	{
		int16_t samples[5] = { -1, 2, 32000, -32000, 1 };

		for (int i = 0; i < 5; ++i)
			TEST_VALUE(get_sample_value(samples, i, sample_s16), samples[i]);
	}

	{
		int32_t samples[5] = { -1, 2, 2000000000, -2000000000, 1 };

		for (int i = 0; i < 5; ++i)
			TEST_VALUE(get_sample_value(samples, i, sample_s32), samples[i]);
	}


	{
		int16_t samples[5] = { 0, 0, 0, 0, 0 };
		int16_t expected_values[5] = { -4, 32700, -31000, 1, 9 };

		for (int i = 0; i < 5; ++i)
		{
			set_sample_value(samples, i, expected_values[i], sample_s16);
			TEST_VALUE(samples[i], expected_values[i]);
		}
	}


	{
		int32_t samples[5] = { 0, 0, 0, 0, 0 };
		int32_t expected_values[5] = { -4, 2000000000, -2000000000, 1, 9 };

		for (int i = 0; i < 5; ++i)
		{
			set_sample_value(samples, i, expected_values[i], sample_s32);
			TEST_VALUE(samples[i], expected_values[i]);
		}
	}


	{
		int16_t input_samples[5] = { -1, 2, 400, 800, 0 };
		int32_t expected_output_samples[5] = { -1l << 16, 2l << 16, 400l << 16, 800l << 16, 0 };

		for (int i = 0; i < 5; ++i)
			TEST_VALUE(convert_sample_value(input_samples[i], sample_s16, sample_s32), expected_output_samples[i]);
	}

	{
		int32_t input_samples[5] = { -1l << 16, 32000l << 16, -32000l << 16, 500 };
		int16_t expected_output_samples[5] = { -1, 32000, -32000, 0 };

		for (int i = 0; i < 5; ++i)
			TEST_VALUE(convert_sample_value(input_samples[i], sample_s32, sample_s16), expected_output_samples[i]);
	}

	return 0;
}



INIT_TEST

