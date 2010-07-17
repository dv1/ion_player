//#include <iostream>
#include <sstream>
#include <boost/foreach.hpp>
#include "command_line_tools.hpp"


namespace ion
{


void split_command_line(std::string const &line, std::string &command, params_t &params)
{
	bool escaping = false, param_started = false;
	param_t param;

	std::string::size_type cmd_end_pos = line.find_first_of(' ');
	command = line.substr(0, cmd_end_pos);
	std::string params_line;
	if (cmd_end_pos != std::string::npos)
		params_line = line.substr(cmd_end_pos + 1);

//	std::cout << "[" << line << "] ==> [" << command << "] [" << params_line << "]" << std::endl;

	BOOST_FOREACH(char const char_, params_line)
	{
		if (escaping)
		{
			param += char_;
			escaping = false;
		}
		else
		{
			switch (char_)
			{
				case '\\':
					param += "\\";
					escaping = true;
					break;

				case '"':
					if (param_started)
					{
						params.push_back(unescape_param(param));
						param = "";
						param_started = false;
					}
					else
					{
						param_started = true;
					}
					break;

				default:
					if (param_started)
						param += char_;
			}
		}
	}
}


std::string recombine_command_line(std::string const &command, params_t const &params)
{
	std::stringstream sstr;
	sstr << command;

	BOOST_FOREACH(param_t const &param, params)
	{
		sstr << " \"" << escape_param(param) << '"';
	}

	return sstr.str();
}


param_t escape_param(param_t const &unescaped_param)
{
	param_t result;
	BOOST_FOREACH(char const char_, unescaped_param)
	{
		switch (char_)
		{
			case '"': result += "\\\""; break;
			case '\n': result += "\\n"; break;
			case '\r': result += "\\r"; break;
			default: result += char_;
		}
	}

	return result;
}


param_t unescape_param(param_t const &escaped_param)
{
	param_t result;
	bool escaping = false;
	BOOST_FOREACH(char const char_, escaped_param)
	{
		if (escaping)
		{
			switch (char_)
			{
				case '\\': result += '\\'; break;
				case 'n': result += '\n'; break;
				case 'r': result += '\r'; break;
				case '"': result += '"'; break;
				default: break;
			}
			escaping = false;
		}
		else
		{
			switch (char_)
			{
				case '\\': escaping = true; break;
				default: result += char_; break;
			}
		}
	}


	return result;
}


}

