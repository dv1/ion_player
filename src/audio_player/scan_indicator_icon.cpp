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


#include <QContextMenuEvent>
#include <QMenu>
#include <QMovie>
#include "scan_indicator_icon.hpp"


namespace ion
{
namespace audio_player
{


scan_indicator_icon::scan_indicator_icon(QWidget *parent):
	QLabel(parent)
{
	busy_indicator_movie = new QMovie(":/icons/busy_indicator", QByteArray(), this);
	setMovie(busy_indicator_movie);

	context_menu = new QMenu(this);
	open_scanner_dialog_action = context_menu->addAction("Show scanner dialog");
	cancel_action = context_menu->addAction("Cancel scan");

	set_running(false);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint const &)), this, SLOT(open_custom_context_menu(QPoint const &)));
}


void scan_indicator_icon::set_running(bool running)
{
	if (running)
	{
		setToolTip("Scanning");
		cancel_action->setEnabled(true);
		busy_indicator_movie->jumpToFrame(0);
		busy_indicator_movie->start();
	}
	else
	{
		setToolTip("Not scanning");
		busy_indicator_movie->stop();
		busy_indicator_movie->jumpToFrame(0);
		cancel_action->setEnabled(false);
	}
}


void scan_indicator_icon::open_custom_context_menu(QPoint const &pos)
{
	context_menu->popup(mapToGlobal(pos));
}


}
}


#include "scan_indicator_icon.moc"

