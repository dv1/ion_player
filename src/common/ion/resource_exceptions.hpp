#ifndef ION_RESOURCE_EXCEPTIONS_HPP
#define ION_RESOURCE_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>


namespace ion
{


class resource_not_found:
	public std::runtime_error
{
public:
	explicit resource_not_found(std::string const &resource_uri):
		runtime_error(resource_uri)
	{
	}
};


class resource_corrupted:
	public std::runtime_error
{
public:
	explicit resource_corrupted(std::string const &resource_uri):
		runtime_error(resource_uri)
	{
	}
};


}


#endif

