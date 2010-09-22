#include <iostream>
#include <boost/thread/locks.hpp>
#include "gme_decoder.hpp"


namespace ion
{
namespace audio_backend
{


gme_decoder::gme_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_):
	decoder(send_command_callback),
	source_(source_),
	initialized(false),
	emu(0)
{
	// Misc checks & initializations

	if (!source_)
		return; // no source available? Nothing can be done in this decoder.

	long source_size = source_->get_size();
	if (source_size == 0)
		return; // 0 bytes? we cannot use this.

	initialized = true;
}


gme_decoder::~gme_decoder()
{
	delete emu;
}


bool gme_decoder::is_initialized() const
{
	return initialized;
}


bool gme_decoder::can_playback() const
{
	return is_initialized();
}


void gme_decoder::pause()
{
}


void gme_decoder::resume()
{
}


long gme_decoder::set_current_position(long const new_position)
{
}


long gme_decoder::get_current_position() const
{
}


long gme_decoder::set_current_volume(long const new_volume)
{
	return max_volume(); // TODO:
}


long gme_decoder::get_current_volume() const
{
	return max_volume(); // TODO:
}


metadata_t gme_decoder::get_metadata() const
{
}


std::string gme_decoder::get_type() const
{
	return "gme";
}


uri gme_decoder::get_uri() const
{
	return (source_) ? source_->get_uri() : uri();
}


long gme_decoder::get_num_ticks() const
{
}


long gme_decoder::get_num_ticks_per_second() const
{
}


void gme_decoder::set_loop_mode(int const new_loop_mode)
{
}


void gme_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
}


unsigned int gme_decoder::get_decoder_samplerate() const
{
	return 0;
}


unsigned int gme_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
}




gme_decoder_creator::gme_decoder_creator()
{
}


decoder_ptr_t gme_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback, std::string const &mime_type)
{
	gme_decoder *gme_decoder_ = new gme_decoder(send_command_callback, source_);
	if (!gme_decoder_->is_initialized())
	{
		delete gme_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(gme_decoder_);
}


}
}

