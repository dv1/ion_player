#include <iostream>
#include <boost/thread/locks.hpp>
#include "vorbis_decoder.hpp"


namespace ion
{
namespace audio_backend
{


namespace
{


size_t vorbis_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	source *source_ = reinterpret_cast < source* > (datasource);
	return source_->read(ptr, size * nmemb);
}


int vorbis_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
	source *source_ = reinterpret_cast < source* > (datasource);
	switch (whence)
	{
		case SEEK_SET:
			if (!source_->can_seek(source::seek_absolute))
				return -1;
			else
				source_->seek(offset, source::seek_absolute);
			break;

		case SEEK_CUR:
			if (!source_->can_seek(source::seek_relative))
				return -1;
			else
				source_->seek(offset, source::seek_relative);
			break;

		case SEEK_END:
			if (!source_->can_seek(source::seek_from_end))
				return -1;
			else
				source_->seek(offset, source::seek_from_end);
			break;
		default: break;
	}

	return source_->get_position();
}


int vorbis_close_func(void *datasource)
{
	source *source_ = reinterpret_cast < source* > (datasource);
	source_->reset();
	return 0;
}


long vorbis_tell_func(void *datasource)
{
	source *source_ = reinterpret_cast < source* > (datasource);
	return source_->get_position();
}


} // unnamed namespace end



vorbis_decoder::vorbis_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_):
	decoder(send_command_callback),
	source_(source_),
	info(0),
	initialized(false)
{
	// Misc checks & initializations

	if (!source_)
		return; // no source available? Nothing can be done in this decoder.

	long source_size = source_->get_size();
	if (source_size == 0)
		return; // 0 bytes? we cannot use this.


	// install IO callbacks

	ov_callbacks io_callbacks;
	io_callbacks.read_func = vorbis_read_func;
	io_callbacks.close_func = vorbis_close_func;

	if (source_->can_seek(source::seek_absolute) || source_->can_seek(source::seek_relative) || source_->can_seek(source::seek_from_end))
	{
		io_callbacks.seek_func = vorbis_seek_func;
		io_callbacks.tell_func = vorbis_tell_func;
	}
	else
	{
		io_callbacks.seek_func = 0;
		io_callbacks.tell_func = 0;
	}

	int ret = ov_open_callbacks(source_.get(), &vorbis_file, NULL, 0, io_callbacks);
	if (ret != 0)
		return;

	info = ov_info(&vorbis_file, -1);
	if (info == 0)
		return;

	initialized = true;
}


vorbis_decoder::~vorbis_decoder()
{
	ov_clear(&vorbis_file);
}


bool vorbis_decoder::is_initialized() const
{
	return initialized && source_;
}


bool vorbis_decoder::can_playback() const
{
	return is_initialized();
}


void vorbis_decoder::pause()
{
}


void vorbis_decoder::resume()
{
}


long vorbis_decoder::set_current_position(long const new_position)
{
	if (!can_playback())
		return -1;

	if ((new_position < 0) || (new_position >= get_num_ticks()))
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);

	ov_pcm_seek(&vorbis_file, new_position);
	return ov_pcm_tell(&vorbis_file);
}


long vorbis_decoder::get_current_position() const
{
	if (!can_playback())
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);
	return ov_pcm_tell(&vorbis_file);
}


long vorbis_decoder::set_current_volume(long const new_volume)
{
	return max_volume(); // TODO:
}


long vorbis_decoder::get_current_volume() const
{
	return max_volume(); // TODO:
}


namespace
{


void set_metadata_from_vorbis_comment(metadata_t &metadata_, char const *field_name, vorbis_comment *comment)
{
	if (comment == 0)
		return;

	char *data = vorbis_comment_query(comment, field_name, 0);
	if (data != 0)
		set_metadata_value(metadata_, field_name, data);
}


}


metadata_t vorbis_decoder::get_metadata() const
{
	metadata_t metadata_ = empty_metadata();

	// NOTE: not using can_playback() here, since it is valid to call get_songinfo() without any playback going on
	if (is_initialized())
	{
		vorbis_comment *comment = ov_comment(&vorbis_file, -1);
		set_metadata_from_vorbis_comment(metadata_, "title", comment);
		set_metadata_from_vorbis_comment(metadata_, "artist", comment);
		set_metadata_from_vorbis_comment(metadata_, "album", comment);
	}

	return metadata_;
}


std::string vorbis_decoder::get_type() const
{
	return "vorbis";
}


uri vorbis_decoder::get_uri() const
{
	return (source_) ? source_->get_uri() : uri();
}


long vorbis_decoder::get_num_ticks() const
{
	return ov_pcm_total(&vorbis_file, -1);
}


long vorbis_decoder::get_num_ticks_per_second() const
{
	return info->rate;
}


void vorbis_decoder::set_loop_mode(int const new_loop_mode)
{
}


void vorbis_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	if (!is_initialized())
		return;

	boost::lock_guard < boost::mutex > lock(mutex_);

	playback_properties_ = new_playback_properties;
}


unsigned int vorbis_decoder::get_decoder_samplerate() const
{
	return info->rate;
}


unsigned int vorbis_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	int bytes_per_tick = 0;

	{
		boost::lock_guard < boost::mutex > lock(mutex_);
		bytes_per_tick = playback_properties_.num_channels * get_sample_size(playback_properties_.sample_type_);
	}

#if 1
	uint8_t *buf = reinterpret_cast < uint8_t* > (dest);
	long remaining_samples = num_samples_to_write;

	// TODO: re-read vorbis info when the section changes
	// since each section can have a different number of channels & different samplerate
	// Also, once the resampler works properly with the partial writes, get rid of the while loop - it wont be necessary anymore

	while (remaining_samples > 0)
	{
		long num_read_bytes = ov_read(&vorbis_file, reinterpret_cast < char* > (buf), remaining_samples * bytes_per_tick, 0, 2, 1, &current_section);
		bool continue_loop = true;

		switch (num_read_bytes)
		{
			case OV_HOLE:
			case OV_EBADLINK:
			case OV_EINVAL:
			case 0:
				continue_loop = false;
				break;
			default:
				break;
		}

		if (!continue_loop)
			break;

		long num_read_samples = num_read_bytes / bytes_per_tick;
		remaining_samples -= num_read_samples;
		buf += num_read_bytes;
	}

	return num_samples_to_write - remaining_samples;
#else
	long num_read_bytes = ov_read(&vorbis_file, reinterpret_cast < char* > (dest), num_samples_to_write * bytes_per_tick, 0, 2, 1, &current_section);
	std::cerr << (num_read_bytes / bytes_per_tick) << ' ' << num_samples_to_write << std::endl;

	switch (num_read_bytes)
	{
		case OV_HOLE:
		case OV_EBADLINK:
		case OV_EINVAL:
		case 0:
			return 0;
		default:
			break;
	}

	return num_read_bytes / bytes_per_tick;
#endif
}




vorbis_decoder_creator::vorbis_decoder_creator()
{
}


decoder_ptr_t vorbis_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback, std::string const &mime_type)
{
	if ((mime_type != "application/ogg") && (mime_type != "application/x-ogg"))
		return decoder_ptr_t();


	vorbis_decoder *vorbis_decoder_ = new vorbis_decoder(send_command_callback, source_);
	if (!vorbis_decoder_->is_initialized())
	{
		delete vorbis_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(vorbis_decoder_);
}


}
}

