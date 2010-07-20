#ifndef ION_FRONTEND_BACKEND_HANDLER_HPP
#define ION_FRONTEND_BACKEND_HANDLER_HPP

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/signals2/signal.hpp>
#include <ion/uri.hpp>


namespace ion
{
namespace frontend
{


class playlist
{
public:
	typedef boost::optional < uri > uri_optional_t;

	virtual ~playlist() {}
	virtual uri_optional_t get_succeeding_uri(uri const &uri_) const = 0;
};


class backend_handler:
	private boost::noncopyable
{
public:
	typedef boost::optional < uri > uri_optional_t;
	typedef boost::signals2::signal < void(uri_optional_t const &new_current_uri) > current_uri_changed_signal_t;
	typedef boost::function < void(std::string const &line) > send_to_backend_function_t;


	explicit backend_handler(send_to_backend_function_t const &send_to_backend_function);
	~backend_handler();


	void set_playlist(playlist const &new_playlist);


	// Actions

	void play(uri const &uri_);
	void stop();


	// Accessors

	uri_optional_t get_current_uri() const;
	inline current_uri_changed_signal_t & get_current_uri_changed_signal() { return current_uri_changed_signal; } // this is mainly interesting for whatever is interested in the current uri; does not have to be the playlist


	// Backend response parser & event handlers

	void parse_received_backend_line(std::string const &line);
	void transition(uri const &old_uri, uri const &new_uri);
	void started(uri const &uri_);
	void stopped(uri const &uri_);
	void song_finished(uri const &uri_);


	// Playlist event handlers

	void resource_added(uri const &uri_);
	void resource_removed(uri const &uri_);


protected:
	send_to_backend_function_t send_to_backend_function;
	current_uri_changed_signal_t current_uri_changed_signal;

	uri_optional_t current_uri, next_uri;

	playlist const *playlist_;
};


}
}


#endif

