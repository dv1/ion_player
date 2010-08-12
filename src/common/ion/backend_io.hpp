#ifndef ION_BACKEND_IO_HPP
#define ION_BACKEND_IO_HPP

#include <iostream>
#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/noncopyable.hpp>

#include <ion/command_line_tools.hpp>
#include <ion/message_callback.hpp>


namespace ion
{


/*
Backend concept:

std::string get_backend_type(Backend const &backend)
void execute_command(Backend &backend, std::string const &command, params_t const &parameters, std::string &response_command, params_t &response_parameters)
*/


template < typename Backend >
class backend_io:
	private boost::noncopyable
{
public:
	typedef Backend backend_t;


	explicit backend_io(std::istream &in, backend_t &backend_, message_callback_t const &message_callback):
		in(in),
		backend_(backend_),
		message_callback(message_callback)
	{
	}


	void run()
	{
		if (in.bad() || in.fail())
			return;

		while (in.good())
		{
			if (!iterate())
				break;
		}
	}


	bool iterate()
	{
		std::string line;
		std::getline(in, line);

		std::string command;
		params_t params;

		split_command_line(line, command, params);

		if (command == "ping")
		{
			if (params.size() >= 1)
				send_response("pong", boost::assign::list_of(params[0]));
			else
				send_response("pong");
		}
		else if (command == "get_backend_type")
		{
			send_response("backend_type", boost::assign::list_of(get_backend_type(backend_)));
		}
		else if (command == "quit")
		{
			return false;
		}
		else
		{
			std::string response_command;
			params_t response_params;
			execute_command(backend_, command, params, response_command, response_params);

			if (!response_command.empty())
				send_response(response_command, response_params);
		}

		return true;
	}


protected:
	void send_response(std::string const &command, params_t const &params)
	{
		message_callback(command, params);
	}


	void send_response(std::string const &line)
	{
		message_callback(line, params_t());
	}


	std::istream &in;

	backend_t &backend_;
	message_callback_t message_callback;
};


}


#endif

