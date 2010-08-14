#ifndef ION_SEND_COMMAND_CALLBACK_HPP
#define ION_SEND_COMMAND_CALLBACK_HPP

#include <boost/function.hpp>
#include "command_line_tools.hpp"


namespace ion
{


// This typedef is widely used in the backend for passing around the send command callback. The send command callback is used by the backend to send events to the frontend
// via the backend process' stdout.
typedef boost::function < void(std::string const &command, params_t const &params) > send_command_callback_t;


}


#endif

