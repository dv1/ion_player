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


#ifndef ION_AUDIO_PLAYER_LOGGER_DIALOG_HPP
#define ION_AUDIO_PLAYER_LOGGER_DIALOG_HPP

#include <QDialog>
#include "ui_logger_dialog.h"


namespace ion
{
namespace audio_player
{


class logger_dialog:
	public QDialog 
{
public:
	explicit logger_dialog(QWidget *parent, unsigned int const line_limit = 1000);
	void add_line(QString const &type, QString const &line);


protected:
	Ui_logger_dialog logger_dialog_ui;
};


}
}


#endif

