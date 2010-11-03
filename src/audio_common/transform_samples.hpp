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


#ifndef ION_AUDIO_TRANSFORM_SAMPLES_HPP
#define ION_AUDIO_TRANSFORM_SAMPLES_HPP

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
struct transform_samples
{
	// resampler must meet the Resampler concept requirements
	explicit transform_samples(Resampler &resampler):
		resampler(resampler)
	{
	}


	// sample_source must meet the SampleSource concept requirements
	// input_properties and output_properties both must meet the AudioProperties concept requirements
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

		if (input_frequency == 0) // if the input frequency is 0, then the sample source can adapt to the output frequency
			input_frequency = output_frequency;

		bool frequencies_match = (input_frequency == output_frequency);
		bool num_channels_match = (num_input_channels == num_output_channels);
		bool sample_types_match = get_sample_type(input_properties) == get_sample_type(output_properties);


		// if the properties fully match, no processing is necessary - just transmit the samples directly to the output and exit
		// do a volume processing if necessary, but otherwise its just one transmission
		if (frequencies_match && sample_types_match && num_channels_match)
		{
			unsigned long num_retrieved_samples = retrieve_samples(sample_source, output, num_output_samples);

			if (volume != max_volume)
			{
				sample_type output_sample_type = get_sample_type(output_properties);
				for (unsigned long i = 0; i < num_retrieved_samples * num_output_channels; ++i)
				{
					set_sample_value(
						output, i,
						adjust_sample_volume(get_sample_value(output, i, output_sample_type), volume, max_volume),
						output_sample_type
					);
				}
			}

			return num_retrieved_samples;
		}


		unsigned long num_retrieved_samples = 0;
		uint8_t *resampler_input = 0;
		sample_type dest_type = sample_unknown;
		// note that this returns true if the resampler is in an uninitialized state
		bool resampler_needed_more_input = is_more_input_needed_for(resampler, num_output_samples);



		// first processing stage: convert samples & mix channels if necessary
		// also, the resampler input is prepared here
		// if resampling is necessary, and the resampler has enough input data for now, this stage is bypassed

		if (frequencies_match || resampler_needed_more_input)
		{
			// with the adjusted value from above, retrieve samples from the sample source, and resize the buffer to the number of actually retrieved samples
			// (since the sample source may have given us less samples than we requested)
			unsigned long num_samples_to_retrieve = num_output_samples;
			source_data_buffer.resize(num_samples_to_retrieve * num_input_channels * get_sample_size(get_sample_type(input_properties)));
			num_retrieved_samples = retrieve_samples(sample_source, &source_data_buffer[0], num_samples_to_retrieve);
			source_data_buffer.resize(num_retrieved_samples * num_input_channels * get_sample_size(get_sample_type(input_properties)));

			// here, the dest and dest_type values are set, as well as the resampler input (if necessary)
			// several optimizations are done here: if the resampler is not necessary, then dest points to the output - any conversion steps will write data directly to the output then
			// if the resampler is necessary, and the sample types match, then the resampler's input is the source data buffer, otherwise it is the "dest" pointer
			// the reason for this is: if the sample types match, no conversion step is necessary, and the resampler can pull data directly from the source data buffer;
			// otherwise, the conversion step needs to convert to an intermediate buffer (the buffer the dest pointer points at), and then the resampler pulls data from this buffer
			uint8_t *dest;
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

				if (sample_types_match && num_channels_match)
				{
					// if the sample types and channel count match, then no conversion step is necessary, and the resampler can pull data from the source data buffer directly
					resampler_input = &source_data_buffer[0];
					dest = 0;
				}
				else
				{
					// if the sample types and channel count do not match, then the resampler needs to pull data from the intermediate buffer dest points to
					// the conversion step will write to dest
					resampling_input_buffer.resize(num_output_samples * num_output_channels * get_sample_size(dest_type));
					dest = &resampling_input_buffer[0];
					resampler_input = dest;
				}
			}


