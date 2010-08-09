#ifndef ION_FRONTEND_SETTINGS_HPP
#define ION_FRONTEND_SETTINGS_HPP

#include <QSettings>


namespace ion
{
namespace frontend
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
	void set_backend_filepath(QString const &new_filepath);
	void set_singleplay_playlist(QString const &new_singleplay_playlist);


protected:
	void restore_geometry();
	void save_geometry();

	main_window &main_window_;
};


}
}


#endif

