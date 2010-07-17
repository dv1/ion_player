#ifndef ION_MESSAGE_CALLBACK_HPP
#define ION_MESSAGE_CALLBACK_HPP

#include <boost/function.hpp>
#include "command_line_tools.hpp"


namespace ion
{


typedef boost::function < void(std::string const &message, params_t const &params) > message_callback_t;


}


#endif

