#include "ion_config.h"
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <ion/backend_main_loop.hpp>
#include <ion/resource_exceptions.hpp>
#include "backend.hpp"
#include "file_source.hpp"
#ifdef WITH_DUMB_DECODER
#include "dumb_decoder.hpp"
#endif
#ifdef WITH_MPG123_DECODER
#include "mpg123_decoder.hpp"
#endif
#ifdef WITH_VORBIS_DECODER
#include "vorbis_decoder.hpp"
#endif
#ifdef WITH_ADPLUG_DECODER
#include "adplug_decoder.hpp"
#endif
#ifdef WITH_FLAC_DECODER
#include "flac_decoder.hpp"
#endif
#ifdef WITH_GME_DECODER
#include "gme_decoder.hpp"
#endif
#ifdef WITH_UADE_DECODER
#include "uade_decoder.hpp"
#endif
#ifdef WITH_ALSA_SINK
#include "alsa_sink.hpp"
#endif


/*
The backend can be called in three ways:
1. Without arguments: the backend starts normally, listens to stdin for command lines, and outputs events to stdout
2. With the -id argument: the backend outputs its type ID to stdout (this is *not* the C++ typeid, but the get_backend_type() result!)
3. With the -info argument and one or more URIs: the backend then scans each URI for metadata, and outputs said metadata to stdout, formatted like a metadata event
*/

// TODO: right now, the modules (decoders, sinks ..) are hardcoded in here. This is not the way it shall work in the
// final version; instead, these are to be present as plugins.


struct creators
{
	typedef boost::ptr_vector < ion::audio_common::decoder_creator > decoder_creators_container_t;
	decoder_creators_container_t decoder_creators_container;

#ifdef WITH_ALSA_SINK
	ion::audio_backend::alsa_sink_creator alsa_sink_creator_;
#endif
	ion::audio_backend::file_source_creator file_source_creator_;

	explicit creators(ion::audio_backend::backend &backend_)
	{
		backend_.get_source_creators().push_back(&file_source_creator_);

#ifdef WITH_ALSA_SINK
		backend_.get_sink_creators().push_back(&alsa_sink_creator_);
#endif

#ifdef WITH_DUMB_DECODER
		decoder_creators_container.push_back(new ion::audio_backend::dumb_decoder_creator);
#endif
#ifdef WITH_MPG123_DECODER
		decoder_creators_container.push_back(new ion::audio_backend::mpg123_decoder_creator);
#endif
#ifdef WITH_VORBIS_DECODER
		decoder_creators_container.push_back(new ion::audio_backend::vorbis_decoder_creator);
#endif
#ifdef WITH_ADPLUG_DECODER
		decoder_creators_container.push_back(new ion::audio_backend::adplug_decoder_creator);
#endif
#ifdef WITH_GME_DECODER
		decoder_creators_container.push_back(new ion::audio_backend::gme_decoder_creator);
#endif
#ifdef WITH_FLAC_DECODER
		decoder_creators_container.push_back(new ion::audio_backend::flac_decoder_creator);
#endif
#ifdef WITH_UADE_DECODER
		decoder_creators_container.push_back(new ion::audio_backend::uade_decoder_creator);
#endif

		BOOST_FOREACH(ion::audio_common::decoder_creator &decoder_creator_, decoder_creators_container)
		{
			backend_.get_decoder_creators().push_back(&decoder_creator_);
		}
	}
};

typedef boost::shared_ptr < creators > creators_ptr_t;




int main(int argc, char **argv)
{
	enum run_mode_type
	{
		default_run_mode,
		print_id_mode,
		get_resource_info_mode
	};

	run_mode_type run_mode;


	std::vector < std::string > params;

	{
		for (int i = 1; i < argc; ++i)
			params.push_back(argv[i]);
	}


	if (params.empty())
		run_mode = default_run_mode;
	else if (params[0] == "-id")
		run_mode = print_id_mode;
	else if ((params[0] == "-info") && (params.size() >= 2)) // 2 params: -info <url>
		run_mode = get_resource_info_mode;
	else
		return -1;


	ion::audio_backend::backend backend_;
	creators_ptr_t creators_;


	switch (run_mode)
	{
		case default_run_mode:
		{

			ion::backend_main_loop < ion::audio_backend::backend > backend_main_loop(std::cin, std::cout, backend_);
			creators_ = creators_ptr_t(new creators(backend_));
#ifdef WITH_ALSA_SINK
			backend_.create_sink("alsa"); // Use the alsa sink for sound output (TODO: this is platform specific; on Windows, one would use Waveout, on OSX it would be CoreAudio etc.)
#else
#error Currently, only ALSA output is supported. Please build the backend with the ALSA sink.
#endif

			backend_main_loop.run();

			break;
		}


		case print_id_mode:
		{
			// The id mode does not need any creator
			std::cout << backend_.get_type() << std::endl;
			break;
		}


		case get_resource_info_mode:
		{
			creators_ = creators_ptr_t(new creators(backend_));

			for (unsigned int i = 1; i < params.size(); ++i)
			{
				try
				{
					std::string const &url = params[i];
					ion::audio_common::decoder::metadata_str_optional_t metadata_str = backend_.get_metadata(url);
					if (metadata_str)
						std::cout << ion::recombine_command_line("metadata", boost::assign::list_of(url)(*metadata_str)) << std::endl;
				}
				catch (ion::unrecognized_resource const &exc)
				{
					std::cout << ion::recombine_command_line("unrecognized_resource", boost::assign::list_of(exc.what())) << std::endl;
				}
				catch (ion::resource_not_found const &exc)
				{
					std::cout << ion::recombine_command_line("resource_not_found", boost::assign::list_of(exc.what())) << std::endl;
				}
				catch (ion::resource_corrupted const &exc)
				{
					std::cout << ion::recombine_command_line("resource_corrupted", boost::assign::list_of(exc.what())) << std::endl;
				}
				catch (ion::uri::invalid_uri const &exc)
				{
					std::cout << ion::recombine_command_line("invalid_uri", boost::assign::list_of(exc.what())) << std::endl;
				}
			}
			break;
		}


		default:
			break;
	}


	return 0;
}

