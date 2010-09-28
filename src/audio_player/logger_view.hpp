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


#ifndef ION_AUDIO_PLAYER_LOGGER_VIEW_HPP
#define ION_AUDIO_PLAYER_LOGGER_VIEW_HPP

#include <QTreeView>
#include "logger_model.hpp"


namespace ion
{
namespace audio_player
{


class logger_view:
	public QTreeView
{
	Q_OBJECT
public:
	logger_view(QWidget *parent);
	logger_view(QWidget *parent, logger_model::line_limit_t line_limit);
	void initialize(logger_model::line_limit_t line_limit);
	void add_line(QString const &source, QString const &type, QString const &line);


public slots:
	void clear();


protected slots:
	void check_if_scrollbar_at_maximum(QModelIndex const &parent, int start_row, int end_row);
	void auto_scroll_to_bottom(QModelIndex const &parent, int start_row, int end_row);


protected:
	logger_model *logger_model_;
	bool set_scrollbar_at_maximum;
};


}
}


#endif

