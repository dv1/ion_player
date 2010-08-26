#include <stddef.h>
#include <QCoreApplication>
#include <QFileInfo>
#include "test_scanner.hpp"


test_scanner::test_scanner(int const max_num_entries_added):
	num_entries_added(max_num_entries_added)
{
	connect(&backend_process, SIGNAL(readyRead()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(readyReadStandardOutput()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(started()), this, SLOT(started()));
	connect(&backend_process, SIGNAL(error(QProcess::ProcessError)), QCoreApplication::instance(), SLOT(quit()));
	connect(&backend_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
	backend_process.setReadChannel(QProcess::StandardOutput);
	backend_process.setProcessChannelMode(QProcess::SeparateChannels);
}


test_scanner::~test_scanner()
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


bool test_scanner::is_process_running() const
{
	return (backend_process.state() != QProcess::NotRunning);
}


void test_scanner::start_process(ion::uri const &uri_to_be_scanned)
{
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
			backend_process.start(*exec_name, (QStringList() << "-info" << uri_to_be_scanned.get_full().c_str()), QIODevice::ReadOnly);
			backend_process.waitForStarted(30000);
			found_exec = true;
			break;
		}
	}

	if (!found_exec)
		std::cerr << "Could not find any backend executable" << std::endl;
}


void test_scanner::add_entry_to_playlist(ion::uri const &new_uri, ion::metadata_t const &new_metadata)
{
	std::cerr << "Adding entry: URI " << new_uri.get_full() << " current playlist " << uintptr_t(current_playlist) << " metadata " << ion::get_metadata_string(new_metadata) << std::endl;
	--num_entries_added;
}


void test_scanner::try_read_stdout_line()
{
	while (backend_process.canReadLine())
	{
		QString line = backend_process.readLine().trimmed();
		std::cerr << "scan backend stdout> " << line.toStdString() << std::endl;

		read_process_stdin_line(line.toStdString());
	}

	// Putting it here because the QProcess IODevice may still contain data in its buffer even after the process finished,
	// meaning that the finished() slot may have been called before try_read_stdout_line()
	if (num_entries_added <= 0)
		QCoreApplication::instance()->quit();
}


void test_scanner::started()
{
	scanning_process_started();
}


void test_scanner::finished(int exit_code, QProcess::ExitStatus exit_status)
{
	switch (exit_status)
	{
		case QProcess::NormalExit: scanning_process_finished((num_entries_added > 0)); break;
		case QProcess::CrashExit: scanning_process_terminated((num_entries_added > 0)); break;
	};
}


#include "test_scanner.moc"

