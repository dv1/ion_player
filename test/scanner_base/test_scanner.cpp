#include <stddef.h>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDateTime>
#include "test_scanner.hpp"


test_scanner::test_scanner(playlists_t &playlists_, int const max_num_entries_added):
	base_t(playlists_),
	num_entries_added(max_num_entries_added)
{
	connect(&tick_timer, SIGNAL(timeout()), this, SLOT(print_tick()));
	tick_timer.setSingleShot(false);
	tick_timer.start(1000);

	connect(&watchdog_timer, SIGNAL(timeout()), this, SLOT(watchdog_timeout()));
	watchdog_timer.setSingleShot(false);

	connect(&kill_timer, SIGNAL(timeout()), this, SLOT(kill_timeout()));
	kill_timer.setSingleShot(false);

	connect(&backend_process, SIGNAL(readyRead()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(readyReadStandardOutput()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(started()), this, SLOT(started()));
	//connect(&backend_process, SIGNAL(error(QProcess::ProcessError)), QCoreApplication::instance(), SLOT(quit()));
	connect(&backend_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
	backend_process.setReadChannel(QProcess::StandardOutput);
	backend_process.setProcessChannelMode(QProcess::SeparateChannels);

	start_backend();
}


test_scanner::~test_scanner()
{
	tick_timer.stop();

	queue.clear();

	std::cerr << "Quitting\n";
	backend_process.waitForFinished(3000);
	if (backend_process.state() != QProcess::NotRunning)
		terminate_backend();
}


void test_scanner::send_to_backend(std::string const &command_line)
{
	std::cerr << '[' << QDateTime::currentDateTime().toString().toStdString() << "]  output> " << command_line << std::endl;
	backend_process.write(std::string(command_line + '\n').c_str());
}


void test_scanner::restart_watchdog_timer()
{
	std::cerr << "restart_watchdog_timer" << std::endl;
	watchdog_timer.start(2000);
}


void test_scanner::stop_watchdog_timer()
{
	std::cerr << "stop_watchdog_timer" << std::endl;
	watchdog_timer.stop();
}


void test_scanner::restart_backend()
{
	std::cerr << "restart_backend" << std::endl;
	start_backend();
}


void test_scanner::adding_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before)
{
	std::cerr << "adding_queue_entry " << uri_.get_full() << ' ' << uintptr_t(&playlist_) << ' ' << before << std::endl;
}


void test_scanner::removing_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before)
{
	std::cerr << "removing_queue_entry " << uri_.get_full() << ' ' << uintptr_t(&playlist_) << ' ' << before << std::endl;
}


void test_scanner::scanning_in_progress(bool const state)
{
	std::cerr << "scanning_in_progress " << state << std::endl;

	if (!state)
	{
		std::cerr << "quit" << std::endl;
		backend_process.write("quit\n");
		QCoreApplication::instance()->quit();
	}
}


void test_scanner::resource_successfully_scanned(ion::uri const &uri_, playlist_t &playlist_, ion::metadata_t const &new_metadata)
{
	std::cerr << "Adding entry: URI " << uri_.get_full() << " current playlist " << uintptr_t(&playlist_) << " metadata " << ion::get_metadata_string(new_metadata) << std::endl;
	--num_entries_added;
}


void test_scanner::unrecognized_resource(ion::uri const &uri_, playlist_t &playlist_)
{
	std::cerr << "unrecognized_resource " << uri_.get_full() << std::endl;
}


void test_scanner::resource_corrupted(ion::uri const &uri_, playlist_t &playlist_)
{
	std::cerr << "resource_corrupted " << uri_.get_full() << std::endl;
}


void test_scanner::scanning_failed(ion::uri const &uri_, playlist_t &playlist_)
{
	std::cerr << "scanning_failed " << uri_.get_full() << std::endl;
}


void test_scanner::try_read_stdout_line()
{
	while (backend_process.canReadLine())
	{
		QString line = backend_process.readLine().trimmed();
		std::cerr << '[' << QDateTime::currentDateTime().toString().toStdString() << "]  scan backend stdout> " << line.toStdString() << std::endl;

		parse_backend_event(line.toStdString());
	}
}


void test_scanner::started()
{
}


void test_scanner::finished(int exit_code, QProcess::ExitStatus exit_status)
{
	kill_timer.stop();

	switch (exit_status)
	{
		case QProcess::CrashExit: std::cerr << "backend crashed\n"; backend_crashed(); break;
		default: break;
	};
}


void test_scanner::print_tick()
{
	std::cerr << '[' << QDateTime::currentDateTime().toString().toStdString() << "]  ################ PING" << std::endl;
}


void test_scanner::watchdog_timeout()
{
	std::cerr << "######## WATCHDOG TIMEOUT" << std::endl;

	terminate_backend();
}


void test_scanner::start_backend()
{
	char const *exec_names[] = {
		"build/release/src/audio_backend/ion_audio_backend",
		"build/debug/src/audio_backend/ion_audio_backend",
		0
	};

	bool found_exec = false;
	for (char const **exec_name = exec_names; *exec_name != 0; ++exec_name)
	{
		if (QFileInfo(*exec_name).exists())
		{
			std::cerr << "Starting \"" << *exec_name << "\"\n";
			backend_process.start(*exec_name);
			backend_process.waitForStarted(30000);
			found_exec = true;
			break;
		}
	}

	if (!found_exec)
		std::cerr << "Could not find any backend executable" << std::endl;
}


void test_scanner::terminate_backend(bool const start_kill_timer)
{
	if (backend_process.state() != QProcess::NotRunning)
	{
		backend_process.terminate();
		std::cerr << "sending scan backend the TERM signal" << std::endl;
	}
	if (start_kill_timer)
		kill_timer.start(30000);
}


void test_scanner::kill_timeout()
{
	if (backend_process.state() != QProcess::NotRunning)
	{
		backend_process.kill();
		std::cerr << "sending scan backend the KILL signal" << std::endl;
	}
}


#include "test_scanner.moc"

