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
#include <ion/scanner_base.hpp>
#include "misc_types.hpp"


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
	typedef base_t::scan_queue_t scan_queue_t;

	explicit scanner(QObject *parent, playlists_t &playlists_, QString const &backend_filepath);
	~scanner();


	void start_scan(playlist_t &playlist_, ion::uri const &uri_to_be_scanned);

	scan_queue_t const & get_scan_queue() const;


	// These three functions are public only because the CRTP requires it (see scanner_base)
	// TODO: use the protected-fail technique to get them back to protected access
	bool is_process_running() const;
	void start_process(ion::uri const &uri_to_be_scanned);
	void add_entry_to_playlist(ion::uri const &new_uri, ion::metadata_t const &new_metadata);
	void report_general_error(std::string const &error_string);
	void report_resource_error(std::string const &error_event, std::string const &uri);


signals:
	void queue_updated();
	void scan_running(bool);
	void scan_canceled();
	void general_scan_error(QString const &error_string);
	void resource_scan_error(QString const &error_type, QString const &uri_string);


public slots:
	void cancel_scan_slot();


protected slots:
	void try_read_stdout_line();
	void started();
	void finished(int exit_code, QProcess::ExitStatus exit_status);


protected:
	QProcess backend_process;
	QString backend_filepath;
};


}
}


#endif

