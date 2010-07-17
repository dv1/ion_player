#ifndef ION_COMMAND_LINE_TOOLS_HPP
#define ION_COMMAND_LINE_TOOLS_HPP

#include <string>
#include <vector>


namespace ion
{


// TODO: turn param_t into a class, with lazy (un)escaping

typedef std::string param_t;
typedef std::vector < param_t > params_t;

void split_command_line(std::string const &line, std::string &command, params_t &params);
std::string recombine_command_line(std::string const &command, params_t const &params);
param_t escape_param(param_t const &unescaped_param);
param_t unescape_param(param_t const &escaped_param);


}

#endif

