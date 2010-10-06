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


#ifndef ION_AUDIO_COMMON_SINK_CREATOR_HPP
#define ION_AUDIO_COMMON_SINK_CREATOR_HPP

#include <string>
#include <ion/uri.hpp>
#include "send_event_callback.hpp"
#include "component_creator.hpp"
#include "sink.hpp"


namespace ion
{
namespace audio_common
{


class sink_creator:
	public component_creator < sink_creator >
{
public:
	virtual sink_ptr_t create(send_event_callback_t const &send_event_callback) = 0;
};


// This is used for plugins
#define DEFINE_ION_AUDIO_COMMON_SINK_CREATOR(SINK_CREATOR_TYPE) \
namespace \
{ \
SINK_CREATOR_TYPE sink_creator_instance_internal; \
} \
\
extern "C" { \
::ion::audio_common::sink_creator *sink_creator_instance = &sink_creator_instance_internal; \
} 


}
}


#endif

