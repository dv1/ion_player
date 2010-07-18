#include <iostream>
#include <string>
#include <boost/assign/list_of.hpp>
#include "backend_io.hpp"


namespace ion
{


backend_io::backend_io(std::istream &in, backend_base &backend_, message_callback_t const &message_callback):
	in(in),
	backend_(backend_),
	message_callback(message_callback)
{
}


backend_io::~backend_io()
{
}


bool backend_io::iterate()
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
		send_response("backend_type", boost::assign::list_of(backend_.get_type()));
	}
	else if (command == "quit")
	{
		return false;
	}
	else
	{
		std::string response_command;
		params_t response_params;
		backend_.exec_command(command, params, response_command, response_params);

		if (!response_command.empty())
			send_response(response_command, response_params);
	}

	return true;
}


void backend_io::run()
{
	if (in.bad() || in.fail())
		return;

	while (in.good())
	{
		if (!iterate())
			break;
	}
}


void backend_io::send_response(std::string const &command, params_t const &params)
{
	message_callback(command, params);
}


void backend_io::send_response(std::string const &line)
{
	message_callback(line, params_t());
}


}

