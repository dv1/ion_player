#include "test.hpp"
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <ion/flat_playlist.hpp>
#include <ion/filter_playlist.hpp>
#include <ion/playlists.hpp>


bool match(ion::playlist::entry_t const &entry)
{
	std::string title = ion::get_metadata_value < std::string > (boost::fusion::at_c < 1 > (entry), "title", "");
	return title.find_first_of('c') != std::string::npos;
}


void print_playlist(ion::playlist const &playlist_)
{
	for (ion::playlist::index_t i = 0; i < ion::get_num_entries(playlist_); ++i)
	{
		std::cout << i << ": " << boost::fusion::at_c < 0 > (*ion::get_entry(playlist_, i)).get_path() << std::endl;
	}
}


int test_main(int, char **)
{
	typedef ion::playlists < ion::flat_playlist > playlists_t;
	typedef ion::filter_playlist < playlists_t > filter_playlist_t;

	ion::flat_playlist::unique_ids_t unique_ids_;

	ion::flat_playlist *playlist1 = new ion::flat_playlist(unique_ids_);
	playlist1->add_entry(ion::flat_playlist::entry_t(ion::uri("foo://bar11"), *ion::parse_metadata("{ \"title\" : \"hello 441\" }"), false), false);
	playlist1->add_entry(ion::flat_playlist::entry_t(ion::uri("foo://bar12"), *ion::parse_metadata("{ \"title\" : \"abc def\" }"), false), false);
	playlist1->add_entry(ion::flat_playlist::entry_t(ion::uri("foo://bar13"), *ion::parse_metadata("{ \"title\" : \"lorem ipsum\" }"), false), false);
	playlist1->add_entry(ion::flat_playlist::entry_t(ion::uri("foo://bar14"), *ion::parse_metadata("{ \"title\" : \"max mustermann\" }"), false), false);

	ion::flat_playlist *playlist2 = new ion::flat_playlist(unique_ids_);
	playlist2->add_entry(ion::flat_playlist::entry_t(ion::uri("foo://bar21"), *ion::parse_metadata("{ \"title\" : \"ugauga\" }"), false), false);
	playlist2->add_entry(ion::flat_playlist::entry_t(ion::uri("foo://bar22"), *ion::parse_metadata("{ \"title\" : \"john smith\" }"), false), false);
	playlist2->add_entry(ion::flat_playlist::entry_t(ion::uri("foo://bar23"), *ion::parse_metadata("{ \"title\" : \"test c55\" }"), false), false);
	playlist2->add_entry(ion::flat_playlist::entry_t(ion::uri("foo://bar24"), *ion::parse_metadata("{ \"title\" : \"xxycz\" }"), false), false);

	playlists_t playlists;
	playlists.add_playlist(playlists_t::playlist_ptr_t(playlist1));
	playlists.add_playlist(playlists_t::playlist_ptr_t(playlist2));

	filter_playlist_t filter_playlist(playlists, boost::phoenix::bind(&match, boost::phoenix::arg_names::arg1));


	TEST_VALUE(ion::get_num_entries(filter_playlist), 3);
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 0)).get_path(), "bar12");
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 1)).get_path(), "bar23");
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 2)).get_path(), "bar24");

	{
		// testing uri-based retrieval and incompatibility marking
		// since the playlist adds an id option to the URI, it is retrieved from one of the entries instead of created manually
		ion::uri test_uri = boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 1));
		TEST_ASSERT(ion::get_metadata_for(filter_playlist, test_uri), "");

		ion::mark_backend_resource_incompatibility(filter_playlist, test_uri, "test_backend");
		TEST_VALUE(boost::fusion::at_c < 2 > (*ion::get_entry(filter_playlist, 1)), true);
	}

	// The following test the signal relay in filter_playlist
	// Internally, if one of the playlists that have URIs in the filter playlist proxy entry list get altered,
	// the filter playlist is notified, and changes its proxy entry list accordingly.

	playlist1->add_entry(ion::flat_playlist::entry_t(ion::uri("foo://extra"), *ion::parse_metadata("{ \"title\" : \"ccccc\" }"), false), true);
	TEST_VALUE(ion::get_num_entries(filter_playlist), 4);
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 0)).get_path(), "bar12");
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 1)).get_path(), "extra");
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 2)).get_path(), "bar23");
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 3)).get_path(), "bar24");

	playlist2->remove_entry(*ion::get_entry(filter_playlist, 2), true);
	//print_playlist(filter_playlist);
	TEST_VALUE(ion::get_num_entries(filter_playlist), 3);
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 0)).get_path(), "bar12");
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 1)).get_path(), "extra");
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 2)).get_path(), "bar24");

	playlists.remove_playlist(*playlist1);
	TEST_VALUE(ion::get_num_entries(filter_playlist), 1);
	TEST_VALUE(boost::fusion::at_c < 0 > (*ion::get_entry(filter_playlist, 0)).get_path(), "bar24");


	return 0;
}


INIT_TEST

