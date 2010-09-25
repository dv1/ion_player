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

