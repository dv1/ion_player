#ifndef ION_BACKEND_BASE_HPP
#define ION_BACKEND_BASE_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <ion/command_line_tools.hpp>


namespace ion
{


class backend_base:
	private boost::noncopyable
{
public:
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


	typedef std::vector < std::string > parameters_t;

	virtual std::string get_type() const = 0;
	virtual bool exec_command(std::string const &command, params_t const &params, std::string &response_command, params_t &response_params) = 0;

	virtual ~backend_base()
	{
	}
};


}


#endif


