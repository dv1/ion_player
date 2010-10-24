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


#ifndef ION_AUDIO_PLAYER_SCANNER_HPP
#define ION_AUDIO_PLAYER_SCANNER_HPP

#include <QProcess>
#include <QDirIterator>
#include <QTimer>
#include <boost/shared_ptr.hpp>
#include <ion/scanner_base.hpp>
#include "misc_types.hpp"


class QTimer;


namespace ion
{
namespace audio_player
{


class scanner:
        public QObject,
	public scanner_base < scanner, ion::audio_player::playlists_t >
{
        Q_OBJECT
public:
	typedef scanner_base < scanner, ion::audio_player::playlists_t > base_t;
	typedef base_t::queue_t queue_t;

	explicit scanner(QObject *parent, playlists_t &playlists_, QString const &backend_filepath);
	~scanner();

	void scan_file(playlist_t &playlist_, QString const &filename);
	void scan_directory(playlist_t &playlist_, QString const &directory_path);


	// These functions are public only because the CRTP requires it (see scanner_base)
	// TODO: use the protected-fail technique to get them back to protected access
	void send_to_backend(std::string const &command_line);
	void restart_watchdog_timer();
	void stop_watchdog_timer();
	void restart_backend();
	void adding_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before);
	void removing_queue_entry(ion::uri const &uri_, playlist_t &playlist_, bool const before);
	void scanning_in_progress(bool const state);
	void resource_successfully_scanned(ion::uri const &uri_, playlist_t &playlist_, ion::metadata_t const &new_metadata);
	void unrecognized_resource(ion::uri const &uri_, playlist_t &playlist_);
	void resource_corrupted(ion::uri const &uri_, playlist_t &playlist_);
	void scanning_failed(ion::uri const &uri_, playlist_t &playlist_);


signals:
	void scan_canceled();
	void resource_scan_error(QString const &error_type, QString const &uri_string);
	void scan_running(bool);

	// these may get replaced later
	void general_scan_error(QString const &error_string);
	void queue_updated();


public slots:
	void cancel_scan_slot();


protected slots:
	void try_read_stdout_line();
	void started();
	void finished(int exit_code, QProcess::ExitStatus exit_status);
	void scan_directory_entry();


protected:
	QString check_if_starts_with_file(QString const &uri_str) const;


	QProcess backend_process;
	QString backend_filepath;

	QTimer scan_directory_timer;
	typedef boost::shared_ptr < QDirIterator > dir_iterator_ptr_t;
	dir_iterator_ptr_t dir_iterator;
	playlist *dir_iterator_playlist;
};


}
}


#endif

