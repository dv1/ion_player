#include <algorithm>
#include <map>
#include <stdint.h>
#include <boost/foreach.hpp>
#include <boost/thread/locks.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "dumb_decoder.hpp"


namespace ion
{
namespace backend
{



namespace
{


typedef boost::function < DUH* (DUMBFILE *) > dumb_read_function_t;
typedef boost::tuple < dumb_read_function_t, std::string > dumb_read_function_entry_t;


void read_module_impl(DUH* &duh, std::vector < uint8_t > &data, dumb_read_function_entry_t const &function_entry)
{
	if (duh != 0)
		return;

	DUMBFILE *file = dumbfile_open_memory(reinterpret_cast < char const * > (&data[0]), data.size());
	duh = function_entry.get < 0 > ()(file);
	dumbfile_close(file);
}


DUH* read_module(source &source_, long const filesize, dumb_decoder::module_type const module_type_)
{
	std::vector < uint8_t > data;

	{
		data.resize(filesize);
		long actual_size = source_.read(&data[0], filesize);
		data.resize(actual_size);
	}

	DUH *duh = 0;

	typedef std::map < dumb_decoder::module_type, dumb_read_function_entry_t > read_funcs_t;
	read_funcs_t read_funcs;
	read_funcs[dumb_decoder::module_type_xm] = dumb_read_function_entry_t(boost::lambda::bind(&dumb_read_xm, boost::lambda::_1), "xm");
	read_funcs[dumb_decoder::module_type_it] = dumb_read_function_entry_t(boost::lambda::bind(&dumb_read_it, boost::lambda::_1), "it");
	read_funcs[dumb_decoder::module_type_s3m] = dumb_read_function_entry_t(boost::lambda::bind(&dumb_read_s3m, boost::lambda::_1), "s3m");
	read_funcs[dumb_decoder::module_type_mod] = dumb_read_function_entry_t(boost::lambda::bind(&dumb_read_mod, boost::lambda::_1), "mod");

	{
		read_funcs_t::iterator read_func_iter = read_funcs.find(module_type_);
		if (read_func_iter != read_funcs.end())
			read_module_impl(duh, data, read_func_iter->second);
	}

	if (duh == 0)
	{
		BOOST_FOREACH(read_funcs_t::value_type &value, read_funcs)
		{
			if (module_type_ == value.first)
			{
				read_module_impl(duh, data, value.second);
				if (duh != 0)
					return duh;
			}
		}
	}

	return duh;
}


}




dumb_decoder::dumb_decoder(message_callback_t const message_callback, source_ptr_t source_, long const filesize):
	decoder(message_callback),
	duh(0),
	duh_sigrenderer(0),
	module_type_(module_type_unknown),
	current_volume(max_volume()),
	source_(source_)
{
	if (!source_)
		return;

	loop_data_.loop_mode = -1;
	loop_data_.cur_num_loops = 0;

	// TODO: put these global initializations in a static function with refcounting
	dumb_resampling_quality = DUMB_RQ_CUBIC;
	dumb_it_max_to_mix = 256;

	if (test_if_module_file())
		duh = read_module(*source_, filesize, module_type_);
}


dumb_decoder::~dumb_decoder()
{
	boost::lock_guard < boost::mutex > lock(mutex_);

	if (duh_sigrenderer != 0) duh_end_sigrenderer(duh_sigrenderer);
	if (duh != 0) unload_duh(duh);
}


bool dumb_decoder::is_initialized() const
{
	return source_ && (duh != 0);
}


bool dumb_decoder::can_playback() const
{
	// Not using a mutex lock here on purpose, since the only function that is running in a
	// different thread is update(), so can_playback() is recreated there, WITH a mutex lock
	// (it is wise to avoid unnecessary locks..)
	return is_initialized() && (duh_sigrenderer != 0);
}


void dumb_decoder::pause()
{
}


void dumb_decoder::resume()
{
}


long dumb_decoder::set_current_position(long const new_position)
{
	if (!can_playback())
		return -1;

	if ((new_position < 0) || (new_position >= get_num_ticks()))
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);

	reinitialize_sigrenderer(playback_properties_.num_channels, new_position);

	return duh_sigrenderer_get_position(duh_sigrenderer);
}


long dumb_decoder::get_current_position() const
{
	if (!can_playback())
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);

	// DUMB does not reset the position when looping - compensate
	return duh_sigrenderer_get_position(duh_sigrenderer) - loop_data_.cur_num_loops * duh_get_length(duh);
}


long dumb_decoder::set_current_volume(long const new_volume)
{
	if ((new_volume < 0) || (new_volume > max_volume()))
		return -1;

	boost::lock_guard < boost::mutex > lock(mutex_);
	current_volume = new_volume;
	return current_volume;
}


long dumb_decoder::get_current_volume() const
{
	return current_volume;
}


Json::Value dumb_decoder::get_songinfo() const
{
	if (!is_initialized()) // NOTE: not using can_playback() here, since it is valid to call get_songinfo() without any playback going on
		return Json::Value();

	Json::Value songinfo;

	char const *title = duh_get_tag(duh, "TITLE");
	if (title != 0)
		songinfo["title"] = title;
	else
		songinfo["title"] = "";

	return songinfo;
}


std::string dumb_decoder::get_type() const
{
	return "dumb";
}


uri dumb_decoder::get_uri() const
{
	return (source_) ? source_->get_uri() : uri();
}


long dumb_decoder::get_num_ticks() const
{
	return (duh != 0) ? duh_get_length(duh) : 0;
}


long dumb_decoder::get_num_ticks_per_second() const
{
	return 65536;
}


void dumb_decoder::set_loop_mode(int const new_loop_mode)
{
	boost::lock_guard < boost::mutex > lock(mutex_);
	set_loop_mode_impl(new_loop_mode);
}


