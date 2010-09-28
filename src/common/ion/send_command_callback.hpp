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

