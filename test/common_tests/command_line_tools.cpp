#include "test.hpp"
#include <string>
#include <stdio.h>
#include <ion/command_line_tools.hpp>
#include <boost/assign/list_of.hpp>


int test_main(int, char **)
{
	{
		std::string input("A\"B\nC\rD\"\n\rE"), expected_output("A\\\"B\\nC\\rD\\\"\\n\\rE");
		std::string actual_output = ion::escape_param(input);
		TEST_ASSERT(actual_output == expected_output, "Expected [[ " << expected_output << " ]] got [[ " << actual_output << " ]]");
	}

	{
		std::string input("A\\\"B\\nC\\rD\\\"\\n\\rE"), expected_output("A\"B\nC\rD\"\n\rE");
		std::string actual_output = ion::unescape_param(input);
		TEST_ASSERT(actual_output == expected_output, "Expected [[ " << expected_output << " ]] got [[ " << actual_output << " ]]");
	}

	{
		std::string command("foo"), expected_line("foo");
		std::string actual_line = ion::recombine_command_line(command, ion::params_t());
		TEST_ASSERT(actual_line == expected_line, "Expected [[" << expected_line << "]] got [[" << actual_line << "]]");

		std::string split_command;
		ion::params_t split_params;
		ion::split_command_line(expected_line, split_command, split_params);
		TEST_ASSERT(command == split_command, "Expected command [[" << command << "]], got [[" << split_command << "]]");
		TEST_ASSERT(split_params.size() == 0, "Expected no params got " << split_params.size());
	}

	{
		std::string command("foo"), param1("abc"), param2("x\"y"), param3("def");
		std::string expected_line("foo \"abc\" \"x\\\"y\" \"def\"");
		std::string actual_line = ion::recombine_command_line(command, boost::assign::list_of(param1)(param2)(param3));
		TEST_ASSERT(actual_line == expected_line, "Expected [[ " << expected_line << " ]] got [[ " << actual_line << " ]]");

		std::string split_command;
		ion::params_t split_params;
		ion::split_command_line(expected_line, split_command, split_params);
		TEST_ASSERT(command == split_command, "Expected command [[" << command << "]], got [[" << split_command << "]]");
		TEST_ASSERT(split_params.size() == 3, "Expected 3 params got " << split_params.size());
		TEST_ASSERT(split_params[0] == param1, "Param #1 should be [[" << param1 << "]], is [[" << split_params[0] << "]]");
		TEST_ASSERT(split_params[1] == param2, "Param #2 should be [[" << param2 << "]], is [[" << split_params[1] << "]]");
		TEST_ASSERT(split_params[2] == param3, "Param #3 should be [[" << param3 << "]], is [[" << split_params[2] << "]]");
	}



	return 0;
}


INIT_TEST

