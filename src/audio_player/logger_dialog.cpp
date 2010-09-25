#include "logger_dialog.hpp"


namespace ion
{
namespace audio_player
{


logger_dialog::logger_dialog(QWidget *parent, unsigned int const line_limit):
	QDialog(parent)
{
	logger_dialog_ui.setupUi(this);
	logger_dialog_ui.log_view->initialize(1000);
}


void logger_dialog::add_line(QString const &type, QString const &line)
{
	logger_dialog_ui.log_view->add_line("backend", type, line);
}


}
}

