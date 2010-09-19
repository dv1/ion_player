#include <iostream>
#include <stdint.h>
#include <adplug.h>
#include <emuopl.h>
#include <surroundopl.h>
#include "adplug_decoder.hpp"


namespace ion
{
namespace audio_backend
{



namespace adplug_detail
{


source_binbase::source_binbase(source &source_):
	source_(source_)
{
	source_.seek(0, source::seek_absolute);
}


source_binbase::~source_binbase()
{
	source_.seek(0, source::seek_absolute);
}


void source_binbase::seek(long p, Offset offs)
{
	std::cerr << "source_binbase::seek " << p << " " << offs << std::endl;
	switch (offs)
	{
		case binio::Set:
			source_.seek(p, source::seek_absolute);
			break;
		case binio::Add:
			source_.seek(p, source::seek_relative);
			break;
		case binio::End:
			source_.seek(p, source::seek_from_end);
			break;
		default:
			break;
	}
}


long source_binbase::pos()
{
	return source_.get_position();
}



source_binistream::source_binistream(source &source_):
	source_binbase(source_),
	source_(source_)
{
	source_.seek(0, source::seek_absolute);
}


source_binistream::~source_binistream()
{
	source_.seek(0, source::seek_absolute);
}


binio::Byte source_binistream::getByte()
{
	std::cerr << "source_binistream::getByte at pos " << source_.get_position() << std::endl;

	binio::Byte read;
	source_.read(&read, 1);
	if (!source_.can_read())
		err |= Eof;
	return read;
}




class adplug_source_file_provider:
	public CFileProvider
{
public:
	adplug_source_file_provider(source &source_);
	virtual binistream* open(std::string filename) const;
	virtual void close(binistream *f) const;

protected:
	source &source_;
};


adplug_source_file_provider::adplug_source_file_provider(source &source_):
	source_(source_)
{
	source_.seek(0, source::seek_absolute);
}


binistream* adplug_source_file_provider::open(std::string) const
{
	source_.seek(0, source::seek_absolute);
	binistream *in = new source_binistream(source_);
	in->setFlag(binio::BigEndian, false);
	in->setFlag(binio::FloatIEEE);
	return in;
}


void adplug_source_file_provider::close(binistream *f) const
{
	source_.seek(0, source::seek_absolute);
	delete f;
}


}




adplug_decoder::adplug_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_, metadata_t const &initial_metadata):
	decoder(send_command_callback),
	opl(0),
	player(0),
	source_(source_),
	to_add(0)
{
	if (!source_)
		return;

	initialize_player(true, true, true, 48000);
}


adplug_decoder::~adplug_decoder()
{
	if (player != 0) delete player;
	if (opl != 0) delete opl;
}


bool adplug_decoder::is_initialized() const
{
	return source_ && (player != 0) && (opl != 0);
}


bool adplug_decoder::can_playback() const
{
	return is_initialized();
}



void adplug_decoder::pause()
{
}


void adplug_decoder::resume()
{
}


long adplug_decoder::set_current_position(long const)
{
	return 0;
}


long adplug_decoder::get_current_position() const
{
	return 0;
}


long adplug_decoder::set_current_volume(long const)
{
	return max_volume();
}


long adplug_decoder::get_current_volume() const
{
	return max_volume();
}


metadata_t adplug_decoder::get_metadata() const
{
	return empty_metadata();
}


std::string adplug_decoder::get_type() const
{
	return "adplug";
}


uri adplug_decoder::get_uri() const
{
	return (source_) ? source_->get_uri() : uri();
}


long adplug_decoder::get_num_ticks() const
{
	return 1;
}


long adplug_decoder::get_num_ticks_per_second() const
{
	return 1;
}


void adplug_decoder::set_loop_mode(int const new_loop_mode)
{
}


void adplug_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	playback_properties_ = new_playback_properties;

	bool stereo = true;
	bool surround = true;
	bool _16bit = true;
	int freq = playback_properties_.frequency;

	initialize_player(stereo, surround, _16bit, freq);
}


void adplug_decoder::initialize_player(bool const stereo, bool const surround, bool const is16bit, unsigned int const frequency)
{
	if (opl != 0) { delete opl; opl = 0; }
	if (player != 0) { delete player; player = 0; }

	if (stereo && surround)
	{
		CEmuopl *oplA = new CEmuopl(frequency, is16bit, false);
		CEmuopl *oplB = new CEmuopl(frequency, is16bit, false);
		opl = new CSurroundopl(oplA, oplB, is16bit);
	}
	else
		opl = new CEmuopl(frequency, is16bit, stereo);

#if 1
	player = CAdPlug::factory(
		"<dummystr>",
		opl,
		CAdPlug::players,
		adplug_detail::adplug_source_file_provider(*source_)
	);
#else
	player = CAdPlug::factory(
		source_->get_uri().get_path(),
		opl
	);
#endif
}


unsigned int adplug_decoder::get_decoder_samplerate() const
{
	return 0;
}


unsigned int adplug_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (player == 0)
		return 0;

	long to_write = num_samples_to_write;
	uint8_t *bufpos = (uint8_t*)dest;
	while (to_write > 0)
	{
		while (to_add < 0)
		{
			to_add += playback_properties_.frequency;
			player->update();
		}

		long i = std::min(to_write, long(to_add / player->getrefresh() + 4) & ~3);
		opl->update((short *)bufpos, i);
		bufpos += i * 2 * 2;
		to_write -= i;
		to_add -= long(player->getrefresh() * i);
	}

	return num_samples_to_write;
}




adplug_decoder_creator::adplug_decoder_creator()
{
}


decoder_ptr_t adplug_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_command_callback_t const &send_command_callback, magic_t)
{
	return decoder_ptr_t(
		new adplug_decoder(
			send_command_callback,
			source_,
			metadata
		)
	);
}


}
}

