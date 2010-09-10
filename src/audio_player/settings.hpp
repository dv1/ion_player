#ifndef ION_AUDIO_PLAYER_SETTINGS_HPP
#define ION_AUDIO_PLAYER_SETTINGS_HPP

#include <QSettings>


namespace ion
{
namespace audio_player
{


class main_window;


class settings:
	public QSettings 
{
public:
	explicit settings(main_window &main_window_, QObject *parent = 0);
	~settings();

	QString get_backend_filepath() const;
	QString get_singleplay_playlist() const;
	bool get_always_on_top_flag() const;
	bool get_on_all_workspaces_flag() const;
	bool get_systray_icon_flag() const;

	void set_backend_filepath(QString const &new_filepath);
	void set_singleplay_playlist(QString const &new_singleplay_playlist);
	void set_always_on_top_flag(bool const new_flag);
	void set_on_all_workspaces_flag(bool const new_flag);
	void set_systray_icon_flag(bool const new_flag);


protected:
	void restore_geometry();
	void save_geometry();

	main_window &main_window_;
};


}
}


#endif

