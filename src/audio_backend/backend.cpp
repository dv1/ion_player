#include <exception>
#include <iostream>
#include <boost/assign/list_of.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/locks.hpp>
#include <ion/metadata.hpp>
#include <ion/resource_exceptions.hpp>
#include "backend.hpp"


namespace ion
{
namespace audio_backend
{


backend::backend():
	current_volume(decoder::max_volume()),
	loop_count(-1)
{
	magic_handle = magic_open(MAGIC_MIME_TYPE | MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_TAR | MAGIC_NO_CHECK_APPTYPE | MAGIC_NO_CHECK_ELF | MAGIC_NO_CHECK_TEXT | MAGIC_NO_CHECK_CDF | MAGIC_NO_CHECK_ENCODING | MAGIC_NO_CHECK_FORTRAN | MAGIC_NO_CHECK_TROFF);
	magic_load(magic_handle, 0);
}


backend::backend(send_command_callback_t const &send_command_callback):
	send_command_callback(send_command_callback),
	current_volume(decoder::max_volume()),
	loop_count(-1)
{
}


backend::~backend()
{
	if (current_sink)
		current_sink->stop();
	magic_close(magic_handle);
}


void backend::set_send_command_callback(send_command_callback_t const &new_send_command_callback)
{
	send_command_callback = new_send_command_callback;
}


decoder::metadata_str_optional_t backend::get_metadata(std::string const &uri_str)
{
	decoder_ptr_t temp_decoder = create_new_decoder(uri_str, "", empty_metadata());
	if (temp_decoder)
		return temp_decoder->get_metadata_str();
	else
		return boost::none;
}


// Helper functions for getting/setting values
namespace
{

// Using a getter function instead of a direct value, to allow for lazy data retrieval (a value would imply that something gets said value in each call, every time)
template < typename Getter >
inline void exec_command_get_value(decoder_ptr_t decoder_, std::string const &response_command_in, Getter const &getter, std::string &response_command_out, params_t &response_params)
{
	if (decoder_)
	{
		response_command_out = response_command_in;
		response_params.clear();
		response_params.push_back(boost::lexical_cast < param_t > (getter()));
	}
	else
		throw std::runtime_error("decoder is not present (possible reason: nothing is currently being played)");
}

template < typename Param, typename Setter >
inline void exec_command_set_value(Setter const &setter, params_t const &params)
{
	if (params.size() == 0)
		throw std::invalid_argument(std::string("missing arguments"));
	Param param = boost::lexical_cast < Param > (params[0]);
	setter(param);
}

}


void backend::exec_command(std::string const &command, params_t const &params, std::string &response_command, params_t &response_params)
{
	// Main command dispatch function.

	try
	{
		#define DECODER_GUARD boost::lock_guard < boost::mutex > lock(decoder_mutex)


		if (command == "play")
		{
			start_playback(params);
			response_command = "";
		}
		else if (command == "stop")
		{
			stop_playback();
			response_command = "";
		}
		else if (command == "resume")
		{
			if (current_sink)
				current_sink->resume();
			response_command = "";
		}
		else if (command == "pause")
		{
			if (current_sink)
				current_sink->pause();
			response_command = "";
		}
		else if (command == "set_next_resource")
		{
			set_next_resource(params);
			response_command = "";
		}
		else if (command == "clear_next_resource")
		{
			clear_next_resource();
			response_command = "";
		}
		else if (command == "get_modules")
		{
			generate_modules_list(response_command, response_params);
		}
		else if (command == "get_loop_mode")
		{
			response_command = "loop_mode";
			response_params.push_back(boost::lexical_cast < std::string > (loop_count));
		}
		else if (command == "set_loop_mode")
		{
			set_loop_mode(params, response_command, response_params);
		}
		else if (command == "set_current_position")
		{
			DECODER_GUARD;
			if (current_decoder)
				exec_command_set_value < long > (boost::lambda::bind(&decoder::set_current_position, current_decoder.get(), boost::lambda::_1), params);
		}
		else if (command == "set_current_volume")
		{
			DECODER_GUARD;
			if (current_decoder)
			{
				exec_command_set_value < long > (boost::lambda::bind(&decoder::set_current_volume,   current_decoder.get(), boost::lambda::_1), params);
				current_volume = current_decoder->get_current_volume();
			}
		}
		else if (command == "get_current_position")
		{
			DECODER_GUARD;
			if (current_decoder)
				exec_command_get_value(current_decoder, "current_position", boost::lambda::bind(&decoder::get_current_position, current_decoder.get()), response_command, response_params);
		}
		else if (command == "get_current_volume")
		{
			response_command = "current_volume";
			response_params.push_back(boost::lexical_cast < std::string > (current_volume));
		}
		else if (command == "get_module_ui")
		{
			if (params.size() == 0)
				throw std::invalid_argument(std::string("missing arguments"));

			// TODO: tighten this code; for instance, put the two maps in an MPL sequence, which can be iterated over
			{
				decoder_creators_t::ordered_t::iterator iter = decoder_creators.get < decoder_creators_t::ordered_tag > ().find(params[0]);
				if (iter != decoder_creators.get < decoder_creators_t::ordered_tag > ().end())
				{
					module_ui ui = (*iter)->get_ui();
					response_command = "module_ui";
					response_params.push_back(params[0]);
					response_params.push_back(ui.html_code);
					response_params.push_back(get_metadata_string(ui.properties));
				}
			}

			{
				sink_creators_t::ordered_t::iterator iter = sink_creators.get < sink_creators_t::ordered_tag > ().find(params[0]);
				if (iter != sink_creators.get < sink_creators_t::ordered_tag > ().end())
				{
					module_ui ui = (*iter)->get_ui();
					response_command = "module_ui";
					response_params.push_back(params[0]);
					response_params.push_back(ui.html_code);
					response_params.push_back(get_metadata_string(ui.properties));
				}
			}
		}
		/*else if (command == "get_metadata")
		{
			DECODER_GUARD;
			if (current_decoder)
				exec_command_get_value(current_decoder, "metadata",   boost::lambda::bind(&decoder::get_metadata_str,   current_decoder.get(), true), response_command, response_params);
		}*/
		else
		{
			response_command = "unknown_command";
			response_params.clear();
			response_params.push_back(command);
		}


		#undef DECODER_GUARD
	}
	catch (resource_not_found const &exc)
	{
		response_command = "resource_not_found";
		response_params.clear();
		response_params.push_back(exc.what());
	}
	catch (ion::uri::invalid_uri const &exc)
	{
		response_command = "invalid_uri";
		response_params.clear();
		response_params.push_back(exc.what());
	}
	catch (std::exception const &exc)
	{
		response_command = "error";
		response_params.clear();
		response_params.push_back(exc.what());
		response_params.push_back(command);
		BOOST_FOREACH(param_t const &param, params)
		{
			response_params.push_back(param);
		}
	}
}


void backend::start_playback(params_t const &params)
{
	if (!current_sink)
		throw std::runtime_error("no sink initialized");

	if (params.empty())
		throw std::invalid_argument("no URL specified");

	std::string uri_str[2], decoder_type[2];
	uri_str[0] = params[0];
	if (params.size() >= 3) uri_str[1] = params[2];

	metadata_t metadata[2];
	if (params.size() >= 2) { metadata[0] = checked_parse_metadata(params[1], "current uri metadata is not a valid JSON object"); decoder_type[0] = metadata[0].get("decoder_type", "").asString(); }
	if (params.size() >= 4) { metadata[1] = checked_parse_metadata(params[3], "next uri metadata is not a valid JSON object"); decoder_type[1] = metadata[1].get("decoder_type", "").asString(); }


	boost::lock_guard < boost::mutex > lock(decoder_mutex);


	decoder_ptr_t new_current_decoder = create_new_decoder(uri_str[0], decoder_type[0], metadata[0]);
	if (!new_current_decoder)
		throw std::runtime_error("start_playback failed : no suitable decoder found");

	if (!new_current_decoder->is_initialized())
		throw std::runtime_error("succeeded, but created invalid decoder");

	decoder_ptr_t old_current_decoder = current_decoder;
	decoder_ptr_t old_next_decoder = next_decoder;

	try
	{
		set_next_decoder(uri_str[1], decoder_type[1], metadata[1]);
	}
	catch (resource_not_found const &exc)
	{
		send_command_callback("resource_not_found", boost::assign::list_of(uri_str[1]));
	}
	catch (std::runtime_error const &exc)
	{
		params_t response_params;

		response_params.clear();
		response_params.push_back(exc.what());
		response_params.push_back("play");
		BOOST_FOREACH(param_t const &param, params)
		{
			response_params.push_back(param);
		}

		send_command_callback("error", response_params);
	}

	// this assignment happens -after- the set_next_decoder() call in case the function throws an exception
	current_decoder = new_current_decoder;

	try
	{
		current_sink->start(current_decoder, next_decoder);
	}
	catch (std::runtime_error const &exc)
	{
		current_decoder = old_current_decoder;
		next_decoder = old_next_decoder;
		throw exc;
	}
}


void backend::set_next_decoder(std::string const &uri_str, std::string const &decoder_type, metadata_t const &metadata)
{
	decoder_ptr_t new_next_decoder;
	if (uri_str.empty())
	{
		send_command_callback("info", boost::assign::list_of("no next decoder given -> not setting a next decoder"));
	}
	else
		new_next_decoder = create_new_decoder(uri_str, decoder_type, metadata);

	if (new_next_decoder && !new_next_decoder->is_initialized())
		throw std::runtime_error("created decoder is invalid -> not setting it as next decoder");

	next_decoder = new_next_decoder;
}


void backend::set_next_resource(params_t const &params)
{
	if (!current_sink)
		throw std::runtime_error("no sink initialized");

	if (params.empty())
		throw std::invalid_argument("no URL specified");

	decoder_mutex.lock();

	if (!current_decoder)
	{
		decoder_mutex.unlock();
		start_playback(params);
		return;
	}

	boost::unique_lock < boost::mutex > lock(decoder_mutex, boost::adopt_lock_t());

	std::string uri_str = params[0], decoder_type;
	metadata_t metadata;
	if (params.size() >= 2) { metadata = checked_parse_metadata(params[1], "metadata is not a valid JSON object"); decoder_type = metadata.get("decoder_type", "").asString(); }

	// in case of failure, an exception is thrown inside set_next_decoder(), this function exits, and the next decoder isn't set
	set_next_decoder(uri_str, decoder_type, metadata);
	current_sink->set_next_decoder(next_decoder);
}


void backend::clear_next_resource()
{
	if (!current_sink)
		throw std::runtime_error("no sink initialized");

	boost::lock_guard < boost::mutex > lock(decoder_mutex);
	next_decoder = decoder_ptr_t();

	current_sink->clear_next_decoder();
}


void backend::stop_playback()
{
	if (current_sink)
		current_sink->stop(); // this notifies about the stop (stop has a boolean parameter, which defaults true)

	boost::lock_guard < boost::mutex > lock(decoder_mutex);

	current_decoder = decoder_ptr_t();
	next_decoder = decoder_ptr_t();
}


void backend::set_loop_mode(params_t const &params, std::string &response_command_out, params_t &response_params)
{
	if (params.empty())
		throw std::invalid_argument("no count specified");

	loop_count = boost::lexical_cast < int > (params[0]);

	boost::lock_guard < boost::mutex > lock(decoder_mutex);
	if (current_decoder)
		current_decoder->set_loop_mode(loop_count);
	if (next_decoder)
		next_decoder->set_loop_mode(loop_count);

	response_command_out = "loop_mode";
	response_params.push_back(boost::lexical_cast < std::string > (loop_count));
}


void backend::generate_modules_list(std::string &response_command_out, params_t &response_params)
{
	response_command_out = "modules";

	// TODO: tighten this code; for instance, put the three maps in an MPL sequence, which can be iterated over

	BOOST_FOREACH(source_creators_t::creators_t::value_type const &value, source_creators)
	{
		response_params.push_back("source");
		response_params.push_back(value->get_type());
	}

	BOOST_FOREACH(decoder_creators_t::creators_t::value_type const &value, decoder_creators)
	{
		response_params.push_back("decoder");
		response_params.push_back(value->get_type());
	}

	BOOST_FOREACH(sink_creators_t::creators_t::value_type const &value, sink_creators)
	{
		response_params.push_back("sink");
		response_params.push_back(value->get_type());
	}
}


void backend::create_sink(std::string const &type)
{
	// This call does not use the decoder mutex, since the sink gets paused (and is eventually stopped).

	sink_creators_t::ordered_t::iterator iter = sink_creators.get < sink_creators_t::ordered_tag > ().find(type);
	if (iter == sink_creators.get < sink_creators_t::ordered_tag > ().end())
		return;

	// First, pause any existing sink
	if (current_sink)
		current_sink->pause(false); // false means no notification

	sink_ptr_t new_sink = (*iter)->create(send_command_callback);
	if (new_sink)
	{
		/* If control reaches this scope, then the new sink was successfully created, and the
		current one is not expected to be functional anymore. Let the new sink take over.
		If a current decoder exists, then the current sink was playing something. Start playback
		of the current decoder in this case (also passing the next decoder as well).
		If no decoder exists, then no playback took place -> just set the new sink to be the
		current one. */

		// But before the steps above are taken, do two things:

		// 1) Set the new sink's song finished callback
		new_sink->set_resource_finished_callback(boost::lambda::bind(&backend::resource_finished_callback, this));
		
		// 2) Check for an edge case where the current decoder
		// is gone but has a succeeding next decoder. If so, set current to next, and then next to
		// nil. TODO: this case should never occur. See if this check is really necessary; if not,
		// remove it.
		if (next_decoder && !current_decoder)
		{
			send_command_callback("info", boost::assign::list_of("encountered a nil current decoder and a non-nil next decoder - compensating"));

			current_decoder = next_decoder;
			next_decoder = decoder_ptr_t();
		}

		if (current_decoder)
		{
			/*
			The decoders are -not- reset in any way, meaning that the sink change should not
			cause the decoders to revert to the beginning of a song for example, even though
			a handshake procedure would happen.
			*/

			// sinks do not "stop" the decoder somehow
			// this is important for the decoder handover from sink to sink
			if (current_sink)
				current_sink->stop(false); // false means: do not notify about the stop
			current_sink = new_sink;
			current_sink->start(current_decoder, next_decoder);
		}
		else
		{
			current_sink = new_sink;
		}
	}
	else
	{
		/* If control reaches this scope, then creating the new sink was unsuccessful,
		and the current sink is left unchanged. Clean up any half-created new sink and resume playback. */
		if (new_sink)
			new_sink = sink_ptr_t();
		if (current_sink)
			current_sink->resume(false); // false means no notification
	}
}


void backend::resource_finished_callback()
{
	boost::lock_guard < boost::mutex > lock(decoder_mutex);

	std::cerr << "[DEBUG] resource finished callback invoked" << std::endl;

	current_decoder = next_decoder;
	next_decoder = decoder_ptr_t();
}


metadata_t backend::checked_parse_metadata(std::string const &metadata_str, std::string const &error_msg)
{
	metadata_optional_t temp_result = parse_metadata(metadata_str);
	if (temp_result)
		return *temp_result;
	else
		throw std::invalid_argument(error_msg);
}


source_ptr_t backend::create_new_source(ion::uri const &uri_)
{
	source_creators_t::ordered_t::iterator source_creator_iter = source_creators.get < source_creators_t::ordered_tag > ().find(uri_.get_type());
	if (source_creator_iter == source_creators.get < source_creators_t::ordered_tag > ().end())
		return source_ptr_t();

	return (*source_creator_iter)->create(uri_);
}


decoder_ptr_t backend::create_new_decoder(std::string const &uri_str, std::string const &decoder_type, metadata_t const &metadata)
{
	// Read uri and try creating a new source for the decoder
	ion::uri uri_(uri_str);
	source_ptr_t new_source = create_new_source(uri_); // no suitable source found -> throw
	if (!new_source)
		throw resource_not_found(uri_str);


	// Use libmagic to get a MIME type
	std::string mime_type("<mimetype_not_set>");

	// TODO: this is a hack. It would be better to extend libmagic to accept the new_source instance (or at least a bunch of callbacks for custom I/O).
	if (uri_.get_type() == "file")
	{
		std::string filename = uri_.get_path();
		char const *mime_type_local = magic_file(magic_handle, filename.c_str());
		if (mime_type_local != 0)
			mime_type = mime_type_local;
	}


	// The actual decoder creation

	decoder_ptr_t new_decoder;

	// Try the given decoder type first
	if (!decoder_type.empty())
	{
		decoder_creators_t::ordered_t::iterator iter = decoder_creators.get < decoder_creators_t::ordered_tag > ().find(decoder_type);
		if (iter != decoder_creators.get < decoder_creators_t::ordered_tag > ().end())
		{
			new_decoder = (*iter)->create(new_source, metadata, send_command_callback, mime_type);
		}
	}

	// If the given decoder type was not set, or invalid, or the creator failed, try all decoder creators
	if (!new_decoder)
	{
		BOOST_FOREACH(decoder_creator *decoder_creator_, decoder_creators.get < decoder_creators_t::sequence_tag > ())
		{
			try
			{
				new_source->reset();

				new_decoder = decoder_creator_->create(new_source, metadata, send_command_callback, mime_type);
				if (new_decoder)
					break;
			}
			catch (std::exception const &exc)
			{
				std::cerr << "Exception thrown while trying out decoder creator " << decoder_creator_->get_type() << ": " << exc.what() << std::endl;
			}
		}
	}


	// Apply some initial settings on the new decoder
	if (new_decoder)
	{
		new_decoder->set_current_volume(current_volume);
		new_decoder->set_loop_mode(loop_count);
	}


	return new_decoder;
}



void set_send_command_callback(backend &backend_, send_command_callback_t const &new_send_command_callback)
{
	backend_.set_send_command_callback(new_send_command_callback);
}


std::string get_backend_type(backend const &backend_)
{
	return backend_.get_type();
}


void execute_command(backend &backend_, std::string const &command, params_t const &parameters, std::string &response_command, params_t &response_parameters)
{
	backend_.exec_command(command, parameters, response_command, response_parameters);
}


}
}
