#include <QScrollBar>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include "backend_log_dialog.hpp"


namespace ion
{
namespace audio_player
{


backend_log_model::backend_log_model(QObject *parent, unsigned int const line_limit):
	QAbstractListModel(parent),
	line_limit(line_limit)
{
}


void backend_log_model::add_line(log_line_type const type, QString const &line)
{
	if (log_lines.size() == line_limit)
	{
		beginRemoveRows(QModelIndex(), 0, 0);
		log_lines.pop_front();
		endRemoveRows();
	}

	beginInsertRows(QModelIndex(), log_lines.size(), log_lines.size());
	log_lines.push_back(log_line_t(type, line));
	endInsertRows();
}


QVariant backend_log_model::headerData(int section, Qt::Orientation orientation, int role) const
{
	if ((orientation != Qt::Horizontal) || (role != Qt::DisplayRole))
		return QVariant();

	switch (section)
	{
		case 0: return "Type";
		case 1: return "Line";
		default: return QVariant();
	}
}


int backend_log_model::columnCount(QModelIndex const &) const
{
	return 2;
}


QVariant backend_log_model::data(QModelIndex const & index, int role) const
{
	if ((role != Qt::DisplayRole) || (index.column() > 1) || (index.row() >= int(log_lines.size())))
		return QVariant();

	log_line_t const & log_line = log_lines[index.row()];

	switch (index.column())
	{
		case 0:
		{
			switch (boost::fusion::at_c < 0 > (log_line))
			{
				case log_line_stdin: return QString("stdin");
				case log_line_stdout: return QString("stdout");
				case log_line_stderr: return QString("stderr");
				default: break;
			}
			break;
		}

		case 1:
			return boost::fusion::at_c < 1 > (log_line);

		default:
			break;
	}

	return QVariant();
}


QModelIndex backend_log_model::parent(QModelIndex const &) const
{
	return QModelIndex();
}


int backend_log_model::rowCount(QModelIndex const &) const
{
	return log_lines.size();
}




backend_log_dialog::backend_log_dialog(QWidget *parent, unsigned int const line_limit):
	QDialog(parent),
	scrollbar_at_maximum(false)
{
	backend_log_dialog_ui.setupUi(this);
	backend_log_model_ = new backend_log_model(this, line_limit);

	backend_log_dialog_ui.log_view->setModel(backend_log_model_);

	connect(backend_log_model_, SIGNAL(rowsAboutToBeInserted(QModelIndex const &, int, int)), this, SLOT(check_if_scrollbar_at_maximum(QModelIndex const &, int, int)));
	connect(backend_log_model_, SIGNAL(rowsInserted(QModelIndex const &, int, int)), this, SLOT(auto_scroll_to_bottom(QModelIndex const &, int, int)));
}


void backend_log_dialog::add_line(backend_log_model::log_line_type const type, QString const &line)
{
	backend_log_model_->add_line(type, line);
}


void backend_log_dialog::check_if_scrollbar_at_maximum(QModelIndex const &, int, int)
{
	QScrollBar *scroll_bar = backend_log_dialog_ui.log_view->verticalScrollBar();
	scrollbar_at_maximum = (scroll_bar->value() == scroll_bar->maximum());
}


void backend_log_dialog::auto_scroll_to_bottom(QModelIndex const &, int, int)
{
	if (scrollbar_at_maximum)
	{
		QScrollBar *scroll_bar = backend_log_dialog_ui.log_view->verticalScrollBar();
		scroll_bar->setValue(scroll_bar->maximum());
		scrollbar_at_maximum = false;
	}
}


}
}


#include "backend_log_dialog.moc"

