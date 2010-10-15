/**************************************************************************

    Copyright (C) 2010  Carlos Rafael Giani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**************************************************************************/


#include <algorithm>
#include <map>
#include <stdint.h>
#include <boost/foreach.hpp>
#include <boost/thread/locks.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>

#include "dumb_decoder.hpp"




extern "C"
{

static void* custom_dumb_stream_open(char const * source_ptr)
{
        return (void*)(source_ptr);
}

static int custom_dumb_stream_skip(void *f, long n)
{
        ion::audio_common::source *source_ = reinterpret_cast < ion::audio_common::source* > (f);
        source_->seek(n, ion::audio_common::source::seek_relative);
        return 0;
}

static int custom_dumb_stream_getc(void *f)
{
        ion::audio_common::source *source_ = reinterpret_cast < ion::audio_common::source* > (f);
        uint8_t b;
        source_->read(reinterpret_cast < char* > (&b), 1);
        return int(b);
}

static long custom_dumb_stream_getnc(char *ptr, long n, void *f)
{
        ion::audio_common::source *source_ = reinterpret_cast < ion::audio_common::source* > (f);
        source_->read(ptr, n);
        return n;
}

static void custom_dumb_stream_close(void *f)
{
        ion::audio_common::source *source_ = reinterpret_cast < ion::audio_common::source* > (f);
        source_->seek(0, ion::audio_common::source::seek_absolute);
}

}



