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


#ifndef ION_AUDIO_CONVERT_HPP
#define ION_AUDIO_CONVERT_HPP

#include <assert.h>
#include <vector>
#include <stdint.h>

#include "mix_channels.hpp"
#include "types.hpp"


namespace ion
{
namespace audio_common
{


template < typename Resampler >
struct convert
{
	explicit convert(Resampler &resampler):
		resampler(resampler)
	{
	}


	template < typename SampleSource, typename InputProperties, typename OutputProperties >
	unsigned long operator()(
		SampleSource &sample_source, InputProperties const &input_properties,
		void *output, unsigned long const num_output_samples, OutputProperties const &output_properties,
		unsigned int const volume, unsigned int const max_volume
	)
	{
		// prerequisites

		unsigned int num_input_channels = get_num_channels(input_properties);
		unsigned int num_output_channels = get_num_channels(output_properties);
		unsigned int input_frequency = get_frequency(input_properties);
		unsigned int output_frequency = get_frequency(output_properties);

		bool frequencies_match = (input_frequency == output_frequency);
		bool num_channels_match = (num_input_channels == num_output_channels);
		bool sample_types_match = get_sample_type(input_properties) == get_sample_type(output_properties);


		// if the properties fully match, no processing is necessary - just transmit the samples directly to the output and exit
		if (frequencies_match && sample_types_match && num_channels_match)
		{
			return retrieve_samples(sample_source, output, num_output_samples);
		}


		// this value is adjusted for the difference between input and output frequency
		// for example, if the input frequency is 48kHz, and the output frequency is 24kHz, the input buffer needs to be 48/24 = 2 times larger
		unsigned long adjusted_num_output_samples = static_cast < unsigned long > (float(num_output_samples) * float(input_frequency) / float(output_frequency) + 0.5f);


		// with the adjusted value from above, retrieve samples from the sample source, and resize the buffer to the number of actually retrieved samples
		// (since the sample source may have given us less samples than we requested)
		unsigned long num_samples_to_retrieve = adjusted_num_output_samples;
		source_data_buffer.resize(num_samples_to_retrieve * num_input_channels * get_sample_size(get_sample_type(input_properties)));
		unsigned long num_retrieved_samples = retrieve_samples(sample_source, &source_data_buffer[0], num_samples_to_retrieve);
		source_data_buffer.resize(num_retrieved_samples * num_input_channels * get_sample_size(get_sample_type(input_properties)));


		// here, the dest and dest_type values are set, as well as the resampler input (if necessary)
		// several optimizations are done here: if the resampler is not necessary, then dest points to the output - any conversion steps will write data directly to the output then
		// if the resampler is necessary, and the sample types match, then the resampler's input is the source data buffer, otherwise it is the "dest" pointer
		// the reason for this is: if the sample types match, no conversion step is necessary, and the resampler can pull data directly from the source data buffer;
		// otherwise, the conversion step needs to convert to an intermediate buffer (the buffer the dest pointer points at), and then the resampler pulls data from this buffer
		uint8_t *dest, *resampler_input;
		sample_type dest_type;
		if (frequencies_match)
		{
			// frequencies match - dest is set to point at the output, meaning that the next step will directly write to the output
			dest_type = get_sample_type(output_properties);
			dest = reinterpret_cast < uint8_t* > (output);
		}
		else
		{
			// frequencies do not match, resampling is necessary - dest is set to point at an intermediate buffer, which the resampler will use

			// ask the resampler what resampling input type it needs - this may differ from the output properties' sample type, but this is ok,
			// since with resampling, an intermediate step between sample type conversion & mixing and actual output is present anyway
			dest_type = find_compatible_type(resampler, get_sample_type(input_properties));

			if (sample_types_match)
			{
				// if the sample types match, then no conversion step is necessary, and the resampler can pull data from the source data buffer directly
				resampler_input = &source_data_buffer[0];
				dest = 0;
			}
			else
			{
				// if the sample types do not match, then the resampler needs to pull data from the intermediate buffer dest points to
				// the conversion step will write to dest
				resampling_input_buffer.resize(adjusted_num_output_samples * num_output_channels * get_sample_size(dest_type));
				dest = &resampling_input_buffer[0];
				resampler_input = dest;
			}
		}


		// first processing stage: convert samples & mix channels
		if (num_channels_match)
		{
			// channel counts match, no mixing necessary

			if (!sample_types_match)
			{
				assert(dest != 0);

				// sample types do not match
				// go through all the input samples, convert them, and write them to dest
				for (unsigned long i = 0; i < num_retrieved_samples * num_input_channels; ++i)
				{
					set_sample_value(
						dest, i,
						convert_sample_value(
							get_sample_value(&source_data_buffer[0], i, get_sample_type(input_properties)),
							get_sample_type(input_properties), dest_type
						),
						dest_type
					);
				}
			}

			// if the sample types match, nothing needs to be done here
		}
		else
		{
			// channels count do not match - call the mixer
			mix_channels(&source_data_buffer[0], dest, num_retrieved_samples, get_sample_type(input_properties), dest_type, get_num_channels(input_properties), get_num_channels(output_properties));
		}


		// the final pass adjusts the output volume; if the volume equals the max volume, it is unnecessary
		// use this boolean to conditionally enable this postpass
		bool do_volume_postpass = (volume != max_volume);


		// second processing stage: resample if necessary
		if (!frequencies_match)
		{
			sample_type resampler_input_type = dest_type;

			// the resampler may have only support for a fixed number of sample types - let it choose a suitable output one
			sample_type resampler_output_type = find_compatible_type(resampler, resampler_input_type, get_sample_type(output_properties));

			// call the actual resampler, which returns the number of samples that were actually sent to output
			num_retrieved_samples = resample(
				resampler,
				resampler_input, num_retrieved_samples,
				output, num_output_samples,
				input_frequency, output_frequency,
				resampler_input_type, resampler_output_type,
				num_output_channels
			);

			// the output type chosen by the resampler may not match the output type given by the output properties, so a final conversion step may be necessary
			if (resampler_output_type != get_sample_type(output_properties))
			{
				sample_type output_sample_type = get_sample_type(output_properties);
				for (unsigned long i = 0; i < num_retrieved_samples * num_output_channels; ++i)
				{
					set_sample_value(
						output, i,
						adjust_sample_volume(
							get_sample_value(output, i, output_sample_type),
							volume, max_volume
						),
						output_sample_type
					);
				}

				// volume adjustment was done in-line in the conversion step above -> no volume post-pass necessary
				do_volume_postpass = false;
			}
		}


		// do the volume postpass if necessary
		if (do_volume_postpass)
		{
			for (unsigned long i = 0; i < num_retrieved_samples * num_output_channels; ++i)
			{
				sample_type output_sample_type = get_sample_type(output_properties);
				set_sample_value(
					output, i,
					adjust_sample_volume(get_sample_value(output, i, output_sample_type), volume, max_volume),
					output_sample_type
				);
			}
		}


		return num_retrieved_samples;
	}


protected:
	inline long adjust_sample_volume(long const sample_value, unsigned int const volume, unsigned int const max_volume)
	{
		return sample_value * long(volume) / long(max_volume);
	}


	typedef std::vector < uint8_t > buffer_t;
	buffer_t resampling_input_buffer, source_data_buffer;
	Resampler &resampler;
};


/*


SampleSource concept:
unsigned long retrieve_samples(SampleSource &sample_source, void *output, unsigned long const num_output_samples)


Resampler concept:

sample_type find_compatible_type(Resampler &resampler, sample_type const type)
sample_type find_compatible_type(Resampler &resampler, sample_type const input_type, sample_type const output_type)
unsigned long resample(
	Resampler &resampler,
	void const *input_data, unsigned long const num_input_samples,
	void *output_data, unsigned long const max_num_output_samples,
	unsigned int const num_input_frequency, unsigned int const num_output_frequency,
	sample_type const input_type, sample_type const output_type,
	unsigned int const num_channels
)


AudioProperties concept:

unsigned int get_num_channels(AudioProperties const &audio_properties)
unsigned int get_frequency(AudioProperties const &audio_properties)
sample_type get_sample_type(AudioProperties const &audio_properties)


*/


}
}


#endif