			// actual mixing and conversion is done here
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
		}
		else
		{
			// either resampling is not necessary, or the resampler does not need any new input for now
			// set number of retrieved samples to number of output samples, and if resampling is necessary, set the dest_type value

			num_retrieved_samples = num_output_samples;

			// the dest_type value is set, since the resampler might reset itself if it sees a change in the input type
			if (!frequencies_match)
				dest_type = find_compatible_type(resampler, get_sample_type(input_properties));
		}


		// the final stage adjusts the output volume; if the volume equals the max volume, it is unnecessary
		// this boolean conditionally enables this stage
		bool do_volume_stage = (volume != max_volume);


		// second processing stage: resample if necessary (otherwise this stage is bypassed)
		if (!frequencies_match)
		{
			sample_type resampler_input_type = dest_type;

			// the resampler may have only support for a fixed number of sample types - let it choose a suitable output one
			sample_type resampler_output_type = find_compatible_type(resampler, resampler_input_type, get_sample_type(output_properties));
			sample_type output_type = get_sample_type(output_properties);
			bool conversion_needed = (resampler_output_type != output_type);

			uint8_t *resampler_output = reinterpret_cast < uint8_t* > (output);
			if (conversion_needed)
			{
				resampling_output_buffer.resize(num_output_samples * num_output_channels * get_sample_size(resampler_output_type));
				resampler_output = &resampling_output_buffer[0];
			}

			// resampler input -can- be zero - sometimes the resampler does not need any more input
			assert(!resampler_needed_more_input || (resampler_input != 0));

			// call the actual resampler, which returns the number of samples that were actually sent to output
			// if this is the first call since the resampler was reset(), this call internally initializes the resampler
			// and sets its internal values to the given ones
			// note that the is_more_input_needed_for() call returns true if the resampler is in such an uninitialized state
			// if the resampler was initialized already, it may reinitialize itself internally if certain parameters change
			// (this is entirely implementation-dependent; from the outside, no such reinitialization is noticeable)
			num_retrieved_samples = resample(
				resampler,
				resampler_input, num_retrieved_samples,
				resampler_output, num_output_samples,
				input_frequency, output_frequency,
				resampler_input_type, resampler_output_type,
				num_output_channels
			);

			// the output type chosen by the resampler may not match the output type given by the output properties, so a final conversion step may be necessary
			if (conversion_needed)
			{
				if (do_volume_stage)
				{
					// if the volume stage is required, use the opportunity to do it together with the final conversion step
					for (unsigned long i = 0; i < num_retrieved_samples * num_output_channels; ++i)
					{
						set_sample_value(
							output, i,
							adjust_sample_volume(
								get_sample_value(resampler_output, i, resampler_output_type),
								volume, max_volume
							),
							output_type
						);
					}

					// volume adjustment was done in-line in the conversion step above -> no further volume adjustment required
					do_volume_stage = false;
				}
				else
				{
					// conversion without volume adjustment
					for (unsigned long i = 0; i < num_retrieved_samples * num_output_channels; ++i)
						set_sample_value(output, i, get_sample_value(resampler_output, i, output_type), output_type);
				}
			}
		}


		// do the volume stage if required
		if (do_volume_stage)
		{
			sample_type output_sample_type = get_sample_type(output_properties);
			for (unsigned long i = 0; i < num_retrieved_samples * num_output_channels; ++i)
			{
				set_sample_value(
					output, i,
					adjust_sample_volume(get_sample_value(output, i, output_sample_type), volume, max_volume),
					output_sample_type
				);
			}
		}


		// finally, return the number of retrieved samples, either from the resampler, or from the source directly,
		// depending on whether or not resampling was necessary
		return num_retrieved_samples;
	}


protected:
	inline long adjust_sample_volume(long const sample_value, unsigned int const volume, unsigned int const max_volume)
	{
		return long(int64_t(sample_value) * int64_t(volume) / int64_t(max_volume));
	}


	typedef std::vector < uint8_t > buffer_t;
	buffer_t resampling_input_buffer, resampling_output_buffer, source_data_buffer;
	Resampler &resampler;
};


}
}


#endif

