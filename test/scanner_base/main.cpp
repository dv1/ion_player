#include <QCoreApplication>
#include <stddef.h>

#include <ion/unique_ids.hpp>
#include <ion/flat_playlist.hpp>
#include "test_scanner.hpp"


int main(int argc, char **argv)
{
	int return_value = 0;

	{
		ion::unique_ids < long > ids;

		QCoreApplication application(argc, argv);
		application.setApplicationName("scanner_test");

		ion::flat_playlist p1(ids), p2(ids), p3(ids), p4(ids);

		std::cout << uintptr_t(&p1) << std::endl;
		std::cout << uintptr_t(&p2) << std::endl;
		std::cout << uintptr_t(&p3) << std::endl;
		std::cout << uintptr_t(&p4) << std::endl;

		test_scanner::playlists_t playlists;

		test_scanner test_scanner_(playlists, 3);
		test_scanner_.start_scan(p1, ion::uri("file://test/sound_samples/mods/test.xm?id=1"));
		test_scanner_.start_scan(p2, ion::uri("file://test/sound_samples/mods/test.xm?id=2"));
		test_scanner_.start_scan(p3, ion::uri("file://test/sound_samples/mods/test.xm?id=3"));
		test_scanner_.start_scan(p4, ion::uri("file://test/sound_samples/mods/test.xm?id=4"));

		return_value = application.exec();
	}


	return return_value;
}

