#ifndef TESTCLASS_HPP_______
#define TESTCLASS_HPP_______

#include <QProcess>
#include <QTimer>

#include <ion/scanner_base.hpp>
#include <ion/playlists.hpp>
#include <ion/playlist.hpp>


class test_scanner:
        public QObject,
	public ion::scanner_base < test_scanner, ion::playlists < ion::playlist > >
{
        Q_OBJECT
public:
	typedef ion::scanner_base < test_scanner, ion::playlists < ion::playlist > > base_t;


	explicit test_scanner(playlists_t &playlists_, int const max_num_entries_added);
	~test_scanner();


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


protected slots:
	void try_read_stdout_line();
	void started();
	void finished(int exit_code, QProcess::ExitStatus exit_status);
	void print_tick();
	void watchdog_timeout();
	void kill_timeout();


protected:
	void start_backend();
	void terminate_backend(bool const start_kill_timer = true);


	QProcess backend_process;
	QTimer tick_timer, watchdog_timer, kill_timer;
	int num_entries_added;
};


#endif

