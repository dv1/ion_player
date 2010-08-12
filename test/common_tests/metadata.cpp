#include "test.hpp"
#include <boost/foreach.hpp>
#include <set>
#include <cstdlib>
#include <ion/metadata.hpp>


int test_main(int, char **)
{
	{
		ion::metadata_t metadata_ = ion::empty_metadata();
		TEST_ASSERT(ion::is_valid(metadata_), "");
		TEST_VALUE(ion::get_metadata_string(metadata_), "{}\n");
	}

	{
		ion::metadata_optional_t metadata_ = ion::parse_metadata("}{");
		TEST_ASSERT(!metadata_, "");
	}

	{
		ion::metadata_optional_t metadata_ = ion::parse_metadata("5");
		TEST_ASSERT(!metadata_, "");
	}

	{
		ion::metadata_optional_t metadata_ = ion::parse_metadata("");
		TEST_ASSERT(metadata_, "");
		TEST_ASSERT(ion::is_valid(*metadata_), "");
		TEST_VALUE(ion::get_metadata_string(*metadata_), "{}\n");
	}

	{
		ion::metadata_optional_t metadata_ = ion::parse_metadata("{\"a\" : 5}");
		TEST_ASSERT(metadata_, "");
		TEST_ASSERT(ion::is_valid(*metadata_), "");
		TEST_VALUE(ion::get_metadata_value(*metadata_, "a", 0), 5);
		TEST_VALUE(ion::get_metadata_value(*metadata_, "b", 121), 121);
	}

	return 0;
}


INIT_TEST

