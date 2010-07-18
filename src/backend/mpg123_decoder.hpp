#ifndef ION_BACKEND_BACKEND_MPG123_DECODER_HPP
#define ION_BACKEND_BACKEND_MPG123_DECODER_HPP

#include <stdint.h>
#include <vector>
#include <mpg123.h>
#include <boost/thread/mutex.hpp>

#include "source.hpp"
#include "decoder.hpp"
#include "decoder_creator.hpp"


namespace ion
{
namespace backend
{


class mpg123_decoder:
	public decoder
{
public:
	explicit mpg123_decoder(message_callback_t const message_callback, source_ptr_t source_);
	~mpg123_decoder();


	virtual bool is_initialized() const;
	virtual bool can_playback() const;

	virtual void pause();
	virtual void resume();

	virtual long set_current_position(long const new_position);
	virtual long get_current_position() const;

	virtual long set_current_volume(long const new_volume);
	virtual long get_current_volume() const;

	virtual Json::Value get_songinfo() const;

	virtual std::string get_type() const;

	virtual uri get_uri() const;

	virtual long get_num_ticks() const;
	virtual long get_num_ticks_per_second() const;

	virtual void set_loop_mode(int const new_loop_mode);

	virtual void set_playback_properties(playback_properties const &new_playback_properties);

	virtual unsigned int update(void *dest, unsigned int const num_samples_to_write);


protected:
	unsigned int update_resample(void *dest, unsigned int const num_samples_to_write);
	unsigned int update_no_resample(void *dest, unsigned int const num_samples_to_write);


	typedef std::vector < uint8_t > buffer_t;

	mutable boost::mutex mutex_;
	playback_properties playback_properties_;
	unsigned int src_frequency, src_num_channels;
	source_ptr_t source_;
	mpg123_handle *mpg123_handle_;
	void *resample_handle;
	buffer_t resample_buffer;
	buffer_t in_buffer, out_buffer;
	long current_volume;
	unsigned long const in_buffer_size, out_buffer_pad_size;
	int last_read_return_value;
	bool has_song_length;
};




class mpg123_decoder_creator:
	public decoder_creator
{
public:
	virtual decoder_ptr_t create(source_ptr_t source_, metadata_t const &metadata, message_callback_t const &message_callback);
	virtual std::string get_type() const { return "mpg123"; }
};


}
}


#endif

