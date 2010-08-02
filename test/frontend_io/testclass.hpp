#ifndef TESTCLASS_HPP_______
#define TESTCLASS_HPP_______


#include <QProcess>
#include <ion/frontend_io.hpp>
#include <ion/simple_playlist.hpp>


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
	typedef ion::frontend_io < ion::simple_playlist > frontend_io_t;
	frontend_io_t *frontend_io_;
	ion::simple_playlist simple_playlist_;
};


#endif
