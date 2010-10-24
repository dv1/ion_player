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

		test_scanner test_scanner_(playlists, 4);
		test_scanner_.issue_scan_request(ion::uri("file://test/sound_samples/mods/test.xm?id=1"), p1);
		test_scanner_.issue_scan_request(ion::uri("file://test/sound_samples/mods/nebularia_-_ultima_ratio.it?id=515"), p2);
		test_scanner_.issue_scan_request(ion::uri("file://test/sound_samples/mods/test.xm?id=2"), p2);
		test_scanner_.issue_scan_request(ion::uri("file://test/sound_samples/mods/test.xm?id=3"), p3);
		test_scanner_.issue_scan_request(ion::uri("file://test/sound_samples/adplug-examples/ZAKMIX4.D00"), p3);
		test_scanner_.issue_scan_request(ion::uri("file://test/sound_samples/mods/test.xm?id=4"), p4);

		return_value = application.exec();
	}


	return return_value;
}

