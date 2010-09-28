/**************************************************************************

    Copyright (C) 2010  Carlos Rafael Giani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**************************************************************************/


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

