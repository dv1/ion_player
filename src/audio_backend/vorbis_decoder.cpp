#include <boost/thread/locks.hpp>
#include "vorbis_decoder.hpp"


namespace ion
{
namespace audio_backend
{


vorbis_decoder::vorbis_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_, metadata_t const &initial_metadata):
	decoder(send_command_callback)
{
}


vorbis_decoder::~vorbis_decoder()
{
}


bool vorbis_decoder::is_initialized() const
{
}


bool vorbis_decoder::can_playback() const
{
}


void vorbis_decoder::pause()
{
}


void vorbis_decoder::resume()
{
}


long vorbis_decoder::set_current_position(long const new_position)
{
}


long vorbis_decoder::get_current_position() const
{
}


long vorbis_decoder::set_current_volume(long const new_volume)
{
}


long vorbis_decoder::get_current_volume() const
{
}


metadata_t vorbis_decoder::get_metadata() const
{
}


std::string vorbis_decoder::get_type() const
{
	return "vorbis";
}


uri vorbis_decoder::get_uri() const
{
}


long vorbis_decoder::get_num_ticks() const
{
}


long vorbis_decoder::get_num_ticks_per_second() const
{
}


void vorbis_decoder::set_loop_mode(int const new_loop_mode)
{
}


void vorbis_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
}


unsigned int vorbis_decoder::get_decoder_samplerate() const
{
}


unsigned int vorbis_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
}




decoder_ptr_t vorbis_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback, std::string const &mime_type)
{
	if ((mime_type != "application/ogg") || (mime_type != "application/x-ogg"))
		return decoder_ptr_t();


}


}
}

