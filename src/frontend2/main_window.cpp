#include <ctime>
#include <cstdlib>

#include <QTimer>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSystemTrayIcon>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "main_window.hpp"

#include "../backend/decoder.hpp"


namespace ion
{
namespace frontend
{


main_window::main_window(uri_optional_t const &command_line_uri)
{
	std::srand(std::time(0));

	main_window_ui.setupUi(this);

	QWidget *sliders_widget = new QWidget(this);
	position_volume_widget_ui.setupUi(sliders_widget);
	main_window_ui.sliders_toolbar->addWidget(sliders_widget);

	settings_dialog = new QDialog(this);
	settings_dialog_ui.setupUi(settings_dialog);

	QToolButton *create_playlist_button = new QToolButton(this);
	create_playlist_button->setDefaultAction(main_window_ui.action_create_new_tab);
	main_window_ui.playlist_tab_widget->setCornerWidget(create_playlist_button);
}


main_window::~main_window()
{
}


}
}


#include "main_window.moc"

