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


#ifndef ION_AUDIO_PLAYER_LOGGER_MODEL_HPP
#define ION_AUDIO_PLAYER_LOGGER_MODEL_HPP

#include <deque>
#include <boost/optional.hpp>
#include <QAbstractListModel>


namespace ion
{
namespace audio_player
{


class logger_model:
	public QAbstractListModel
{
	Q_OBJECT
public:
	struct log_line
	{
		QString source, type, line;


		log_line()
		{
		}


		log_line(QString const &source, QString const &type, QString const &line):
			source(source),
			type(type),
			line(line)
		{
		}
	};


	typedef std::deque < log_line > log_lines_t;
	typedef boost::optional < unsigned int > line_limit_t;


	explicit logger_model(QObject *parent, line_limit_t const line_limit);

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int columnCount(QModelIndex const & parent) const;
	virtual QVariant data(QModelIndex const & index, int role) const;
	virtual QModelIndex parent(QModelIndex const & index) const;
	virtual int rowCount(QModelIndex const & parent) const;


public slots:
	void add_line(QString const &source, QString const &type, QString const &line);
	void clear();


protected:
	boost::optional < unsigned int > line_limit;
	log_lines_t log_lines;
};


}
}


#endif

