#ifndef ION_AUDIO_BACKEND_COMPONENT_CREATOR_HPP
#define ION_AUDIO_BACKEND_COMPONENT_CREATOR_HPP

#include <map>
#include <string>


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
};


}
}


#endif

