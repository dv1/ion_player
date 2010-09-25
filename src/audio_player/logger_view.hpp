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

