#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "testclass.hpp"


testclass::testclass():
	backend_handler_(0)
{
	backend_handler_ = new ion::frontend::backend_handler(boost::lambda::bind(&testclass::print_backend_line, this, boost::lambda::_1));

	simple_playlist_.entries.push_back(ion::frontend::simple_playlist::entry(ion::uri("file:///home/carlos/hd2/audio/mods/witness.mod"), ion::metadata_t("{}")));
	simple_playlist_.entries.push_back(ion::frontend::simple_playlist::entry(ion::uri("file:///home/carlos/hd2/audio/mods/glowbug.s3m"), ion::metadata_t("{}")));
	simple_playlist_.entries.push_back(ion::frontend::simple_playlist::entry(ion::uri("file:///home/carlos/hd2/audio/mods/suspiria.xm"), ion::metadata_t("{}")));

	backend_handler_->set_playlist(simple_playlist_);

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
	delete backend_handler_;
}


void testclass::started()
{
	backend_handler_->play(ion::uri("file:///home/carlos/hd2/audio/mods/witness.mod"));
//	backend_process.write("get_backend_type\n");
}


void testclass::try_read_stdout_line()
{
	if (!backend_process.canReadLine())
		return;

	QString line = backend_process.readLine().trimmed();
	std::cout << "stdout> " << line.toStdString() << std::endl;
	backend_handler_->parse_received_backend_line(line.toStdString());
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

