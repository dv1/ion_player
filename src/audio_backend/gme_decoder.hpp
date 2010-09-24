#ifndef ION_AUDIO_BACKEND_BACKEND_GME_DECODER_HPP
#define ION_AUDIO_BACKEND_BACKEND_GME_DECODER_HPP

#include "decoder.hpp"
#include <boost/thread/mutex.hpp>
#include "decoder_creator.hpp"


namespace ion
{
namespace audio_backend
{


class gme_decoder:
	public decoder
{
public:
	explicit gme_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_);
	~gme_decoder();


	virtual bool is_initialized() const;
	virtual bool can_playback() const;

	virtual void pause();
	virtual void resume();

	virtual long set_current_position(long const new_position);
	virtual long get_current_position() const;

	virtual long set_current_volume(long const new_volume);
	virtual long get_current_volume() const;

	virtual metadata_t get_metadata() const;

	virtual std::string get_type() const;

	virtual uri get_uri() const;

	virtual long get_num_ticks() const;
	virtual long get_num_ticks_per_second() const;

	virtual void set_loop_mode(int const new_loop_mode);

	virtual void set_playback_properties(playback_properties const &new_playback_properties);

	virtual unsigned int get_decoder_samplerate() const;

	virtual unsigned int update(void *dest, unsigned int const num_samples_to_write);


protected:
	bool reset_emu(unsigned int const sample_rate);
	void set_fade();


	mutable boost::mutex mutex_;
	source_ptr_t source_;
	playback_properties playback_properties_;
	unsigned int track_nr;

	struct internal_data;
	internal_data *internal_data_;
};




class gme_decoder_creator:
	public decoder_creator
{
public:
	explicit gme_decoder_creator();

	virtual decoder_ptr_t create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback, std::string const &mime_type);
	virtual std::string get_type() const { return "gme"; }
};


}
}


#endif
