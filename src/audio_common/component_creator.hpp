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


template < typename Derived >
class component_creator
{
public:
	virtual ~component_creator()
	{
	}

	virtual std::string get_type() const = 0;

	virtual module_ui get_ui() const
	{
		return module_ui("<html><head></head><body>No user interface for this module available.</body></html>", empty_metadata());
	}
};


template < typename Creator >
struct component_creators
{
protected:
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

