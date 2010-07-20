#ifndef TESTCLASS_HPP_______
#define TESTCLASS_HPP_______


#include <QProcess>
#include "backend_handler.hpp"
#include "simple_playlist.hpp"


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


	QProcess backend_process;
	ion::frontend::backend_handler *backend_handler_;
	ion::frontend::simple_playlist simple_playlist_;
};


#endif
