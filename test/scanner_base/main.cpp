#include <QCoreApplication>
#include <stddef.h>

#include "test_scanner.hpp"


int main(int argc, char **argv)
{
	int return_value = 0;

	{
		QCoreApplication application(argc, argv);
		application.setApplicationName("scanner_test");

		int i1 = 1, i2 = 2, i3 = 3, i4 = 4;

		std::cout << uintptr_t(&i1) << std::endl;
		std::cout << uintptr_t(&i2) << std::endl;
		std::cout << uintptr_t(&i3) << std::endl;
		std::cout << uintptr_t(&i4) << std::endl;

		test_scanner test_scanner_(3);
		test_scanner_.start_scan(i1, ion::uri("file://test/sound_samples/mods/test.xm?id=1"));
		test_scanner_.start_scan(i2, ion::uri("file://test/sound_samples/mods/test.xm?id=2"));
		test_scanner_.start_scan(i3, ion::uri("file://test/sound_samples/mods/test.xm?id=3"));
		test_scanner_.start_scan(i4, ion::uri("file://test/sound_samples/mods/test.xm?id=4"));

		return_value = application.exec();
	}


	return return_value;
}

