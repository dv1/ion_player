#ifndef ION_BACKEND_BACKEND_DUMB_DECODER_HPP
#define ION_BACKEND_BACKEND_DUMB_DECODER_HPP

#include <boost/thread/mutex.hpp>
#include <dumb.h>
#include "source.hpp"
#include "decoder.hpp"
#include "decoder_creator.hpp"


namespace ion
{
namespace backend
{


class dumb_decoder:
	public decoder
{
public:
	enum module_type
	{
		module_type_unknown,

		module_type_xm,
		module_type_it,
		module_type_s3m,
		module_type_mod
	};


	explicit dumb_decoder(message_callback_t const message_callback, source_ptr_t source_, long const filesize);
	~dumb_decoder();


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

	virtual unsigned int get_decoder_samplerate() const;

	virtual unsigned int update(void *dest, unsigned int const num_samples_to_write);


	struct loop_data
	{
		long loop_mode, cur_num_loops;
	};


protected:
	bool test_if_module_file();
	void reinitialize_sigrenderer(unsigned const int new_num_channels, long const new_position);
	void set_loop_mode_impl(int const new_loop_mode);


	mutable boost::mutex mutex_;
	DUH *duh;
	DUH_SIGRENDERER *duh_sigrenderer;
	playback_properties playback_properties_;
	module_type module_type_;
	long current_volume;
	loop_data loop_data_;
	source_ptr_t source_;
};




class dumb_decoder_creator:
	public decoder_creator
{
public:
	virtual decoder_ptr_t create(source_ptr_t source_, metadata_t const &metadata, message_callback_t const &message_callback);
	virtual std::string get_type() const { return "dumb"; }
};


}
}


#endif

