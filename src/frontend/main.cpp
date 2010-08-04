#include <QApplication>
#include <QResource>
#include <iostream>

#include "main_window.hpp"


int main(int argc, char **argv)
{
#ifdef _MSC_VER
	Q_INIT_RESOURCE(resources);
#else
	Q_INIT_RESOURCE(resources_qrc);
#endif


	int return_value = 0;

	{
		QApplication application(argc, argv);
		application.setApplicationName("ion_player");

		ion::frontend::main_window *main_window_ = new ion::frontend::main_window;
		main_window_->show();
		return_value = application.exec();
		delete main_window_;
	}

#ifdef _MSC_VER
	Q_CLEANUP_RESOURCE(resources);
#else
	Q_CLEANUP_RESOURCE(resources_qrc);
#endif


	return return_value;
}