namespace ion
{
namespace audio_backend
{


using namespace audio_common;



namespace
{


typedef boost::function < DUH* (char const *) > dumb_read_function_t;
typedef boost::tuple < dumb_read_function_t, std::string > dumb_read_function_entry_t;


void read_module_impl(DUH* &duh, source &source_, dumb_read_function_entry_t const &function_entry)
{
	if (duh != 0)
		return;

	char const *source_ptr = reinterpret_cast < char const* > (&source_);
	duh = function_entry.get < 0 > ()(source_ptr);
}


DUH* read_module(source &source_, long const filesize, dumb_decoder::module_type &module_type_)
{
	DUH *duh = 0;

	typedef std::map < dumb_decoder::module_type, dumb_read_function_entry_t > read_funcs_t;
	read_funcs_t read_funcs;
	read_funcs[dumb_decoder::module_type_xm] = dumb_read_function_entry_t(boost::phoenix::bind(&dumb_load_xm, boost::phoenix::arg_names::arg1), "xm");
	read_funcs[dumb_decoder::module_type_it] = dumb_read_function_entry_t(boost::phoenix::bind(&dumb_load_it, boost::phoenix::arg_names::arg1), "it");
	read_funcs[dumb_decoder::module_type_s3m] = dumb_read_function_entry_t(boost::phoenix::bind(&dumb_load_s3m, boost::phoenix::arg_names::arg1), "s3m");
	read_funcs[dumb_decoder::module_type_mod] = dumb_read_function_entry_t(boost::phoenix::bind(&dumb_load_mod, boost::phoenix::arg_names::arg1), "mod");

	{
		read_funcs_t::iterator read_func_iter = read_funcs.find(module_type_);
		if (read_func_iter != read_funcs.end())
			read_module_impl(duh, source_, read_func_iter->second);
	}

	if (duh == 0)
	{
		BOOST_FOREACH(read_funcs_t::value_type &value, read_funcs)
		{
			if (module_type_ != value.first)
			{
				read_module_impl(duh, source_, value.second);
				if (duh != 0)
				{
					module_type_ = value.first;
					return duh;
				}
			}
		}
	}

	return duh;
}


}




dumb_decoder::dumb_decoder(send_event_callback_t const send_event_callback, source_ptr_t source_, long const filesize, module_type const module_type_):
	decoder(send_event_callback),
	duh(0),
	duh_sigrenderer(0),
	module_type_(module_type_),
	source_(source_)
{
	if (!source_)
		return;

	loop_data_.loop_mode = -1;
	loop_data_.cur_num_loops = 0;
	playback_properties_.num_channels = 0;

	duh = read_module(*source_, filesize, this->module_type_);
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


metadata_t dumb_decoder::get_metadata() const
{
	metadata_t metadata_ = empty_metadata();

	if (!is_initialized()) // NOTE: not using can_playback() here, since it is valid to call get_songinfo() without any playback going on
		return metadata_;

	char const *title = duh_get_tag(duh, "TITLE");
	if (title != 0)
		set_metadata_value(metadata_, "title", title);
	else
		set_metadata_value(metadata_, "title", "");

	{
		std::string type_str;

		switch (module_type_)
		{
			case module_type_mod: type_str = "mod"; break;
			case module_type_s3m: type_str = "s3m"; break;
			case module_type_xm: type_str = "xm"; break;
			case module_type_it: type_str = "it"; break;
			default: break;
		}

		if (!type_str.empty())
			set_metadata_value(metadata_, "dumb_module_type", type_str);
	}

	return metadata_;
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


decoder_properties dumb_decoder::get_decoder_properties() const
{
	return decoder_properties(0, playback_properties_.num_channels, audio_common::sample_s16);
}


unsigned int dumb_decoder::update(void *dest, unsigned int const num_samples_to_write)
{
	if (!is_initialized())
		return 0;

	boost::lock_guard < boost::mutex > lock(mutex_);

	// NOTE: NOT using can_playback() here, since this would otherwise lead to race conditions
	if ((duh == 0) || (duh_sigrenderer == 0))
		return 0;

	unsigned int l = duh_render(duh_sigrenderer, 16, 0, 1.0f, 65536.0f / float(playback_properties_.frequency), num_samples_to_write, dest);
	return l;
}




dumb_decoder_creator::dumb_decoder_creator()
{
	dumb_resampling_quality = DUMB_RQ_CUBIC;
	dumb_it_max_to_mix = 256;

        fs.open = &custom_dumb_stream_open;
        fs.skip = &custom_dumb_stream_skip;
        fs.getc = &custom_dumb_stream_getc;
        fs.getnc = &custom_dumb_stream_getnc;
        fs.close = &custom_dumb_stream_close;
        register_dumbfile_system(&fs);
}


decoder_ptr_t dumb_decoder_creator::create(source_ptr_t source_, metadata_t const &metadata, send_event_callback_t const &send_event_callback)
{
	// Check if the source has a size; if not, then the source may not have an end; decoding is not possible then
	long filesize = source_->get_size();
	if (filesize < 0)
		return decoder_ptr_t();


	dumb_decoder::module_type module_type_ = dumb_decoder::module_type_unknown;

	// retrieve the format from the metadata if present (MOD/S3M/XM/IT/...), to allow for quicker loading
	if (has_metadata_value(metadata, "dumb_module_type"))
	{
		std::string type_str = get_metadata_value < std::string > (metadata, "dumb_module_type", "");
		if (type_str == "mod") module_type_ = dumb_decoder::module_type_mod;
		else if (type_str == "s3m") module_type_ = dumb_decoder::module_type_s3m;
		else if (type_str == "xm") module_type_ = dumb_decoder::module_type_xm;
		else if (type_str == "it") module_type_ = dumb_decoder::module_type_it;
	}

	// if the format is not known beforehand, the code needs to test what module type it is
	if (module_type_ == dumb_decoder::module_type_unknown)
		module_type_ = test_if_module_file(source_);

	if (module_type_ == dumb_decoder::module_type_unknown) // this is not a module file -> exit
		return decoder_ptr_t();

	// at this point, it is clear that this most likely is a module file -> try to load it
	dumb_decoder *dumb_decoder_ = new dumb_decoder(send_event_callback, source_, filesize, module_type_);
	if (!dumb_decoder_->is_initialized())
	{
		delete dumb_decoder_;
		return decoder_ptr_t();
	}
	else
		return decoder_ptr_t(dumb_decoder_);
}


dumb_decoder::module_type dumb_decoder_creator::test_if_module_file(source_ptr_t source_)
{
	if (!source_->can_seek(source::seek_absolute))
		return dumb_decoder::module_type_unknown;


	typedef boost::array < uint8_t, 4 > fourcc_t;

	struct fourcc_entry
	{
		int offset;
		dumb_decoder::module_type module_type_;
		fourcc_t fourcc;
	};

	static fourcc_entry const fourcc_entries[] = {
		{ 0, dumb_decoder::module_type_xm, {{ 0x45, 0x78, 0x74, 0x65 }} }, // XM
		{ 0, dumb_decoder::module_type_it, {{ 0x49, 0x4D, 0x50, 0x4D }} }, // IT

		{ 44, dumb_decoder::module_type_s3m, {{ 0x53, 0x43, 0x52, 0x4D }} }, // S3M

		// Fasttracker MOD (4/6/8 channels)
		{ 1080, dumb_decoder::module_type_mod, {{ '4', 'C', 'H', 'N' }} },
		{ 1080, dumb_decoder::module_type_mod, {{ '6', 'C', 'H', 'N' }} },
		{ 1080, dumb_decoder::module_type_mod, {{ '8', 'C', 'H', 'N' }} },

		// Mahoney & Kaktus Protracker 4 channel
		{ 1080, dumb_decoder::module_type_mod, {{ 'M', '.', 'K', '.' }} },
		{ 1080, dumb_decoder::module_type_mod, {{ 'M', '&', 'K', '!' }} },
		{ 1080, dumb_decoder::module_type_mod, {{ 'M', '!', 'K', '!' }} },

		// Startracker 4/8 channel
		{ 1080, dumb_decoder::module_type_mod, {{ 'F', 'L', 'T', '4' }} },
		{ 1080, dumb_decoder::module_type_mod, {{ 'F', 'L', 'T', '8' }} },

		// Startracker 4/8 channel
		{ 1080, dumb_decoder::module_type_mod, {{ 'E', 'X', '0', '4' }} },
		{ 1080, dumb_decoder::module_type_mod, {{ 'E', 'X', '0', '8' }} },



		{ -1, dumb_decoder::module_type_unknown, {{ 0, 0, 0, 0 }} }
	};


	for (fourcc_entry const *fourcc_entry_ = fourcc_entries; fourcc_entry_->offset >= 0; ++fourcc_entry_)
	{
		source_->seek(fourcc_entry_->offset, source::seek_absolute);
		fourcc_t read_fourcc;
		source_->read(&read_fourcc[0], 4);
		if (read_fourcc == fourcc_entry_->fourcc)
		{
			source_->seek(0, source::seek_absolute);
			return fourcc_entry_->module_type_;
		}
	}


	return dumb_decoder::module_type_unknown;
}


}
}

