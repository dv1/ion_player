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


#ifndef ION_AUDIO_MIX_CHANNELS_HPP
#define ION_AUDIO_MIX_CHANNELS_HPP

#include "mix_channels.hpp"
#include "types.hpp"


namespace ion
{
namespace audio_common
{


/**
* This function allows for mixing channels in an interleaved audio stream.
* Interleaving refers to the order of each channel's samples. For instance, with two channels, the samples are ordered in this fashion:
* 1 2 1 2 1 2 1 2 ... as opposed to: 1 1 1 1 2 2 2 2 ...
* Noninterleaved streams are not supported by this function (and the audio backend in general).
* In the mixing process, sample type conversion is also performed if the input and output sample types differ.
*
* Currently, the following channel configurations are supported:
* 2 input -> 1 output (stereo -> mono)
* 1 input -> 2 output (mono -> stereo)
* N input -> N output (number of channels are the same; nothing is mixed, data is just copied)
*
* @param source_data Pointer to the source audio stream
* @param dest_data Pointer to the buffer where the mixed audio stream shall be written to
* @param num_samples Number of samples to mix; note, this value does not implicitely contain a number of channels; to get the total size of the audio data
* in bytes, this calculation is used: total_size = num_samples * num_channels * get_sample_size(sample_type)
* @param input_type Type of the input samples
* @param output_type Type of the output samples
* @num_input_channels Number of channels the input stream is made of
* @num_output_channels Number of channels the output stream shall have
* @pre Parameters are valid; dest_data points to a buffer that is at least (num_samples * num_output_channels * get_sample_size(output_type)) bytes big;
* number of input and output channels are supported by this function
* @post dest_data will contain the mixed audio stream
*/
void mix_channels(
	void const *source_data, void *dest_data,
	unsigned int const num_samples,
	sample_type const input_type, sample_type const output_type,
	unsigned int const num_input_channels, unsigned int const num_output_channels
);


}
}


#endif

