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


#include <stdint.h>
#include "types.hpp"


namespace ion
{
namespace audio_common
{


unsigned int get_sample_size(sample_type const &type)
{
	switch (type)
	{
		case sample_s16: return 2;
		case sample_s24: return 3;
		case sample_s24_x8_lsb: return 4;
		case sample_s24_x8_msb: return 4;
		case sample_s32: return 4;
		default: return 0;
	}
}


long get_sample_value(void const *src, unsigned int const sample_value_index, sample_type const type)
{
	uint8_t const *ptr = reinterpret_cast < uint8_t const * > (src) + get_sample_size(type) * sample_value_index;

	switch (type)
	{
		case sample_s16: return *(reinterpret_cast < int16_t const * > (ptr));
		case sample_s32: return *(reinterpret_cast < int32_t const * > (ptr));
		default: return 0;
	}
}


void set_sample_value(void *dest, unsigned int const sample_value_index, long const value, sample_type const type)
{
	uint8_t *ptr = reinterpret_cast < uint8_t* > (dest) + get_sample_size(type) * sample_value_index;

	switch (type)
	{
		case sample_s16: *(reinterpret_cast < int16_t* > (ptr)) = value; break;
		case sample_s32: *(reinterpret_cast < int32_t* > (ptr)) = value; break;
		default: break;
	}
}


long convert_sample_value(long const value, sample_type const input_type, sample_type const output_type)
{
	if (input_type == output_type)
		return value;

	switch (input_type)
	{
		case sample_s16:
			switch (output_type)
			{
				case sample_s32: return value << 16;
				default: return 0;
			}
			break;

		case sample_s32:
			switch (output_type)
			{
				case sample_s16: return value >> 16;
				default: return 0;
			}
			break;

		default:
			return 0;
	}
}


}
}

