#include "logger_model.hpp"


namespace ion
{
namespace audio_player
{


logger_model::logger_model(QObject *parent, line_limit_t const line_limit):
	QAbstractListModel(parent),
	line_limit(line_limit)
{
}


void logger_model::add_line(QString const &source, QString const &type, QString const &line)
{
	if (line_limit)
	{
		if (log_lines.size() == *line_limit)
		{
			beginRemoveRows(QModelIndex(), 0, 0);
			log_lines.pop_front();
			endRemoveRows();
		}
	}

	beginInsertRows(QModelIndex(), log_lines.size(), log_lines.size());
	log_lines.push_back(log_line(source, type, line));
	endInsertRows();
}


void logger_model::clear()
{
	log_lines.clear();
	reset();
}


QVariant logger_model::headerData(int section, Qt::Orientation orientation, int role) const
{
	if ((orientation != Qt::Horizontal) || (role != Qt::DisplayRole))
		return QVariant();

	switch (section)
	{
		case 0: return "Source";
		case 1: return "Type";
		case 2: return "Line";
		default: return QVariant();
	}
}


int logger_model::columnCount(QModelIndex const & parent) const
{
	return 3;
}


QVariant logger_model::data(QModelIndex const & index, int role) const
{
	if ((role != Qt::DisplayRole) || (index.column() > 2) || (index.row() >= int(log_lines.size())))
		return QVariant();

	log_line const & log_line_ = log_lines[index.row()];

	switch (index.column())
	{
		case 0: return log_line_.source;
		case 1: return log_line_.type;
		case 2: return log_line_.line;
		default:
			break;
	}

	return QVariant();
}


QModelIndex logger_model::parent(QModelIndex const & index) const
{
	return QModelIndex();
}


int logger_model::rowCount(QModelIndex const & parent) const
{
	return log_lines.size();
}


}
}


#include "logger_model.moc"

