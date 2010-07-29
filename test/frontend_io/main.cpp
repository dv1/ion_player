#include <QCoreApplication>

#include "testclass.hpp"


int main(int argc, char **argv)
{
	int return_value = 0;

	{
		QCoreApplication application(argc, argv);
		application.setApplicationName("frontend_io_test");

		testclass testclass_;

		return_value = application.exec();
	}


	return return_value;
}

