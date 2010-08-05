#ifndef ION_FRONTEND_AUDIO_FRONTEND_IO_HPP
#define ION_FRONTEND_AUDIO_FRONTEND_IO_HPP

#include <QObject>

#include <ion/frontend_io_base.hpp>
#include <ion/simple_playlist.hpp>


class QTabWidget;
class QTreeView;


namespace ion
{
namespace frontend
{


class audio_frontend_io:
	public frontend_io_base < simple_playlist >
{
public:
	typedef frontend_io_base < simple_playlist > base_t;


	explicit audio_frontend_io(send_line_to_backend_callback_t const &send_line_to_backend_callback);


protected:
};


}
}


#endif

