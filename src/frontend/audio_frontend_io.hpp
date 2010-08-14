#ifndef ION_FRONTEND_AUDIO_FRONTEND_IO_HPP
#define ION_FRONTEND_AUDIO_FRONTEND_IO_HPP

#include <QObject>

#include <ion/frontend_base.hpp>
#include "simple_playlist.hpp"


class QTabWidget;
class QTreeView;


namespace ion
{
namespace frontend
{


class audio_frontend_io:
	public frontend_base < simple_playlist >
{
public:
	typedef frontend_base < simple_playlist > base_t;


	explicit audio_frontend_io(send_line_to_backend_callback_t const &send_line_to_backend_callback);

	void pause(bool const set);
	bool is_paused() const;

	void set_current_volume(long const new_volume);
	void set_current_position(unsigned int const new_position);
	void issue_get_position_command();
	unsigned int get_current_position() const;

	void play(uri const &uri_);
	void stop();


protected:
	void parse_command(std::string const &event_command_name, params_t const &event_params);

	bool paused;
	unsigned int current_position;
};


}
}


#endif

