#ifndef ION_COMMAND_LINE_TOOLS_HPP
#define ION_COMMAND_LINE_TOOLS_HPP

#include <string>
#include <vector>


namespace ion
{


/*
In here, several types and function declarations for command-line parsing and (re)construction are located.
A command line looks like this:   <command> [<param1> [<param2> [ .... ] ] ]
Each parameter is *always* enclosed in quotes, without exception. This is a valid command:  ping "abc"   while this is not: ping abc
Commands always use only one line; multiline commands do not exist. Since some parameter values might have symbols that interfere with this
(and with the quotes requirement), escaping is used. The value

a"b
c

becomes

a\"b\nc

This escaping is done by escape_param(), which is automatically called inside recombine_command_line(). unescape_param() does the reverse,
and is called inside split_command_line().

split_command_line() takes a line and splits it in a command and zero or more parameters. recombine_command_line() does the opposite, it takes
a command and zero or more parameters, and outputs a line.

The param_t parameter is used for storing a parameter value. params_t is a random access sequence of parameter values.
*/


// TODO: turn param_t into a class, with lazy (un)escaping (only if it really pays off; this is far from being a bottleneck right now)

typedef std::string param_t;
typedef std::vector < param_t > params_t;

void split_command_line(std::string const &line, std::string &command, params_t &params);
std::string recombine_command_line(std::string const &command, params_t const &params);
param_t escape_param(param_t const &unescaped_param);
param_t unescape_param(param_t const &escaped_param);


}

#endif

