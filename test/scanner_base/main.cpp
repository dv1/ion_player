#include <QCoreApplication>

#include "test_scanner.hpp"


int main(int argc, char **argv)
{
	int return_value = 0;

	{
		QCoreApplication application(argc, argv);
		application.setApplicationName("scanner_test");

		int i = 5;
		test_scanner test_scanner_(3);
		test_scanner_.start_scan(i, ion::uri("file://test/sound_samples/mods/test.xm?id=1"));
		test_scanner_.start_scan(i, ion::uri("file://test/sound_samples/mods/test.xm?id=2"));
		test_scanner_.start_scan(i, ion::uri("file://test/sound_samples/mods/test.xm?id=3"));
		test_scanner_.start_scan(i, ion::uri("file://test/sound_samples/mods/test.xm?id=4"));

		return_value = application.exec();
	}


	return return_value;
}

