#include <iostream>
#include <QDesktopWidget>
#include "main_window.hpp"
#include "settings.hpp"


namespace ion
{
namespace frontend
{


settings::settings(main_window &main_window_, QObject *parent):
	QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName(), parent),
	main_window_(main_window_)
{
	std::cout << "organization: " << QCoreApplication::organizationName().toStdString() << std::endl;
	std::cout << "application:  " << QCoreApplication::applicationName().toStdString() << std::endl;
	restore_geometry();
}


settings::~settings()
{
	save_geometry();
}


QString settings::get_backend_filepath() const
{
	return value("backend_filepath", QString("")).value < QString > ();
}


void settings::set_backend_filepath(QString const &new_filepath)
{
	setValue("backend_filepath", new_filepath);
}


void settings::restore_geometry()
{
	beginGroup("main_window");
	main_window_.resize(value("size", QSize(600, 400)).toSize());
	main_window_.move(value("pos", QApplication::desktop()->screenGeometry().topLeft() + QPoint(200, 200)).toPoint());
	endGroup();
}


void settings::save_geometry()
{
	beginGroup("main_window");
	setValue("size", main_window_.size());
	setValue("pos", main_window_.pos());
	endGroup();
}


}
}

