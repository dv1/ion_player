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


#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include "scanner.hpp"


namespace ion
{
namespace audio_player
{


scanner::scanner(QObject *parent, playlists_t &playlists_, QString const &backend_filepath):
	QObject(parent),
	base_t(playlists_),
	backend_filepath(backend_filepath),
	terminate_sent(false)
{
	playlist_removed_connection = playlists_.get_playlist_removed_signal().connect(boost::phoenix::bind(&scanner::playlist_removed, this, boost::phoenix::arg_names::arg1));

	connect(&watchdog_timer, SIGNAL(timeout()), this, SLOT(watchdog_timeout()));
	watchdog_timer.setSingleShot(false);

	connect(&backend_process, SIGNAL(readyRead()), this, SLOT(try_read_stdout_line()));
	connect(&backend_process, SIGNAL(started()), this, SLOT(started()));
	connect(&backend_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
	backend_process.setReadChannel(QProcess::StandardOutput);
	backend_process.setProcessChannelMode(QProcess::SeparateChannels);

	scan_directory_timer.setSingleShot(false);
	connect(&scan_directory_timer, SIGNAL(timeout()), this, SLOT(scan_directory_entry()));

	start_backend();
}


scanner::~scanner()
{
	playlist_removed_connection.disconnect();

	scan_directory_timer.stop();
	dir_iterator_playlist = 0;
	dir_iterator = dir_iterator_ptr_t();

	queue.clear();

	backend_process.write("quit\n");

	backend_process.waitForFinished(30000);
	if (backend_process.state() != QProcess::NotRunning)
		terminate_backend(true);
}


void scanner::scan_file(playlist_t &playlist_, QString const &filename)
{
	// TODO: converting to utf8 here, which assumes that this is OK enough for file access
	// however, this may not be the case - the file system may use some other encoding
	// -> encode to a file system specific encoding
	QByteArray filename_array = check_if_starts_with_file(filename).toUtf8();
	std::string filename_str(filename_array.constData(), filename_array.length());
	ion::uri uri_(filename_str);
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


bool scanner::is_scanning_directory() const
{
	return (dir_iterator);
}


void scanner::send_to_backend(std::string const &command_line)
{
	backend_process.write((command_line + "\n").c_str());
}


void scanner::restart_watchdog_timer()
{
	watchdog_timer.start(30000);
}


void scanner::stop_watchdog_timer()
{
	watchdog_timer.stop();
}


void scanner::restart_backend()
{
	start_backend();
}


void scanner::adding_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before)
{
	emit queue_entry_being_added(QString(uri_.get_full().c_str()), &playlist_, before);
}


void scanner::removing_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before)
{
	emit queue_entry_being_removed(QString(uri_.get_full().c_str()), &playlist_, before);
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
	terminate_sent = false;

	switch (exit_status)
	{
		case QProcess::CrashExit: backend_crashed(); break;
		default: break;
	};
}


void scanner::playlist_removed(playlist_t &playlist_)
{
	if (&playlist_ == dir_iterator_playlist)
	{
		dir_iterator_playlist = 0;
		dir_iterator = dir_iterator_ptr_t();
		scan_directory_timer.stop();
	}
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


void scanner::start_backend()
{
	backend_process.start(backend_filepath, (QStringList() << "-scan"), QIODevice::ReadWrite);
	backend_process.waitForStarted(30000);
}


void scanner::terminate_backend(bool const do_wait)
{
	if (backend_process.state() != QProcess::NotRunning)
	{
		if (terminate_sent)
		{
			std::cerr << "sending scan backend the KILL signal" << std::endl;
			backend_process.kill();
		}
		else
		{
			backend_process.terminate();
			std::cerr << "sending scan backend the TERM signal" << std::endl;
		}
	}

	if (do_wait)
	{
		backend_process.waitForFinished(20000);
		if (backend_process.state() != QProcess::NotRunning)
			backend_process.kill();
	}
	else
		terminate_sent = true;
}


void scanner::watchdog_timeout()
{
	terminate_backend();
}


}
}


#include "scanner.moc"

