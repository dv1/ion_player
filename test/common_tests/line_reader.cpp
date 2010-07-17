#include "test.hpp"
#include <string>
#include <stdio.h>
#include <ion/line_reader.hpp>


int test_main(int, char **)
{
	{
		char const *lines[] = {
			"Test ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890",
			"x",
			"abc",
			"foo bar",
			0
		};


		FILE *tmpf = tmpfile();

		for (char const **line = lines; (*line) != 0; ++line)
			fputs((std::string(*line) + "\n").c_str(), tmpf);
		fseek(tmpf, 0, SEEK_SET);

		int fd = fileno(tmpf);
		ion::line_reader lr(11);
		std::string tmpl;

		int lineno = 1;
		for (char const **line = lines; (*line) != 0; ++line, ++lineno)
		{
			TEST_ASSERT((tmpl = lr(fd)) == (*line), "Line " << lineno << ": expected \"" << (*line) << "\", got \"" << tmpl << "\"");
		}

		fclose(tmpf);
	}


	return 0;
}


INIT_TEST

