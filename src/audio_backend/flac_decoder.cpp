#include <FLAC++/decoder.h>
#include "flac_decoder.hpp"
#include <stdint.h>
#include <vector>


namespace ion
{
namespace audio_backend
{


// TODO: clean up this code, rewrite it if necessary & extract metadata from flac


class flac_decoder::custom_flac_decoder:
	public FLAC::Decoder::Stream
{
public:
	struct flac_metadata
	{
		FLAC__uint64 total_samples;
		unsigned int sample_rate;
		unsigned int channels;
		unsigned int bps;


		flac_metadata():
			total_samples(0),
			sample_rate(0),
			channels(0),
			bps(0)
		{
		}
	};

	typedef std::vector < int16_t > sample_buffer_t;



	explicit custom_flac_decoder(source& source_):
		source_(source_),
		ok(true)
	{
		FLAC__StreamDecoderInitStatus init_status = init();
		switch (init_status)
		{
			case FLAC__STREAM_DECODER_INIT_STATUS_OK:
			case FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED:
				ok = true;
				break;
			default:
				ok = false;
				break;
		}

		if (ok)
			process_until_end_of_metadata();
	}


	~custom_flac_decoder()
	{
		finish();
	}


	flac_metadata const & get_flac_metadata() const { return flac_metadata_; }
	bool is_ok() const { return ok; }
	sample_buffer_t & get_sample_buffer() { return sample_buffer; }


	bool end_of_stream() const
	{
		return get_state() == FLAC__STREAM_DECODER_END_OF_STREAM;
	}


protected:
	virtual ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes)
	{
		long l = source_.read(buffer, *bytes);

		if (source_.end_of_data_reached())
		{
			*bytes = 0;
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		}
		else if (!source_.is_ok())
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		else
		{
			*bytes = std::max(l, long(0));
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	}


	virtual ::FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset)
	{
		if (!source_.can_seek(source::seek_absolute))
			return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;

		source_.seek(absolute_byte_offset, source::seek_absolute);
		return source_.is_ok() ? FLAC__STREAM_DECODER_SEEK_STATUS_OK : FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}


	virtual ::FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset)
	{
		long position = source_.get_position();
		if (!source_.is_ok())
			return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
		else if (position < 0)
			return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
		else
		{
			*absolute_byte_offset = position;
			return FLAC__STREAM_DECODER_TELL_STATUS_OK;
		}
	}


	virtual ::FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length)
	{
		long size = source_.get_size();

		if (!source_.is_ok())
			return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
		else if (size == -1)
		{
			return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
		}
		else
		{
			*stream_length = size;
			return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
		}
	}


	virtual bool eof_callback()
	{
		return source_.end_of_data_reached();
	}


	virtual ::FLAC__StreamDecoderWriteStatus write_callback(::FLAC__Frame const *frame, const FLAC__int32 * const buffer[])
	{
		if (end_of_stream())
		{
			std::cerr << sample_buffer.size() << ' ' << frame->header.blocksize << ' ' << frame->header.channels << std::endl;
			return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
		}

		unsigned long sample_offset = sample_buffer.size();
		sample_buffer.resize(sample_offset + frame->header.blocksize * frame->header.channels);

		int16_t *buf_pos = &sample_buffer[sample_offset];
		for (unsigned long sample_nr = 0; sample_nr < frame->header.blocksize; ++sample_nr)
		{
			for (unsigned long channel_nr = 0; channel_nr < frame->header.channels; ++channel_nr)
			{
				*buf_pos++ = buffer[channel_nr][sample_nr];
			}
		}

		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}


	virtual void metadata_callback(::FLAC__StreamMetadata const *metadata)
	{
		if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
		{
			flac_metadata_.total_samples = metadata->data.stream_info.total_samples;
			flac_metadata_.sample_rate = metadata->data.stream_info.sample_rate;
			flac_metadata_.channels = metadata->data.stream_info.channels;
			flac_metadata_.bps = metadata->data.stream_info.bits_per_sample;
		}
	}


	virtual void error_callback(::FLAC__StreamDecoderErrorStatus status)
	{
		ok = false;
	}



	source& source_;
	flac_metadata flac_metadata_;
	bool ok;
	sample_buffer_t sample_buffer;
};




flac_decoder::flac_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_):
	decoder(send_command_callback),
	source_(source_),
	custom_flac_decoder_(0),
	current_position(0),
	initialized(false)
{
	// Misc checks & initializations

	if (!source_)
		return; // no source available? Nothing can be done in this decoder.

	long source_size = source_->get_size();
	if (source_size == 0)
		return; // 0 bytes? we cannot use this.

	custom_flac_decoder_ = new custom_flac_decoder(*source_);
	if (!custom_flac_decoder_->is_ok())
		return;

	initialized = true;
}


