#ifndef ION_SCOPE_GUARD_HPP
#define ION_SCOPE_GUARD_HPP

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>


namespace ion
{


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

