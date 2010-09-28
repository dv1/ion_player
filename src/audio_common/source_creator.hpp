#ifndef ION_AUDIO_COMMON_SOURCE_CREATOR_HPP
#define ION_AUDIO_COMMON_SOURCE_CREATOR_HPP

#include <ion/send_command_callback.hpp>
#include <ion/uri.hpp>
#include "component_creator.hpp"
#include "source.hpp"


namespace ion
{
namespace audio_common
{


class source_creator:
	public component_creator < source_creator >
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

