#include "test.hpp"
#include <ion/scope_guard.hpp>


int state = 0;

void start()
{
	state = 1;
}

void stop()
{
	state = 2;
}


struct foo
{
};


int test_main(int, char **)
{
	TEST_ASSERT(state == 0, "State is " << state);
	{
		ion::scope_guard sg(start, stop);
		TEST_ASSERT(state == 1, "State is " << state);
	}
	TEST_ASSERT(state == 2, "State is " << state);


	state = 0;


	try {
		ion::scope_guard sg(start, stop);
		TEST_ASSERT(state == 1, "State is " << state);

		throw foo();
	}
	catch (foo const &)
	{
		TEST_ASSERT(state == 2, "State is " << state);
	}

	TEST_ASSERT(state == 2, "State is " << state);



	return 0;
}



INIT_TEST