flac_decoder::~flac_decoder()
{
	if (custom_flac_decoder_ != 0)
		delete custom_flac_decoder_;
}

bool flac_decoder::is_initialized() const
{
	return initialized && source_;
}


bool flac_decoder::can_playback() const
{
	return is_initialized();
}


void flac_decoder::pause()
{
}


void flac_decoder::resume()
{
}


long flac_decoder::set_current_position(long const new_position)
{
	if (!can_playback())
		return -1;

	if ((new_position < 0) || (new_position >= get_num_ticks()))
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);

	custom_flac_decoder_->seek_absolute(new_position);
	current_position = new_position;

	return new_position;
}


long flac_decoder::get_current_position() const
{
	boost::lock_guard < boost::mutex > lock(mutex_);
	return current_position;
}


long flac_decoder::set_current_volume(long const new_volume)
{
	return max_volume();
}


long flac_decoder::get_current_volume() const
{
	return max_volume();
}


metadata_t flac_decoder::get_metadata() const
{
	return empty_metadata();
}


std::string flac_decoder::get_type() const
{
	return "flac";
}


uri flac_decoder::get_uri() const
{
	return (source_) ? source_->get_uri() : uri();
}


long flac_decoder::get_num_ticks() const
{
	return custom_flac_decoder_->get_flac_metadata().total_samples;
}


long flac_decoder::get_num_ticks_per_second() const
{
	return custom_flac_decoder_->get_flac_metadata().sample_rate;
}


void flac_decoder::set_loop_mode(int const new_loop_mode)
{
}


void flac_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	if (!is_initialized())
		return;

	boost::lock_guard < boost::mutex > lock(mutex_);

	playback_properties_ = new_playback_properties;
}


unsigned int flac_decoder::get_decoder_samplerate() const
{
	return custom_flac_decoder_->get_flac_metadata().sample_rate;
}


unsigned int flac_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	while (custom_flac_decoder_->is_ok() && !custom_flac_decoder_->end_of_stream() && (custom_flac_decoder_->get_sample_buffer().size() < (num_samples_to_write * playback_properties_.num_channels)))
	{
		if (!custom_flac_decoder_->process_single())
			break;
	}

	long num_samples_to_return = std::min(
		long(custom_flac_decoder_->get_sample_buffer().size() / playback_properties_.num_channels),
		long(num_samples_to_write)
	);

/*	std::cerr
		<< (custom_flac_decoder_->end_of_stream() ? 'Y' : 'N') << ' '
		<< custom_flac_decoder_->get_sample_buffer().size() << ' '
		<< num_samples_to_write << ' '
		<< playback_properties_.num_channels << ' '
		<< (num_samples_to_write * playback_properties_.num_channels) << ' '
		<< num_samples_to_return << ' '
		<< (long(custom_flac_decoder_->get_sample_buffer().size()) - long(num_samples_to_write * playback_properties_.num_channels)) << std::endl;*/

	current_position += num_samples_to_return;

	if (custom_flac_decoder_->is_ok())
	{
		long remaining_size = long(custom_flac_decoder_->get_sample_buffer().size()) - long(num_samples_to_return * playback_properties_.num_channels);
		std::memcpy(dest, &custom_flac_decoder_->get_sample_buffer()[0], num_samples_to_return * playback_properties_.num_channels * get_sample_size(playback_properties_.sample_type_));
		if (remaining_size > 0)
		{
			std::memmove(
				&custom_flac_decoder_->get_sample_buffer()[0],
				&custom_flac_decoder_->get_sample_buffer()[num_samples_to_write * playback_properties_.num_channels],
				remaining_size * get_sample_size(playback_properties_.sample_type_)
			);

			custom_flac_decoder_->get_sample_buffer().resize(remaining_size);
		}
		else
			custom_flac_decoder_->get_sample_buffer().resize(0);

		return num_samples_to_return;
	}
	else
		return 0;
}




flac_decoder_creator::flac_decoder_creator()
{
}


decoder_ptr_t flac_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback, std::string const &mime_type)
{
	if (
		(mime_type != "application/flac") &&
		(mime_type != "application/x-flac") &&
		(mime_type != "audio/flac") &&
		(mime_type != "audio/x-flac")
	)
		return decoder_ptr_t();


	flac_decoder *flac_decoder_ = new flac_decoder(send_command_callback, source_);
	if (!flac_decoder_->is_initialized())
	{
		delete flac_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(flac_decoder_);
}



}
}