bool dumb_decoder::test_if_module_file()
{
	if (!source_->can_seek(source::seek_absolute))
		return false;


	typedef boost::array < uint8_t, 4 > fourcc_t;

	struct fourcc_entry
	{
		int offset;
		module_type module_type_;
		fourcc_t fourcc;
	};

	static fourcc_entry const fourcc_entries[] = {
		{ 0, module_type_xm, {{ 0x45, 0x78, 0x74, 0x65 }} }, // XM
		{ 0, module_type_it, {{ 0x49, 0x4D, 0x50, 0x4D }} }, // IT

		{ 44, module_type_s3m, {{ 0x53, 0x43, 0x52, 0x4D }} }, // S3M

		// Fasttracker MOD (4/6/8 channels)
		{ 1080, module_type_mod, {{ '4', 'C', 'H', 'N' }} },
		{ 1080, module_type_mod, {{ '6', 'C', 'H', 'N' }} },
		{ 1080, module_type_mod, {{ '8', 'C', 'H', 'N' }} },

		// Mahoney & Kaktus Protracker 4 channel
		{ 1080, module_type_mod, {{ 'M', '.', 'K', '.' }} },
		{ 1080, module_type_mod, {{ 'M', '&', 'K', '!' }} },
		{ 1080, module_type_mod, {{ 'M', '!', 'K', '!' }} },

		// Startracker 4/8 channel
		{ 1080, module_type_mod, {{ 'F', 'L', 'T', '4' }} },
		{ 1080, module_type_mod, {{ 'F', 'L', 'T', '8' }} },

		// Startracker 4/8 channel
		{ 1080, module_type_mod, {{ 'E', 'X', '0', '4' }} },
		{ 1080, module_type_mod, {{ 'E', 'X', '0', '8' }} },



		{ -1, module_type_unknown, {{ 0, 0, 0, 0 }} }
	};


	for (fourcc_entry const *fourcc_entry_ = fourcc_entries; fourcc_entry_->offset >= 0; ++fourcc_entry_)
	{
		source_->seek(fourcc_entry_->offset, source::seek_absolute);
		fourcc_t read_fourcc;
		source_->read(&read_fourcc[0], 4);
		if (read_fourcc == fourcc_entry_->fourcc)
		{
			source_->seek(0, source::seek_absolute);
			module_type_ = fourcc_entry_->module_type_;
			return true;
		}
	}


	return false;
}


namespace
{

int custom_dumb_loop_callback(void *data)
{
	dumb_decoder::loop_data *loop_data_ = reinterpret_cast < dumb_decoder::loop_data* > (data);
	if (loop_data_->loop_mode < 0)
		return 1;
	else if (loop_data_->loop_mode > 0)
	{
		if (loop_data_->cur_num_loops >= loop_data_->loop_mode)
			return 1;
		else
		{
			++loop_data_->cur_num_loops;
			return 0;
		}
	}
	else
	{
		++loop_data_->cur_num_loops;
		return 0;
	}
}

}


void dumb_decoder::reinitialize_sigrenderer(unsigned const int new_num_channels, long const new_position)
{
	if (duh_sigrenderer != 0) duh_end_sigrenderer(duh_sigrenderer);
	duh_sigrenderer = duh_start_sigrenderer(duh, 0, new_num_channels, new_position);

	if (duh_sigrenderer != 0)
	{
		DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(duh_sigrenderer);
		dumb_it_set_loop_callback(itsr, &custom_dumb_loop_callback, &loop_data_);
		dumb_it_set_xm_speed_zero_callback(itsr, &custom_dumb_loop_callback, &loop_data_);
	}
	// if duh_sigrenderer is zero for some reason, any subsequent update() call will return zero, telling the sink that this decoder is done
}


void dumb_decoder::set_loop_mode_impl(int const new_loop_mode)
{
	loop_data_.loop_mode = new_loop_mode;
	loop_data_.cur_num_loops = 0;
}


void dumb_decoder::set_playback_properties(playback_properties const &new_playback_properties)
{
	if (!is_initialized())
		return;

	boost::lock_guard < boost::mutex > lock(mutex_);

	// reinitialize sigrenderer if the amount of channels changed
	if (playback_properties_.num_channels != new_playback_properties.num_channels)
	{
		long cur_position = 0;
		if (duh_sigrenderer != 0)
			cur_position = duh_sigrenderer_get_position(duh_sigrenderer);
		reinitialize_sigrenderer(new_playback_properties.num_channels, cur_position);
	}

	// finally, copy over the new properties
	playback_properties_ = new_playback_properties;
}


unsigned int dumb_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	boost::lock_guard < boost::mutex > lock(mutex_);

	// NOTE: NOT using can_playback() here, since this would otherwise lead to race conditions
	if ((duh == 0) || (duh_sigrenderer == 0))
		return 0;

	// TODO: for the sample type, evaluate the type from the playback properties
	unsigned int l = duh_render(duh_sigrenderer, 16, 0, float(current_volume) / float(max_volume()), 65536.0f / float(playback_properties_.frequency), num_samples_to_write, dest);
	return l;
}




decoder_ptr_t dumb_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, message_callback_t const &message_callback)
{
	// Check if the source has a size; if not, then the source may not have an end; decoding is not possible then
	long filesize = source_->get_size();
	if (filesize < 0)
		return decoder_ptr_t();

	// TODO: use metadata to determine the format (MOD/S3M/XM/IT/...)
	dumb_decoder *dumb_decoder_ = new dumb_decoder(message_callback, source_, filesize);
	if (!dumb_decoder_->is_initialized())
	{
		delete dumb_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(dumb_decoder_);
}


}
}

