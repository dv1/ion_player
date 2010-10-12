#include <cstring>
#include <stdint.h>
#include "mix_channels.hpp"


namespace ion
{
namespace audio_common
{


void mix_channels_1_to_2(
	void const *source_data, void *dest_data,
	unsigned int const num_samples,
	sample_type const input_type, sample_type const output_type
)
{
	int output_pos = 0;

	for (unsigned int i = 0; i < num_samples; ++i, output_pos += 2)
	{
		long sample_value = 
			convert_sample_value(
				get_sample_value(source_data, i, input_type),
				input_type,
				output_type
			);

		set_sample_value(dest_data, output_pos, sample_value, output_type);
		set_sample_value(dest_data, output_pos + 1, sample_value, output_type);
	}
}


void mix_channels_2_to_1(
	void const *source_data, void *dest_data,
	unsigned int const num_samples,
	sample_type const input_type, sample_type const output_type
)
{
	for (unsigned int i = 0; i < num_samples; ++i)
	{
		int64_t sample_value_1 = 
			convert_sample_value(
				get_sample_value(source_data, i * 2 + 0, input_type),
				input_type,
				output_type
			);

		int64_t sample_value_2 = 
			convert_sample_value(
				get_sample_value(source_data, i * 2 + 1, input_type),
				input_type,
				output_type
			);

		set_sample_value(dest_data, i, long((sample_value_1 + sample_value_2) / 2), output_type);
	}
}


void mix_channels(
	void const *source_data, void *dest_data,
	unsigned int const num_samples,
	sample_type const input_type, sample_type const output_type,
	unsigned int const num_input_channels, unsigned int const num_output_channels
)
{
	if (num_input_channels == num_output_channels)
	{
		if (input_type == output_type)
			std::memcpy(dest_data, source_data, num_samples * get_sample_size(input_type));
		else
		{
			for (unsigned int i = 0; i < num_samples * num_output_channels; ++i)
			{
				set_sample_value(
					dest_data, i,
					convert_sample_value(
						get_sample_value(source_data, i, input_type),
						input_type,
						output_type
					),
					output_type
				);
			}
		}
	}


	if ((num_input_channels == 1) && (num_output_channels == 2))
		mix_channels_1_to_2(source_data, dest_data, num_samples, input_type, output_type);
	else if ((num_input_channels == 2) && (num_output_channels == 1))
		mix_channels_2_to_1(source_data, dest_data, num_samples, input_type, output_type);
}


}
}

