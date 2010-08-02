#include <QCoreApplication>
#include <QFileInfo>

#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "testclass.hpp"


testclass::testclass():
	frontend_io_(0)
{
	frontend_io_ = new frontend_io_t(boost::lambda::bind(&testclass::print_backend_line, this, boost::lambda::_1));

	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=1"), ion::metadata_t("{}")));
	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=2"), ion::metadata_t("{}")));
	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=3"), ion::metadata_t("{}")));
	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=4"), ion::metadata_t("{}")));

	frontend_io_->get_current_uri_changed_signal().connect(boost::lambda::bind(&testclass::current_uri_changed, this, boost::lambda::_1));
	frontend_io_->set_current_playlist(simple_playlist_);

	connect(&backend_process, SIGNAL(readyRead()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(readyReadStandardOutput()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(readyReadStandardError()), this, SLOT(try_read_stderr_line()));
	connect(&backend_process, SIGNAL(started()), this, SLOT(started()));
	connect(&backend_process, SIGNAL(error(QProcess::ProcessError)), QCoreApplication::instance(), SLOT(quit()));
	connect(&backend_process, SIGNAL(finished(int, QProcess::ExitStatus)), QCoreApplication::instance(), SLOT(quit()));
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
	while (backend_process.canReadLine())
	{
		QString line = backend_process.readLine().trimmed();
		std::cerr << "stdout> " << line.toStdString() << std::endl;
		frontend_io_->parse_incoming_line(line.toStdString());

		if (line.startsWith("resource_finished"))
			print_backend_line("quit");
	}

}


void testclass::try_read_stderr_line()
{
}


void testclass::print_backend_line(std::string const &line)
{
	std::cerr << "stdin> " << line << std::endl;
	backend_process.write((line + "\n").c_str());
}


void testclass::current_uri_changed(ion::uri_optional_t const &new_uri)
{
	if (new_uri)
		std::cerr << "Current URI changed to " << new_uri->get_full() << std::endl;
	else
		std::cerr << "Current URI cleared" << std::endl;
}


#include "testclass.moc"

