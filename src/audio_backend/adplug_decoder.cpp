#include <stdint.h>
#include <boost/lexical_cast.hpp>
#include <boost/thread/locks.hpp>
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
}


source_binbase::~source_binbase()
{
	source_.reset();
}


void source_binbase::seek(long p, Offset offs)
{
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
}


source_binistream::source_binistream(source_binistream const &other):
	source_binbase(other),
	binistream(other),
	source_(other.source_)
{
}


source_binistream::~source_binistream()
{
}


binio::Byte source_binistream::getByte()
{
	binio::Byte read;
	if (source_.can_read())
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
}


binistream* adplug_source_file_provider::open(std::string) const
{
	binistream *in = new source_binistream(source_);
	if (in != 0)
	{
		in->setFlag(binio::BigEndian, false);
		in->setFlag(binio::FloatIEEE);
	}
	return in;
}


void adplug_source_file_provider::close(binistream *f) const
{
	source_binistream *ff = (source_binistream*)f;
	if (f != 0)
		delete ff;
}


}




adplug_decoder::adplug_decoder(send_command_callback_t const send_command_callback, source_ptr_t source_, metadata_t const &initial_metadata):
	decoder(send_command_callback),
	opl(0),
	player(0),
	source_(source_),
	to_add(0),
	subsong_nr(0),
	stereo(true),
	surround(true),
	is16bit(true),
	current_position(0),
	seek_to(-1),
	loop_mode(0),
	cur_num_loops(0)
{
	if (!source_)
		return;

	try
	{
		uri::options_t const &options_ = source_->get_uri().get_options();
		uri::options_t::const_iterator iter = options_.find("subsong_index");
		if (iter != options_.end())
			subsong_nr = boost::lexical_cast < long > (iter->second);
	}
	catch (boost::bad_lexical_cast const &)
	{
	}

	initialize_player(48000);
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


long adplug_decoder::set_current_position(long const new_position)
{
	boost::lock_guard < boost::mutex > lock(mutex_);
	seek_to = new_position;
	return new_position;
}


long adplug_decoder::get_current_position() const
{
	boost::lock_guard < boost::mutex > lock(mutex_);
	return long(current_position);
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
	metadata_t metadata_ = empty_metadata();
	if (!is_initialized())
		return metadata_;

	std::string title_ = player->gettitle();
	if (!title_.empty())
		set_metadata_value(metadata_, "title", title_);

	std::string author_ = player->getauthor();
	if (!author_.empty())
		set_metadata_value(metadata_, "author", author_);

	std::string desc_ = player->getdesc();
	if (!desc_.empty())
		set_metadata_value(metadata_, "description", desc_);

	set_metadata_value(metadata_, "num_subsongs", player->getsubsongs());

	return metadata_;
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
	return (is_initialized()) ? long(cur_song_length) : long(0);
}


long adplug_decoder::get_num_ticks_per_second() const
{
	return 1000;
}


void adplug_decoder::set_loop_mode(int const new_loop_mode)
{
	boost::lock_guard < boost::mutex > lock(mutex_);
	loop_mode = new_loop_mode;
	cur_num_loops = 0;
}


void adplug_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	playback_properties_ = new_playback_properties;
	initialize_player(playback_properties_.frequency);
}


void adplug_decoder::initialize_player(unsigned int const frequency)
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

	adplug_detail::adplug_source_file_provider file_provider(*source_);
	player = CAdPlug::factory(
		source_->get_uri().get_path(),
		opl,
		CAdPlug::players,
		file_provider
	);
	cur_song_length = player->songlength(subsong_nr);
	player->rewind(subsong_nr);
}


unsigned int adplug_decoder::get_decoder_samplerate() const
{
	return 0;
}


unsigned int adplug_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (player == 0)
		return 0;

	{
		boost::lock_guard < boost::mutex > lock(mutex_);

		if (seek_to != -1)
		{
			if (seek_to < current_position)
			{
				player->rewind(subsong_nr);
				current_position = 0.0f;
			}

			while (current_position < seek_to)
			{
				if (!player->update())
					break;
				current_position += 1000.0f / player->getrefresh();
			}

			seek_to = -1;
		}

		float songlength = float(cur_song_length);
		if (current_position >= songlength)
		{
			if (loop_mode < 0)
				return 0;
			else if (loop_mode >= 0)
			{
				if ((loop_mode > 0) && (cur_num_loops >= loop_mode))
					return 0;

				++cur_num_loops;
				current_position = std::fmod(current_position, songlength);
			}
		}
	}

	long to_write = num_samples_to_write;
	uint8_t *bufpos = reinterpret_cast < uint8_t*> (dest);
	while (to_write > 0)
	{
		while (to_add < 0)
		{
			to_add += playback_properties_.frequency;
			player->update();

			{
				boost::lock_guard < boost::mutex > lock(mutex_);
				current_position += 1000.0f / player->getrefresh();
			}
		}

		long i = std::min(to_write, long(to_add / player->getrefresh() + 4) & ~3);
		opl->update(reinterpret_cast < short * > (bufpos), i);
		bufpos += i * (stereo ? 2 : 1) * (is16bit ? 2 : 1);
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
	adplug_decoder *adplug_decoder_ = new adplug_decoder(
		send_command_callback,
		source_,
		metadata
	);

	if (!adplug_decoder_->is_initialized())
	{
		delete adplug_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(adplug_decoder_);
}


}
}

