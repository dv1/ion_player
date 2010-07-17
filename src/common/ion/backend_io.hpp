#ifndef ION_BACKEND_IO_HPP
#define ION_BACKEND_IO_HPP


#include <boost/noncopyable.hpp>
#include <ion/command_line_tools.hpp>
#include <ion/message_callback.hpp>
#include "backend_base.hpp"


namespace ion
{


class backend_io:
	private boost::noncopyable
{
public:
	explicit backend_io(std::istream &in, backend_base &backend_, message_callback_t const &message_callback);
	~backend_io();

	void run();
	bool iterate();


protected:
	void send_response(std::string const &command, params_t const &params);
	void send_response(std::string const &line);


	std::istream &in;

	backend_base &backend_;
	message_callback_t message_callback;
};


}


#endif

