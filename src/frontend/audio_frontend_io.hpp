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

	void pause(bool const set);
	bool is_paused() const;

	void set_current_volume(long const new_volume);


protected:
	void parse_command(std::string const &event_command_name, params_t const &event_params);

	bool paused;
};


}
}


#endif

