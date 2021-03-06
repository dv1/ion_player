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


#ifndef ION_AUDIO_COMMON_TYPES_HPP
#define ION_AUDIO_COMMON_TYPES_HPP

#include <json/value.h>


/*
This header contains a number of miscellaneous types and related functions.
*/


namespace ion
{
namespace audio_common
{


//////
// sample types and functions

enum sample_type
{
	sample_s16,        // 16-bit integer sample, no padding (-> 16 bit total)
	sample_s24,        // 24-bit integer sample, no padding (-> 24 bit total)
	sample_s24_x8_lsb, // 24-bit integer sample, 8 bit of padding (-> 32 bit total); these 8 bit are the LSB; bitmask  MSB xxxxxxxxxxxxxxxxxxxxxxxx00000000 LSB  (x = sample bit)
	sample_s24_x8_msb, // 24-bit integer sample, 8 bit of padding (-> 32 bit total); these 8 bit are the MSB; bitmask  MSB 00000000xxxxxxxxxxxxxxxxxxxxxxxx LSB  (x = sample bit)
	sample_s32,        // 32-bit integer sample, no padding (-> 32 bit total)
	sample_unknown
};


unsigned int get_sample_size(sample_type const &type);
long get_sample_value(void const *src, unsigned int const sample_value_index, sample_type const type);
void set_sample_value(void *dest, unsigned int const sample_value_index, long const value, sample_type const type);
long convert_sample_value(long const value, sample_type const input_type, sample_type const output_type);




//////
// playback properties class


struct playback_properties
{
	unsigned int frequency; // = The playback frequency (for example, 44100 = 44100 Hz = 44100 samples per second)
	unsigned int num_buffer_samples; // The number of channels does NOT affect this value.
	unsigned int num_channels;
	sample_type sample_type_;


	playback_properties():
		num_buffer_samples(0), num_channels(0), sample_type_(sample_unknown)
	{
	}

	explicit playback_properties(unsigned int const frequency, unsigned int const num_buffer_samples, unsigned int const num_channels, sample_type const sample_type_):
		frequency(frequency), num_buffer_samples(num_buffer_samples), num_channels(num_channels), sample_type_(sample_type_)
	{
	}

	bool is_valid() const
	{
		return
			(frequency > 0) &&
			(num_buffer_samples > 0) &&
			(num_channels > 0) &&
			(sample_type_ != sample_unknown)
			;
	}
};



// extrinsic functions to let the decoder meet the AudioProperties concept requirements

namespace
{

inline unsigned int get_num_channels(playback_properties const &audio_properties) { return audio_properties.num_channels; }
inline unsigned int get_frequency(playback_properties const &audio_properties) { return audio_properties.frequency; }
inline sample_type get_sample_type(playback_properties const &audio_properties) { return audio_properties.sample_type_; }

}




//////
// decoder properties class



struct decoder_properties
{
	unsigned int frequency; // = The playback frequency (for example, 44100 = 44100 Hz = 44100 samples per second)
	unsigned int num_channels;
	sample_type sample_type_;


	decoder_properties():
		num_channels(0), sample_type_(sample_unknown)
	{
	}

	explicit decoder_properties(unsigned int const frequency, unsigned int const num_channels, sample_type const sample_type_):
		frequency(frequency), num_channels(num_channels), sample_type_(sample_type_)
	{
	}

	explicit decoder_properties(playback_properties const &src):
		frequency(src.frequency), num_channels(src.num_channels), sample_type_(src.sample_type_)
	{
	}

	bool is_valid() const
	{
		return
			(frequency > 0) &&
			(num_channels > 0) &&
			(sample_type_ != sample_unknown)
			;
	}
};



// extrinsic functions to let the decoder meet the AudioProperties concept requirements

namespace
{

inline unsigned int get_num_channels(decoder_properties const &audio_properties) { return audio_properties.num_channels; }
inline unsigned int get_frequency(decoder_properties const &audio_properties) { return audio_properties.frequency; }
inline sample_type get_sample_type(decoder_properties const &audio_properties) { return audio_properties.sample_type_; }

}




//////
// module ui class


struct module_ui
{
	std::string html_code;
	Json::Value properties;

	explicit module_ui():
		properties(Json::objectValue)
	{
	}

	explicit module_ui(std::string const &html_code, Json::Value const &properties):
		html_code(html_code),
		properties(Json::objectValue)
	{
	}
};


}
}


#endif

