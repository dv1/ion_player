#ifndef ION_AUDIO_PLAYER_BACKEND_LOG_DIALOG_HPP
#define ION_AUDIO_PLAYER_BACKEND_LOG_DIALOG_HPP

#include <deque>
#include <boost/fusion/container/vector.hpp>
#include <QAbstractListModel>
#include <QDialog>
#include <QVariant>
#include "ui_backend_log_dialog.h"


// TODO: such a log dialog is useful in other areas as well. Make it more generic and reusable.


namespace ion
{
namespace audio_player
{


class backend_log_model:
	public QAbstractListModel
{
public:
	enum log_line_type
	{
		log_line_stdin,
		log_line_stdout,
		log_line_stderr,
		log_line_misc
	};
	typedef boost::fusion::vector2 < log_line_type, QString > log_line_t;
	typedef std::deque < log_line_t > log_lines_t;


	explicit backend_log_model(QObject *parent, unsigned int const line_limit);

	void add_line(log_line_type const type, QString const &line);

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int columnCount(QModelIndex const & parent) const;
	virtual QVariant data(QModelIndex const & index, int role) const;
	virtual QModelIndex parent(QModelIndex const & index) const;
	virtual int rowCount(QModelIndex const & parent) const;

protected:
	unsigned int line_limit;
	log_lines_t log_lines;
};


class backend_log_dialog:
	public QDialog 
{
	Q_OBJECT
public:
	explicit backend_log_dialog(QWidget *parent, unsigned int const line_limit = 1000);
	void add_line(backend_log_model::log_line_type const type, QString const &line);


protected slots:
	void check_if_scrollbar_at_maximum(QModelIndex const &parent, int start_row, int end_row);
	void auto_scroll_to_bottom(QModelIndex const &parent, int start_row, int end_row);


protected:
	Ui_backend_log_dialog backend_log_dialog_ui;
	backend_log_model *backend_log_model_;
	bool scrollbar_at_maximum;
};


}
}


#endif

