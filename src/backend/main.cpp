#include <boost/shared_ptr.hpp>
#include <ion/backend_main_loop.hpp>
#include "backend.hpp"
#include "alsa_sink.hpp"
#include "file_source.hpp"
#include "dumb_decoder.hpp"
#include "mpg123_decoder.hpp"


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
	ion::backend::alsa_sink_creator alsa_sink_creator_;
	ion::backend::file_source_creator file_source_creator_;
	ion::backend::dumb_decoder_creator dumb_decoder_creator_;
	ion::backend::mpg123_decoder_creator mpg123_decoder_creator_;


	explicit creators(ion::backend::backend &backend_)
	{
		backend_.get_sink_creators()["alsa"] = &alsa_sink_creator_;
		backend_.get_decoder_creators()["dumb"] = &dumb_decoder_creator_;
		backend_.get_decoder_creators()["mpg123"] = &mpg123_decoder_creator_;
		backend_.get_source_creators()["file"] = &file_source_creator_;
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


	ion::backend::backend backend_;
	creators_ptr_t creators_;


	switch (run_mode)
	{
		case default_run_mode:
		{

			ion::backend_main_loop < ion::backend::backend > backend_main_loop(std::cin, std::cout, backend_);
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
				std::string const &url = params[i];
				std::cout << ion::recombine_command_line("metadata", boost::assign::list_of(url)(backend_.get_metadata(url))) << std::endl;
			}
			break;
		}


		default:
			break;
	}


	return 0;
}

