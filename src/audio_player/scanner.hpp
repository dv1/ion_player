#ifndef ION_AUDIO_PLAYER_SCANNER_HPP
#define ION_AUDIO_PLAYER_SCANNER_HPP

#include <QProcess>
#include <ion/scanner_base.hpp>
#include <ion/playlist.hpp>


namespace ion
{
namespace audio_player
{


class scanner:
        public QObject,
	public scanner_base < scanner, playlist >
{
        Q_OBJECT
public:
	typedef scanner_base < scanner, playlist > base_t;
	typedef base_t::scan_queue_t scan_queue_t;

	explicit scanner(QObject *parent, QString const &backend_filepath);
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

