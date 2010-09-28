#ifndef ION_AUDIO_COMMON_SINK_CREATOR_HPP
#define ION_AUDIO_COMMON_SINK_CREATOR_HPP

#include <string>
#include <ion/send_command_callback.hpp>
#include <ion/uri.hpp>
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
	virtual sink_ptr_t create(send_command_callback_t const &send_command_callback) = 0;
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
