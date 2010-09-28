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

