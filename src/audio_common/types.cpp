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
		default: return 0;
	}
}


}
}

