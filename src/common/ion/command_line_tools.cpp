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

	// First, separate command and parameters in command and params_line
	std::string::size_type cmd_end_pos = line.find_first_of(' ');
	command = line.substr(0, cmd_end_pos);
	std::string params_line;
	if (cmd_end_pos != std::string::npos)
		params_line = line.substr(cmd_end_pos + 1);


	// Now parse params_line, and extract parameters, taking escape characters into account
	// Parsing works by assembling a parameter char-by-char, and pushing this parameter into the params sequence once it is fully read
	BOOST_FOREACH(char const char_, params_line)
	{
		if (escaping)
		{
			// Escape mode: do not interpret the next character, just add it to the current parameter
			param += char_;
			escaping = false;
		}
		else
		{
			switch (char_)
			{
				case '\\': // \ (backslash) is the escape symbol -> switch to escape mode, do not add this backslash to the current parameter
					param += "\\";
					escaping = true;
					break;

				case '"':
					// The quote signalizes start/end of a parameter. If a parameter was started before, unescape it and store it in params,
					// thereby ending it. Otherwise, it means no parameter was started earlier, so one must be started right now.
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
					// Other symbols are simply added to the current parameter.
					if (param_started)
						param += char_;
			}
		}
	}
}


std::string recombine_command_line(std::string const &command, params_t const &params)
{
	std::stringstream sstr;
	sstr << command; // First store the command in the stringstream

	// Now for each parameter, escape it, put it inside quotes, and store it in the stringstream; the parameters are separated by a whitespace
	BOOST_FOREACH(param_t const &param, params)
	{
		sstr << " \"" << escape_param(param) << '"';
	}

	return sstr.str(); // Return the line
}


param_t escape_param(param_t const &unescaped_param)
{
	param_t result;
	BOOST_FOREACH(char const char_, unescaped_param)
	{
		// Go through each character, and escape it if necessary
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
	// This loop knows two modes: regular, and escaping. If a backslash (\) is encountered in regular mode, it switches to escaping mode (the backslash is then -not- added to the result).
	// In escape mode, the next character will determine the output; for example, n means newline -> the \n character will be added to the result. The mode is then reset to regular.
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

