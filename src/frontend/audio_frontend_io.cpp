#include "audio_frontend_io.hpp"


namespace ion
{
namespace frontend
{


audio_frontend_io::audio_frontend_io(send_line_to_backend_callback_t const &send_line_to_backend_callback):
	base_t(send_line_to_backend_callback)
{
}


}
}

