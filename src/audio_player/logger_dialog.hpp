#ifndef ION_AUDIO_PLAYER_LOGGER_DIALOG_HPP
#define ION_AUDIO_PLAYER_LOGGER_DIALOG_HPP

#include <QDialog>
#include "ui_logger_dialog.h"


namespace ion
{
namespace audio_player
{


class logger_dialog:
	public QDialog 
{
public:
	explicit logger_dialog(QWidget *parent, unsigned int const line_limit = 1000);
	void add_line(QString const &type, QString const &line);


protected:
	Ui_logger_dialog logger_dialog_ui;
};


}
}


#endif

