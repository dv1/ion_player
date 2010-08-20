#include "scanner.hpp"


namespace ion
{
namespace frontend
{


scanner::scanner(QObject *parent, QString const &backend_filepath):
	QObject(parent),
	backend_filepath(backend_filepath)
{
	connect(&backend_process, SIGNAL(readyRead()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(started()), this, SLOT(started()));
	connect(&backend_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
	backend_process.setReadChannel(QProcess::StandardOutput);
	backend_process.setProcessChannelMode(QProcess::SeparateChannels);
}


scanner::~scanner()
{
	scan_queue.clear();

	backend_process.waitForFinished(30000);
	if (backend_process.state() != QProcess::NotRunning)
	{
		backend_process.terminate();
		std::cerr << "sending scan backend the TERM signal" << std::endl;
	}
	backend_process.waitForFinished(30000);
	if (backend_process.state() != QProcess::NotRunning)
	{
		backend_process.kill();
		std::cerr << "sending scan backend the KILL signal" << std::endl;
	}
}


bool scanner::is_process_running() const
{
	return (backend_process.state() != QProcess::NotRunning);
}


void scanner::start_process(ion::uri const &uri_to_be_scanned)
{
	backend_process.start(backend_filepath, (QStringList() << "-info" << uri_to_be_scanned.get_full().c_str()), QIODevice::ReadOnly);
	backend_process.waitForStarted(30000);
}


void scanner::add_entry_to_playlist(ion::uri const &new_uri, ion::metadata_t const &new_metadata)
{
	current_playlist->add_entry(playlist::entry_t(new_uri, new_metadata));
}


void scanner::try_read_stdout_line()
{
	while (backend_process.canReadLine())
	{
		QString line = backend_process.readLine().trimmed();
		if (!line.isNull())
			std::cerr << "scan backend stdout> " << line.toStdString() << std::endl;
		else
			std::cerr << "scan backend received null line?" << std::endl;

		read_process_stdin_line(line.toStdString());
	}
}


void scanner::started()
{
	scanning_process_started();
}


void scanner::finished(int exit_code, QProcess::ExitStatus exit_status)
{
	switch (exit_status)
	{
		case QProcess::NormalExit: scanning_process_finished(true); break;
		case QProcess::CrashExit: scanning_process_terminated(); break;
	};
}


}
}


#include "scanner.moc"

