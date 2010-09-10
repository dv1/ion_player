#ifndef ION_AUDIO_PLAYER_AUDIO_FRONTEND_HPP
#define ION_AUDIO_PLAYER_AUDIO_FRONTEND_HPP

#include <QObject>

#include <ion/frontend_base.hpp>
#include <ion/playlist.hpp>


namespace ion
{
namespace audio_player
{


class audio_frontend:
	public frontend_base < playlist >
{
public:
	typedef frontend_base < playlist > base_t;


	explicit audio_frontend(send_line_to_backend_callback_t const &send_line_to_backend_callback);

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

