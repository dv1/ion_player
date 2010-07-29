#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "testclass.hpp"


testclass::testclass():
	frontend_io_(0)
{
	frontend_io_ = new ion::frontend_io(boost::lambda::bind(&testclass::print_backend_line, this, boost::lambda::_1));

	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/witness.mod"), ion::metadata_t("{}")));
	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/back_again.mod"), ion::metadata_t("{}")));
	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/spx-shuttledeparture.it"), ion::metadata_t("{}")));
	simple_playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/suspiria.xm"), ion::metadata_t("{}")));

	frontend_io_->set_current_playlist(simple_playlist_);

	connect(&backend_process, SIGNAL(readyRead()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(readyReadStandardOutput()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(readyReadStandardError()), this, SLOT(try_read_stderr_line()));
	connect(&backend_process, SIGNAL(started()), this, SLOT(started()));
	backend_process.setReadChannel(QProcess::StandardOutput);
	backend_process.setProcessChannelMode(QProcess::SeparateChannels);
	backend_process.start("build/debug/src/backend/backend");
}


testclass::~testclass()
{
	delete frontend_io_;
}


void testclass::started()
{
	frontend_io_->play(ion::uri("file://test/sound_samples/mods/witness.mod"));
//	backend_process.write("get_backend_type\n");
}


void testclass::try_read_stdout_line()
{
	if (!backend_process.canReadLine())
		return;

	QString line = backend_process.readLine().trimmed();
	std::cout << "stdout> " << line.toStdString() << std::endl;
	frontend_io_->parse_incoming_line(line.toStdString());
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

