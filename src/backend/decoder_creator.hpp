#ifndef ION_BACKEND_DECODER_CREATOR_HPP
#define ION_BACKEND_DECODER_CREATOR_HPP

#include <string>
#include <ion/send_command_callback.hpp>
#include <ion/uri.hpp>
#include <ion/metadata.hpp>
#include "component_creator.hpp"
#include "decoder.hpp"
#include "source.hpp"


namespace ion
{
namespace backend
{


class decoder_creator:
	public component_creator < decoder_creator >
{
public:
	virtual decoder_ptr_t create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback) = 0;
};


// This is used for plugins
#define DEFINE_ION_BACKEND_DECODER_CREATOR(DECODER_CREATOR_TYPE) \
namespace \
{ \
DECODER_CREATOR_TYPE decoder_creator_instance_internal; \
} \
\
extern "C" { \
::ion::backend::decoder_creator *decoder_creator_instance = &decoder_creator_instance_internal; \
} 


}
}


#endif

