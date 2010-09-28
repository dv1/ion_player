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


#ifndef ION_AUDIO_COMMON_BACKEND_SOURCE_HPP
#define ION_AUDIO_COMMON_BACKEND_SOURCE_HPP

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <ion/uri.hpp>


namespace ion
{
namespace audio_common
{


class source:
	private boost::noncopyable
{
public:
	virtual ~source()
	{
	}

	enum seek_type
	{
		seek_absolute,
		seek_relative,
		seek_from_end
	};


	/*
	* Resets the source to its original state, resetting any internal statse, flushing any internal buffers and setting the internal read position to the beginning
	* of the source (if possible). Note that certain source types, such as broadcasts, cannot be fully reset; the contents that are read will be different. In any way,
	* this call tries to reset as much as possible.
	* Unlike seek(seek_absolute, 0) this also works when the end-of-data was reached.
	* @post source will be reset to the state it was when it was created (or as close as possible).
	*/
	virtual void reset() = 0;

	/*
	* Reads num_bytes bytes to dest, returning the amount of bytes actually read (this might be less, but never more than num_bytes).
	* If the source cannot read (because of a failure or end-of-file/stream), this function does nothing, and returns 0.
	* Be sure that the buffer pointed to by dest can hold at least num_bytes bytes!
	*
	* @param dest pointer to location where to send the read bytes to
	* @param num_bytes amount of bytes to read
	* @return the amount of actually read bytes; 0 if the call was unsuccessful, >0 otherwise. The source may have read less than what was specified by num_bytes,
	* but will never read *more* bytes than specified.
	* @pre num_bytes must be valid; dest must point to a valid location
	* @post The location pointed to by dest will have its first N bytes overwritten, where N is read()'s return value. If N < num_bytes, this function will only affect
	* the first N bytes, that is, the trailing (num_bytes - N) bytes will be left unchanged.
	*/
	virtual long read(void *dest, long const num_bytes) = 0;

	/*
	* Determines whether or not it is possible to read from this source, returning true if it is possible, false otherwise.
	* The return value may differ over time, particularly if an end-of-file/stream is encountered, making it useful for read loops.
	* For instance, it is a common method to call read() as long as can_read() returns true.
	* A return value of false can have multiple causes: end of data reached, fatal error occurred etc.
	* Generally speaking, if end_of_data_reached() returns true, this call will return false.
	* If is_ok() returns false, this call will return false.
	*
	* @return true if it is possible to read from this source, false otherwise
	*/
	virtual bool can_read() const = 0;

	/*
	* Determines whether or not the source reached the end of available data.
	* Note that this is expected to return true *after a reading operation detected the end-of-data*.
	* This means that checking this first, and then reading, will fail - it has to be the other way round.
	* once this returns true, the source may be left in an undefined state, meaning that operations like seeking won't
	* work properly. From this point on, use reset() to get the source back to a working state.
	* @return true if end of data was reached, false otherwise.
	*/
	virtual bool end_of_data_reached() const = 0;

	/*
	* Determines whether or not the source is in an operational state (an OK state).
	* If the source is in a non-operational state, this means an error occurred, and the source is no longer able to handle I/O requests
	* properly. Try using reset() to get it working again.
	* Note that end-of-data reached does not imply that the source is in a non-operational state.
	* @return true if the source is in an operational state, false otherwise.
	*/
	virtual bool is_ok() const = 0;

	/*
	* Seeks to the given position, using the given seek type.
	* - seek_absolute just sets the current position to the given one. If new_position is <0, the current position will be set to 0. if new_position >= size of file/stream,
	*   the current position will be set to the end of the file/stream.
	* - seek_relative adds new_position to the current position; thus, if new_position is negative, it will cause the current position to move backwards, while a positive
	*   new_position value causes a forward seek. If the sum of new_position and the current position is <0 or >= size of file/stream, the current position will be set like
	*   in the seek_absolute case.
	* - seek_from_end behaves similarly to seek_relative, except that new_position is added to the end position; new_position == 0 causes the source to go to the end-of-file/stream,
	*   negative values will cause a seek relative to this end-of-file/stream position. For example, new_position == -1 means the source will go to the last byte in the file/stream.
	*   Positive values are invalid.
	*
	* Not all sources may have the ability to seek, or can only support some seeking types. Use can_seek() to check. If seeking with the given type is not supported, or not supported
	* at all, this function will do nothing.
	*
	* @param new_position new position to use; see the explanation above for the meaning of this value
	* @param type seeking type to use
	* @pre seeking must be supported for the given type; end-of-stream was not reached yet
	* @post if the call was successful, the current position will have been adjusted as described above; if the call failed, nothing will have changed
	*/
	virtual void seek(long const new_position, seek_type const type) = 0;

	/*
	* Determines whether or not it is possible to seek in this source using the given type, returning true if it is possible, false otherwise.
	* The return value does not differ over time.
	*
	* @return true if it is possible to seek in this source using the given type, false otherwise
	*/
	virtual bool can_seek(seek_type const type) const = 0;

	/*
	* Returns the current position, in bytes. The current position will be affected by read() and seek(), but may not be affected by other, implementation specific means as well,
	* meaning it may not change over time if read() and seek() have not been called.
	* If the notion of a current position makes no sense for this source, -1 is returned. Note that this does NOT imply seek() will fail.
	* If the return value is -1, it will always be -1, and not differ over time.
	*
	* @return The current position, in bytes, or -1 if a current position is unavailable
	*/
	virtual long get_position() const = 0;

	/*
	* Returns the size of the data, in bytes. The size does not differ over time.
	* If the notion of a data size makes no sense for this source, -1 is returned.
	*
	* @return The size of the data, in bytes, or -1 if a data size is unavailable
	*/
	virtual long get_size() const = 0;

	/*
	* Returns the uri for this source.
	* @return The uri for this source
	*/
	virtual uri get_uri() const = 0;
};

typedef boost::shared_ptr < source > source_ptr_t;


}
}


#endif

