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


class unrecognized_resource:
	public std::runtime_error
{
public:
	explicit unrecognized_resource(std::string const &resource_uri):
		runtime_error(resource_uri)
	{
	}
};


}


#endif

