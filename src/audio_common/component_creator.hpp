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


#ifndef ION_AUDIO_COMMON_COMPONENT_CREATOR_HPP
#define ION_AUDIO_COMMON_COMPONENT_CREATOR_HPP

#include <string>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include "types.hpp"


namespace ion
{
namespace audio_common
{


// Base class for all the other creators (source/decoder/sink creators and all their derivatives).
class component_creator
{
public:
	virtual ~component_creator()
	{
	}

	// The type specifier; it is used for creating components akin to the factory pattern
	virtual std::string get_type() const = 0;

	// UI for common settings for the components this creator instantiates. The UI is written in HTML. See the design document for more about module GUIs.
	virtual module_ui get_ui() const
	{
		return module_ui("<html><head></head><body>No user interface for this module available.</body></html>", empty_metadata());
	}
};


// This struct defines a type for a creator container. This container can retrieve creators by using the creator's type string.
// Additionally, the container preserves the order of the components.
// To see how to use the container, consult the boost multi-index container documentation (http://www.boost.org/doc/libs/1_44_0/libs/multi_index/doc/index.html).
template < typename Creator >
struct component_creators
{
protected:
	// This functor is used for custom key retrieval in the multi-index container, making it possible to use a type string as a key
	struct get_type_from_creator
	{
		typedef std::string result_type;
		result_type operator()(Creator *creator_instance) const
		{
			return creator_instance->get_type();
		}
	};


public:
	struct sequence_tag {};
	struct ordered_tag {};

	typedef Creator creator_t;

	typedef boost::multi_index::multi_index_container <
		Creator*,
		boost::multi_index::indexed_by <
			boost::multi_index::random_access < boost::multi_index::tag < sequence_tag > >,
			boost::multi_index::ordered_unique < boost::multi_index::tag < ordered_tag >, get_type_from_creator >
		>
	> creators_t;

	typedef typename creators_t::template index < sequence_tag > ::type sequenced_t;
	typedef typename creators_t::template index < ordered_tag > ::type ordered_t;
};


}
}


#endif

