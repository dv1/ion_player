#ifndef ION_AUDIO_PLAYER_SCAN_INDICATOR_ICON_HPP
#define ION_AUDIO_PLAYER_SCAN_INDICATOR_ICON_HPP

#include <QLabel>


class QMenu;
class QMovie;


namespace ion
{
namespace audio_player
{


class scan_indicator_icon:
	public QLabel
{
	Q_OBJECT
public:
	explicit scan_indicator_icon(QWidget *parent);

	QAction* get_open_scanner_dialog_action() { return open_scanner_dialog_action; }
	QAction* get_cancel_action() { return cancel_action; }

public slots:
	void set_running(bool running);
	void open_custom_context_menu(QPoint const &pos);

protected:

	QMenu *context_menu;
	QMovie *busy_indicator_movie;
	QAction *open_scanner_dialog_action;
	QAction *cancel_action;
};


}
}


#endif

