#include "test.hpp"
#include <boost/foreach.hpp>
#include <set>
#include <cstdlib>
#include <ion/unique_ids.hpp>


int test_main(int, char **)
{
	{
		std::srand(0);
		ion::unique_ids < int > ids;

		int id1 = ids.create_new();
		int id2 = ids.create_new();

		TEST_ASSERT(id1 != id2, "");
		TEST_ASSERT(ids.get_used_ids().size() == 2, "");
		TEST_ASSERT(ids.get_used_ids().find(id1) != ids.get_used_ids().end(), "");
		TEST_ASSERT(ids.get_used_ids().find(id2) != ids.get_used_ids().end(), "");

		ids.erase(id1);
		TEST_ASSERT(ids.get_used_ids().size() == 1, "");
		TEST_ASSERT(ids.get_used_ids().find(id1) == ids.get_used_ids().end(), "");
		TEST_ASSERT(ids.get_used_ids().find(id2) != ids.get_used_ids().end(), "");

		ids.insert(id1);
		TEST_ASSERT(ids.get_used_ids().size() == 2, "");
		TEST_ASSERT(ids.get_used_ids().find(id1) != ids.get_used_ids().end(), "");
		TEST_ASSERT(ids.get_used_ids().find(id2) != ids.get_used_ids().end(), "");

		int id3 = ids.create_new();
		TEST_ASSERT(id1 != id3, "");
		TEST_ASSERT(id2 != id3, "");
		TEST_ASSERT(ids.get_used_ids().size() == 3, "");
		TEST_ASSERT(ids.get_used_ids().find(id3) != ids.get_used_ids().end(), "");

		ids.insert(id1);
		TEST_ASSERT(ids.get_used_ids().size() == 3, "");
		TEST_ASSERT(ids.get_used_ids().find(id1) != ids.get_used_ids().end(), "");
		TEST_ASSERT(ids.get_used_ids().find(id2) != ids.get_used_ids().end(), "");
	}

	return 0;
}


INIT_TEST

