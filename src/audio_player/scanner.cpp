/**************************************************************************

    Copyright (C) 2010  Carlos Rafael Giani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**************************************************************************/


#include "scanner.hpp"


namespace ion
{
namespace audio_player
{


#if 1


scanner::scanner(QObject *parent, playlists_t &playlists_, QString const &backend_filepath):
	QObject(parent),
	base_t(playlists_),
	backend_filepath(backend_filepath)
{
	connect(&backend_process, SIGNAL(readyRead()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(started()), this, SLOT(started()));
	connect(&backend_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
	backend_process.setReadChannel(QProcess::StandardOutput);
	backend_process.setProcessChannelMode(QProcess::SeparateChannels);

	scan_directory_timer.setSingleShot(false);
	connect(&scan_directory_timer, SIGNAL(timeout()), this, SLOT(scan_directory_entry()));

	backend_process.start(backend_filepath);
	backend_process.waitForStarted(30000);
}


scanner::~scanner()
{
	scan_directory_timer.stop();
	dir_iterator_playlist = 0;
	dir_iterator = dir_iterator_ptr_t();

	queue.clear();

	backend_process.write("quit\n");

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


void scanner::scan_file(playlist_t &playlist_, QString const &filename)
{
	ion::uri uri_(check_if_starts_with_file(filename).toStdString());
	issue_scan_request(uri_, playlist_);
}


void scanner::scan_directory(playlist_t &playlist_, QString const &directory_path)
{
	if (dir_iterator)
		return;

	dir_iterator_playlist = &playlist_;
	dir_iterator = dir_iterator_ptr_t(new QDirIterator(directory_path, QDirIterator::Subdirectories));
	scan_directory_timer.start(0);
}


void scanner::send_to_backend(std::string const &command_line)
{
	backend_process.write((command_line + "\n").c_str());
}


void scanner::restart_watchdog_timer()
{
}


void scanner::stop_watchdog_timer()
{
}


void scanner::restart_backend()
{
}


void scanner::adding_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before)
{
}


void scanner::removing_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before)
{
}


void scanner::scanning_in_progress(bool const state)
{
	emit scan_running(state);
}


void scanner::resource_successfully_scanned(ion::uri const &uri_, playlist_t &playlist_, ion::metadata_t const &new_metadata)
{
	add_entry(playlist_, create_entry(playlist_, uri_, new_metadata), true);
}


void scanner::unrecognized_resource(ion::uri const &uri_, playlist_t &/*playlist_*/)
{
	emit resource_scan_error("unrecognized_resource", uri_.get_full().c_str());
}


void scanner::resource_corrupted(ion::uri const &uri_, playlist_t &/*playlist_*/)
{
	emit resource_scan_error("resource_corrupted", uri_.get_full().c_str());
}


void scanner::scanning_failed(ion::uri const &uri_, playlist_t &/*playlist_*/)
{
	emit resource_scan_error("scanning_failed", uri_.get_full().c_str());
}


void scanner::cancel_scan_slot()
{
	cancel_scan();
	dir_iterator_playlist = 0;
	dir_iterator = dir_iterator_ptr_t();
	scan_directory_timer.stop();
	emit scan_canceled();
}


void scanner::try_read_stdout_line()
{
	while (backend_process.canReadLine())
	{
		QString line = backend_process.readLine().trimmed();
		parse_backend_event(line.toStdString());
	}
}


void scanner::started()
{
}


void scanner::finished(int exit_code, QProcess::ExitStatus exit_status)
{
	switch (exit_status)
	{
		case QProcess::CrashExit: backend_crashed(); break;
		default: break;
	};
}


void scanner::scan_directory_entry()
{
	if (!dir_iterator || !dir_iterator_playlist)
		return;

	if (dir_iterator->hasNext())
	{
		QString filename = dir_iterator->next();
		QFileInfo fileinfo(filename);
		if (fileinfo.isFile() && fileinfo.isReadable()) // filter out directories and unreadable files
			scan_file(*dir_iterator_playlist, filename);
	}
	else
	{
		dir_iterator_playlist = 0;
		dir_iterator = dir_iterator_ptr_t();
		scan_directory_timer.stop();
	}
}


QString scanner::check_if_starts_with_file(QString const &uri_str) const
{
	/*
	This function exists because sometimes, KDE file/director dialogs seem to return paths with file:// prepended.
	It does not seem to happen all the time, may be a bug inside KDE, and was not observed with the default Qt dialogs.
	(The KDE dialogs do get used even though this is not a KDE project - KDE replaces Qt's default file & director dialogs
	with its own.)
	*/
	if (uri_str.startsWith("file://"))
		return uri_str;
	else
		return QString("file://") + uri_str;
}


#else


scanner::scanner(QObject *parent, playlists_t &playlists_, QString const &backend_filepath):
	QObject(parent),
	base_t(playlists_),
	backend_filepath(backend_filepath)
{
	connect(&backend_process, SIGNAL(readyRead()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(started()), this, SLOT(started()));
	connect(&backend_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
	backend_process.setReadChannel(QProcess::StandardOutput);
	backend_process.setProcessChannelMode(QProcess::SeparateChannels);

	scan_directory_timer.setSingleShot(false);
	connect(&scan_directory_timer, SIGNAL(timeout()), this, SLOT(scan_directory_entry()));

	backend_process.start(backend_filepath);
	backend_process.waitForStarted(30000);
}


scanner::~scanner()
{
	scan_directory_timer.stop();
	dir_iterator_playlist = 0;
	dir_iterator = dir_iterator_ptr_t();

	scan_queue.clear();

	backend_process.write("quit\n");

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


void scanner::start_scan(playlist_t &playlist_, ion::uri const &uri_to_be_scanned)
{
	if (scan_queue.empty())
	{
		emit scan_running(true);
	}

	base_t::start_scan(playlist_, uri_to_be_scanned);
	emit queue_updated();
}


void scanner::scan_file(playlist_t &playlist_, QString const &filename)
{
	ion::uri uri_(check_if_starts_with_file(filename).toStdString());
	start_scan(playlist_, uri_);
}


void scanner::scan_directory(playlist_t &playlist_, QString const &directory_path)
{
	if (dir_iterator)
		return;

	dir_iterator_playlist = &playlist_;
	dir_iterator = dir_iterator_ptr_t(new QDirIterator(directory_path, QDirIterator::Subdirectories));
	scan_directory_timer.start(0);
}


bool scanner::is_already_scanning() const
{
	return (dir_iterator);
}


scanner::scan_queue_t const & scanner::get_scan_queue() const
{
	return scan_queue;
}


bool scanner::is_process_running() const
{
	//return (backend_process.state() != QProcess::NotRunning);
	return false;
}


void scanner::start_process(ion::uri const &uri_to_be_scanned)
{
	emit queue_updated();

	backend_process.write(std::string("get_metadata \"" + uri_to_be_scanned.get_full() + "\"\n").c_str());
	/*backend_process.start(backend_filepath, (QStringList() << "-info" << uri_to_be_scanned.get_full().c_str()), QIODevice::ReadOnly);
	backend_process.waitForStarted(30000);*/
}


void scanner::add_entry_to_playlist(ion::uri const &new_uri, ion::metadata_t const &new_metadata)
{
	add_entry(*current_playlist, create_entry(*current_playlist, new_uri, new_metadata), true);
//	scanning_process_finished(false);

/*	if (scan_queue.empty())
		emit scan_running(false);*/
}


void scanner::report_general_error(std::string const &error_string)
{
	emit general_scan_error(QString(error_string.c_str()));
}


void scanner::report_resource_error(std::string const &error_event, std::string const &uri)
{
	emit resource_scan_error(QString(error_event.c_str()), QString(uri.c_str()));
}


void scanner::cancel_scan_slot()
{
	cancel_scan();
	dir_iterator_playlist = 0;
	dir_iterator = dir_iterator_ptr_t();
	scan_directory_timer.stop();
	emit scan_canceled();
}


void scanner::try_read_stdout_line()
{
	while (backend_process.canReadLine())
	{
		QString line = backend_process.readLine().trimmed();
		read_process_stdin_line(line.toStdString());
	}
}


void scanner::started()
{
	scanning_process_started();
}


void scanner::finished(int exit_code, QProcess::ExitStatus exit_status)
{
	if (scan_queue.empty())
	{
		emit scan_running(false);
	}

	switch (exit_status)
	{
		case QProcess::NormalExit: scanning_process_finished(false); break;
		case QProcess::CrashExit: scanning_process_terminated(false); break;
		default: break;
	};
}


QString scanner::check_if_starts_with_file(QString const &uri_str) const
{
	/*
	This function exists because sometimes, KDE file/director dialogs seem to return paths with file:// prepended.
	It does not seem to happen all the time, may be a bug inside KDE, and was not observed with the default Qt dialogs.
	(The KDE dialogs do get used even though this is not a KDE project - KDE replaces Qt's default file & director dialogs
	with its own.)
	*/
	if (uri_str.startsWith("file://"))
		return uri_str;
	else
		return QString("file://") + uri_str;
}


void scanner::scan_directory_entry()
{
	if (!dir_iterator || !dir_iterator_playlist)
		return;

	if (dir_iterator->hasNext())
	{
		QString filename = dir_iterator->next();
		QFileInfo fileinfo(filename);
		if (fileinfo.isFile() && fileinfo.isReadable()) // filter out directories and unreadable files
			scan_file(*dir_iterator_playlist, filename);
	}
	else
	{
		dir_iterator_playlist = 0;
		dir_iterator = dir_iterator_ptr_t();
		scan_directory_timer.stop();
	}
}


#endif


}
}


#include "scanner.moc"

