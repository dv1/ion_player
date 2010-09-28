/**************************************************************************

    Copyright (C) 2010  Carlos Rafael Giani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**************************************************************************/


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

