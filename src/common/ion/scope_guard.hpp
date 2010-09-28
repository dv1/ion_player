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


#ifndef ION_SCOPE_GUARD_HPP
#define ION_SCOPE_GUARD_HPP

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>


namespace ion
{


// Utility class to make use of RAII with function objects. A supplied start function is called in the constructor, a stop function in the destructor.

class scope_guard:
	public boost::noncopyable
{
public:
	typedef boost::function < void() > function_t;

	explicit scope_guard(function_t const &start_function, function_t const &stop_function):
		start_function(start_function),
		stop_function(stop_function)
	{
		start_function();
	}


	~scope_guard()
	{
		stop_function();
	}


private:
	function_t start_function, stop_function;
};


}

#endif

