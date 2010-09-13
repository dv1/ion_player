#ifndef ION_AUDIO_BACKEND_COMPONENT_CREATOR_HPP
#define ION_AUDIO_BACKEND_COMPONENT_CREATOR_HPP

#include <map>
#include <string>
#include "types.hpp"


namespace ion
{
namespace audio_backend
{


template < typename Derived >
class component_creator
{
public:
	typedef std::map < std::string, Derived* > creators_t;

	virtual ~component_creator()
	{
	}

	virtual std::string get_type() const = 0;

	virtual module_ui get_ui() const
	{
		return module_ui("<html><head></head><body>No user interface for this module available.</body></html>", empty_metadata());
	}
};


}
}


#endif

