#include "test.hpp"
#include <sstream>
#include <ion/command_line_tools.hpp>
#include <ion/backend_main_loop.hpp>
#include <send_event_callback.hpp>



int num_exec_command_calls = 0;
std::stringstream in, out;


class mock_backend
{
public:
	virtual std::string get_type() const { return "mock_backend"; }

	virtual void exec_command(std::string const &command, ion::params_t const &params, std::string &response_command, ion::params_t &response_params)
	{
		++num_exec_command_calls;
		response_command = command + "_out";
		response_params = params;
	}


	ion::audio_common::send_event_callback_t cb;
};


void set_send_event_callback(mock_backend &backend_, ion::audio_common::send_event_callback_t const &new_send_event_callback)
{
	backend_.cb = new_send_event_callback;
}


std::string get_backend_type(mock_backend const &backend)
{
	return backend.get_type();
}


void execute_command(mock_backend &backend, std::string const &command, ion::params_t const &parameters, std::string &response_command, ion::params_t &response_parameters)
{
	backend.exec_command(command, parameters, response_command, response_parameters);
}



int test_main(int, char **)
{
	mock_backend mb;

	ion::backend_main_loop < mock_backend > mainloop(in, out, mb);

	std::string input_line = "foo \"abc\" \"d\\\"ef\"";
	std::string expected_line = "foo_out \"abc\" \"d\\\"ef\"";

	{
		in << input_line << std::endl;
		mainloop.iterate();
		std::string actual_line;
		std::getline(out, actual_line);
		TEST_ASSERT(actual_line == expected_line, "Expected [[" << expected_line << "]] got [[" << actual_line << "]]");
	}


	{
		in << "ping" << std::endl;
		mainloop.iterate();
		std::string response;
		std::getline(out, response);
		TEST_ASSERT(response == "pong", "response is [[" << response << "]]");
	}


	{
		in << "ping \"112515\"" << std::endl;
		mainloop.iterate();
		std::string response;
		std::getline(out, response);
		TEST_ASSERT(response == "pong \"112515\"", "response is [[" << response << "]]");
	}


	{
		in << "get_backend_type" << std::endl;
		mainloop.iterate();
		std::string response;
		std::getline(out, response);
		TEST_ASSERT(response == "backend_type \"mock_backend\"", "response is [[" << response << "]]");
	}


	return 0;
}



INIT_TEST

