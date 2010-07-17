#include <ion/backend_io.hpp>
#include "backend.hpp"
#include "alsa_sink.hpp"
#include "file_source.hpp"
#include "dumb_decoder.hpp"
#include "mpg123_decoder.hpp"


void send_message(std::string const &command, ion::params_t const &params)
{
	std::cout << ion::recombine_command_line(command, params) << std::endl;
}



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


	switch (run_mode)
	{
		case default_run_mode:
		{
			ion::backend::alsa_sink_creator alsa_sink_creator_;
			ion::backend::file_source_creator file_source_creator_;
			ion::backend::dumb_decoder_creator dumb_decoder_creator_;
			ion::backend::mpg123_decoder_creator mpg123_decoder_creator_;

			ion::backend::backend backend_(send_message);

			backend_.get_sink_creators()["alsa"] = &alsa_sink_creator_;
			backend_.get_decoder_creators()["dumb"] = &dumb_decoder_creator_;
			backend_.get_decoder_creators()["mpg123"] = &mpg123_decoder_creator_;
			backend_.get_source_creators()["file"] = &file_source_creator_;

			backend_.create_sink("alsa");


			ion::backend_io backend_io(std::cin, backend_, send_message);
			backend_io.run();

			break;
		}


		case print_id_mode:
		{
			ion::backend::backend backend_(send_message);
			std::cout << backend_.get_type() << std::endl;
			break;
		}


		case get_resource_info_mode:
		{
			ion::backend::file_source_creator file_source_creator_;
			ion::backend::dumb_decoder_creator dumb_decoder_creator_;
			ion::backend::backend backend_(send_message);
			backend_.get_source_creators()["file"] = &file_source_creator_;
			backend_.get_decoder_creators()["dumb"] = &dumb_decoder_creator_;

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

