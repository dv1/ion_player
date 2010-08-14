#ifndef TESTCLASS_HPP_______
#define TESTCLASS_HPP_______


#include <QProcess>
#include <ion/frontend_base.hpp>
#include "../frontend/simple_playlist.hpp"


class testclass:
	public QObject
{
	Q_OBJECT
public:
	explicit testclass();
	~testclass();


protected slots:
	void try_read_stdout_line();
	void try_read_stderr_line();
	void started();


protected:
	void print_backend_line(std::string const &line);
	void current_uri_changed(ion::uri_optional_t const &new_uri);


	QProcess backend_process;
	typedef ion::frontend_base < ion::frontend::simple_playlist > frontend_t;
	frontend_t *frontend_;
	ion::frontend::simple_playlist simple_playlist_;
};


#endif
