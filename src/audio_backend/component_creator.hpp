#ifndef ION_AUDIO_BACKEND_COMPONENT_CREATOR_HPP
#define ION_AUDIO_BACKEND_COMPONENT_CREATOR_HPP

#include <string>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include "types.hpp"


namespace ion
{
namespace audio_backend
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

