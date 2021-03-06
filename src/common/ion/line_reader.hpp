/****************************************************************************

Copyright (c) 2010 Carlos Rafael Giani

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

****************************************************************************/


#ifndef ION_LINE_READER_HPP
#define ION_LINE_READER_HPP

#include <algorithm>
#include <string>
#include <cstring>
#include <unistd.h>
#include <boost/noncopyable.hpp>


namespace ion
{


class line_reader
{
public:
	explicit line_reader(int const line_read_buf_size = 1024):
		line_read_buf_size(line_read_buf_size)
	{
		line_read_buf = new char[line_read_buf_size + 1];
		num_available = 0;
		previous_fd = -1;
	}


	~line_reader()
	{
		delete [] line_read_buf;
	}


	std::string operator()(int const fd, char const delimiter = '\n')
	{
		// flush buffer if fd changed
		if ((previous_fd != -1) && (previous_fd != fd))
			num_available = 0;


		std::string line;

		while (true)
		{
			if (num_available > 0)
			{
				int line_size = std::find(line_read_buf, line_read_buf + num_available, delimiter) - line_read_buf;
				line.append(line_read_buf, line_read_buf + line_size);

				if (line_size == num_available)
				{
					num_available = 0;
				}
				else
				{
					int move_offset = line_size + 1; // + 1 to skip the delimiter
					int num_to_move = num_available - move_offset;
					if (num_to_move > 0)
					{
						std::memmove(line_read_buf, line_read_buf + move_offset, num_to_move);
						num_available = num_to_move;
					}
					else
						num_available = 0;

					break;
				}
			}

			int num_read = read(fd, line_read_buf + num_available, line_read_buf_size - num_available);
			if (num_read <= 0)
			{
				if (num_available > 0)
					line.append(line_read_buf, line_read_buf + num_available);
				break;
			}
			else
				num_available += num_read;
		}

		return line;
	}




private:
	int const line_read_buf_size;
	char *line_read_buf;
	int num_available;
	int previous_fd;
};


}


#endif

