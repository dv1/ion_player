#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP


#include <cmath>
#include <iostream>
#include <memory>


namespace ion
{
namespace test
{



struct test_assert_exc
{
};

struct args_data
{
	int argc;
	char **argv;
};


}
}



#define TEST_ASSERT(EXPR, MSG) \
{ \
	if (!(EXPR)) \
	{ \
		std::cerr << "TEST FAILURE: Expression \"" << #EXPR << "\" at " << __FILE__ << ":" << __LINE__ << " failed: " << MSG << std::endl; \
		throw ion::test::test_assert_exc(); \
	} \
}

#define TEST_EXCEPTION(EXC, EXPR, MSG) \
{ \
	try \
	{ \
		{ \
			EXPR; \
		} \
		std::cerr << "TEST FAILURE: Exception \"" << #EXC << "\" was not thrown at "  << __FILE__ << ":" << __LINE__ << ": " << MSG << std::endl; \
		throw ion::test::test_assert_exc(); \
	} \
	catch (EXC const &) \
	{ \
	} \
} \

#define TEST_NO_EXCEPTION(EXPR, MSG) \
{ \
	try \
	{ \
		EXPR; \
	} \
	catch (...) \
	{ \
		std::cerr << "TEST FAILURE: Exception was thrown at "  << __FILE__ << ":" << __LINE__ << ": " << MSG << std::endl; \
		throw ion::test::test_assert_exc(); \
	} \
} \





#define INIT_TEST     \
namespace ion     \
{     \
namespace test     \
{     \
     \
int test_start(void *ptr)     \
{     \
	std::auto_ptr < args_data > data(reinterpret_cast < args_data* > (ptr));     \
	try   \
	{   \
		return test_main(data->argc, data->argv);     \
	}  \
	catch (ion::test::test_assert_exc)   \
	{   \
		std::cerr << "TEST FAILED" << std::endl; \
		return -1;   \
	}  \
}     \
     \
}     \
}     \
     \
int main(int argc, char **argv)     \
{     \
	ion::test::args_data *data = new ion::test::args_data;     \
	data->argc = argc;     \
	data->argv = argv;     \
     \
     return test_start(data); \
}


#endif

