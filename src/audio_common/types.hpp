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

#include <ion/metadata.hpp>


namespace ion
{
namespace audio_common
{


enum sample_type
{
	sample_s16,
	sample_s24,
	sample_unknown
};


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

	playback_properties& merge_properties(playback_properties const &other_properties)
	{
		if (this->frequency == 0)                 this->frequency = other_properties.frequency;
		if (this->num_buffer_samples == 0)        this->num_buffer_samples = other_properties.num_buffer_samples;
		if (this->num_channels == 0)              this->num_channels = other_properties.num_channels;
		if (this->sample_type_ == sample_unknown) this->sample_type_ = other_properties.sample_type_;
		return *this;
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


struct module_ui
{
	std::string html_code;
	metadata_t properties;

	explicit module_ui():
		properties(empty_metadata())
	{
	}

	explicit module_ui(std::string const &html_code, metadata_t const &properties):
		html_code(html_code),
		properties(properties)
	{
	}
};


unsigned int get_sample_size(sample_type const &type);


}
}


#endif

