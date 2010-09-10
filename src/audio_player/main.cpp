#include <QApplication>
#include <QFileInfo>
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

		ion::uri_optional_t command_line_uri = boost::none;
		if (argc >= 2)
		{
			try
			{
				QString filename = QString("file://") + QFileInfo(QString(argv[1])).canonicalFilePath();
				command_line_uri = ion::uri(filename.toStdString());
			}
			catch (ion::uri::invalid_uri const &e)
			{
				std::cerr << "The supplied URI \"" << e.what() << "\" is invalid" << std::endl;
			}
		}

		ion::audio_player::main_window *main_window_ = new ion::audio_player::main_window(command_line_uri);
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

