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


#ifndef ION_BACKEND_MAIN_LOOP_HPP
#define ION_BACKEND_MAIN_LOOP_HPP

#include <iostream>
#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/noncopyable.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>

#include <ion/command_line_tools.hpp>


namespace ion
{


/*
This is a utility class for writing backend processes. Said processes need a main loop that gets lines from the input stream and outputs them to the output stream.
Usually, the input stream is stdin and the output one is stdout. A backend process' main.cpp file then contains an instance of a backend type, an instance of this
class (whose template typename is set to the backend type), the backend instance is passed to this classes' constructor, and finally, either run() or iterate() is
called.

The backend type instance is created _first_, and _then_ an instance of this main loop class is created; the backend type instance is passed to its constructor.
*/

// See Backend concept in docs/concepts.txt
template < typename Backend >
class backend_main_loop:
	private boost::noncopyable
{
public:
	typedef Backend backend_t;
	typedef backend_main_loop < Backend > self_t;


	/*
	The constructor accepts an input stream, an output stream, and a backend instance. The input/output streams are used for communication with the frontend.
	Commands are received from the input stream, and events are sent to the output stream.
	The given backend has its send command callback set by using a set_send_event_callback() function call.

	@param in The input stream
	@param out The output stream
	@param backend_ The backend that will get commands and send events through this class
	@pre in, out, and backend_ must be valid
	@post Main loop will be ready to run; the backend will have its send command callback set to this class' internal send command callback
	*/
	explicit backend_main_loop(std::istream &in, std::ostream &out, backend_t &backend_):
		in(in),
		out(out),
		backend_(backend_)
	{
		set_send_event_callback(backend_, boost::phoenix::bind(&self_t::send_response_command_params, this, boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2));
	}


	/*
	Starts the main loop, and exits as soon as the input stream is no longer able to receive new input.
	Internally, it calls iterate() within the loop. If the input stream is not ready to read when run() is called,
	nothing happens - the function exits.
	@post The backend will have its internal states shut down. The user has signalized that the backend shall be ended - exit the main() function as soon as possible
	*/
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


	/*
	Handles one main loop iteration. If you use your own main loop, call this instead of run().
	This function blocks until a line is read, and then parses that line. If the line is invalid or empty, this function does nothing.
	The return vale is false when the quit command was received, otherwise it is always true.
	@post One command may have been executed, its effects depending on the specific command
	*/
	bool iterate()
	{
		std::string line;
		std::getline(in, line);
		if (line.empty())
			return true;

		std::string command;
		params_t params;

		split_command_line(line, command, params);

		if (command == "ping")
		{
			if (params.size() >= 1)
				send_response_command_params("pong", boost::assign::list_of(params[0]));
			else
				send_response_line("pong");
		}
		else if (command == "get_backend_type")
		{
			send_response_command_params("backend_type", boost::assign::list_of(get_backend_type(backend_)));
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
				send_response_command_params(response_command, response_params);
		}

		return true;
	}


protected:
	// The send command callback the backend is given. Also used inside iterate() directly.
	void send_response_command_params(std::string const &command, params_t const &params)
	{
		out << ion::recombine_command_line(command, params) << std::endl;
	}


	// A variant of the send command callback that sends a line directly. Not used by the backend - in fact it is only used in iterate().
	void send_response_line(std::string const &line)
	{
		out << line << std::endl;
	}




	std::istream &in;
	std::ostream &out;

	backend_t &backend_;
};


}


#endif

