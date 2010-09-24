#include <boost/shared_ptr.hpp>
#include <ion/backend_main_loop.hpp>
#include <ion/resource_exceptions.hpp>
#include "backend.hpp"
#include "alsa_sink.hpp"
#include "file_source.hpp"
#include "dumb_decoder.hpp"
#include "mpg123_decoder.hpp"
#include "vorbis_decoder.hpp"
#include "adplug_decoder.hpp"
#include "flac_decoder.hpp"
#include "gme_decoder.hpp"


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
	ion::audio_backend::alsa_sink_creator alsa_sink_creator_;
	ion::audio_backend::file_source_creator file_source_creator_;
	ion::audio_backend::dumb_decoder_creator dumb_decoder_creator_;
	ion::audio_backend::mpg123_decoder_creator mpg123_decoder_creator_;
	ion::audio_backend::vorbis_decoder_creator vorbis_decoder_creator_;
	ion::audio_backend::adplug_decoder_creator adplug_decoder_creator_;
	ion::audio_backend::gme_decoder_creator gme_decoder_creator_;
	ion::audio_backend::flac_decoder_creator flac_decoder_creator_;


	explicit creators(ion::audio_backend::backend &backend_)
	{
		backend_.get_source_creators().push_back(&file_source_creator_);
		backend_.get_sink_creators().push_back(&alsa_sink_creator_);
		backend_.get_decoder_creators().push_back(&dumb_decoder_creator_);
		backend_.get_decoder_creators().push_back(&mpg123_decoder_creator_);
		backend_.get_decoder_creators().push_back(&vorbis_decoder_creator_);
		backend_.get_decoder_creators().push_back(&flac_decoder_creator_);
		backend_.get_decoder_creators().push_back(&adplug_decoder_creator_);
		backend_.get_decoder_creators().push_back(&gme_decoder_creator_);
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
			backend_.create_sink("alsa"); // Use the alsa sink for sound output (TODO: this is platform specific; on Windows, one would use Waveout, on OSX it would be CoreAudio etc.)

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
					ion::audio_backend::decoder::metadata_str_optional_t metadata_str = backend_.get_metadata(url);
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

