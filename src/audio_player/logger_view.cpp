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


#include <QScrollBar>
#include "logger_view.hpp"


namespace ion
{
namespace audio_player
{


logger_view::logger_view(QWidget *parent):
	QTreeView(parent),
	logger_model_(0),
	set_scrollbar_at_maximum(false)
{
}


logger_view::logger_view(QWidget *parent, logger_model::line_limit_t line_limit):
	QTreeView(parent),
	logger_model_(0),
	set_scrollbar_at_maximum(false)
{
	initialize(line_limit);
}


void logger_view::initialize(logger_model::line_limit_t line_limit)
{
	if (logger_model_ != 0)
	{
		disconnect(logger_model_, SIGNAL(rowsAboutToBeInserted(QModelIndex const &, int, int)), this, SLOT(check_if_scrollbar_at_maximum(QModelIndex const &, int, int)));
		disconnect(logger_model_, SIGNAL(rowsInserted(QModelIndex const &, int, int)), this, SLOT(auto_scroll_to_bottom(QModelIndex const &, int, int)));
		delete logger_model_;
		logger_model_ = 0;
	}

	logger_model_ = new logger_model(this, line_limit);

	setModel(logger_model_);

	connect(logger_model_, SIGNAL(rowsAboutToBeInserted(QModelIndex const &, int, int)), this, SLOT(check_if_scrollbar_at_maximum(QModelIndex const &, int, int)));
	connect(logger_model_, SIGNAL(rowsInserted(QModelIndex const &, int, int)), this, SLOT(auto_scroll_to_bottom(QModelIndex const &, int, int)));
}


void logger_view::add_line(QString const &source, QString const &type, QString const &line)
{
	if (logger_model_ != 0)
		logger_model_->add_line(source, type, line);
}


void logger_view::clear()
{
	if (logger_model_ != 0)
		logger_model_->clear();
}


void logger_view::check_if_scrollbar_at_maximum(QModelIndex const &parent, int start_row, int end_row)
{
	QScrollBar *scroll_bar = verticalScrollBar();
	set_scrollbar_at_maximum = (scroll_bar->value() == scroll_bar->maximum());
}


void logger_view::auto_scroll_to_bottom(QModelIndex const &parent, int start_row, int end_row)
{
	if (set_scrollbar_at_maximum)
	{
		QScrollBar *scroll_bar = verticalScrollBar();
		scroll_bar->setValue(scroll_bar->maximum());
		set_scrollbar_at_maximum = false;
	}
}


}
}


#include "logger_view.moc"

