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


#include "logger_dialog.hpp"


namespace ion
{
namespace audio_player
{


logger_dialog::logger_dialog(QWidget *parent, unsigned int const line_limit):
	QDialog(parent)
{
	logger_dialog_ui.setupUi(this);
	logger_dialog_ui.log_view->initialize(1000);
}


void logger_dialog::add_line(QString const &type, QString const &line)
{
	logger_dialog_ui.log_view->add_line("backend", type, line);
}


}
}

