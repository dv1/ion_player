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


#ifndef ION_AUDIO_COMMON_SOURCE_CREATOR_HPP
#define ION_AUDIO_COMMON_SOURCE_CREATOR_HPP

#include <ion/uri.hpp>
#include "component_creator.hpp"
#include "source.hpp"


namespace ion
{
namespace audio_common
{


class source_creator:
	public component_creator
{
public:
	virtual source_ptr_t create(ion::uri const &uri_) = 0;
};


// This is used for plugins
#define DEFINE_ION_AUDIO_COMMON_SOURCE_CREATOR(SOURCE_CREATOR_TYPE) \
namespace \
{ \
SOURCE_CREATOR_TYPE source_creator_instance_internal; \
} \
\
extern "C" { \
::ion::audio_common::source_creator *source_creator_instance = &source_creator_instance_internal; \
} 



}
}


#endif

