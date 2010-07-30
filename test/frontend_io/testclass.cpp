#include <QCoreApplication>
#include <QFileInfo>

#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "testclass.hpp"


testclass::testclass():
	frontend_io_(0)
{
	frontend_io_ = new ion::frontend_io(boost::lambda::bind(&testclass::print_backend_line, this, boost::lambda::_1));

	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=1"), ion::metadata_t("{}")));
	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=2"), ion::metadata_t("{}")));
	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=3"), ion::metadata_t("{}")));
	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=4"), ion::metadata_t("{}")));

	frontend_io_->set_current_playlist(simple_playlist_);

	connect(&backend_process, SIGNAL(readyRead()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(readyReadStandardOutput()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(readyReadStandardError()), this, SLOT(try_read_stderr_line()));
	connect(&backend_process, SIGNAL(started()), this, SLOT(started()));
	connect(&backend_process, SIGNAL(error()), QCoreApplication::instance(), SLOT(quit()));
	connect(&backend_process, SIGNAL(finished()), QCoreApplication::instance(), SLOT(quit()));
	backend_process.setReadChannel(QProcess::StandardOutput);
	backend_process.setProcessChannelMode(QProcess::SeparateChannels);

	char const *exec_names[] = {
		"build/release/src/backend/backend",
		"build/debug/src/backend/backend",
		0
	};

	bool found_exec = false;
	for (char const **exec_name = exec_names; *exec_name != 0; ++exec_name)
	{
		if (QFileInfo(*exec_name).exists())
		{
			backend_process.start(*exec_name);
			found_exec = true;
			break;
		}
	}

	if (!found_exec)
		std::cerr << "Could not find any backend executable" << std::endl;
}


testclass::~testclass()
{
	delete frontend_io_;
}


void testclass::started()
{
	frontend_io_->play(ion::uri("file://test/sound_samples/mods/test.xm?id=1"));
//	backend_process.write("get_backend_type\n");
}


void testclass::try_read_stdout_line()
{
	if (!backend_process.canReadLine())
		return;

	QString line = backend_process.readLine().trimmed();
	std::cout << "stdout> " << line.toStdString() << std::endl;
	frontend_io_->parse_incoming_line(line.toStdString());

	if (line.startsWith("resource_finished"))
		print_backend_line("quit");
}


void testclass::try_read_stderr_line()
{
	if (!backend_process.canReadLine())
		return;

	QString line = backend_process.readLine();
	std::cout << "stderr> " << line.toStdString() << std::endl;
}


void testclass::print_backend_line(std::string const &line)
{
	std::cout << "stdin> " << line << std::endl;
	backend_process.write((line + "\n").c_str());
}


#include "testclass.moc"

