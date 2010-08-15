#ifndef TESTCLASS_HPP_______
#define TESTCLASS_HPP_______

#include <QProcess>
#include <ion/scanner.hpp>


class test_scanner:
        public QObject,
	public ion::scanner < test_scanner, int >
{
        Q_OBJECT
public:
	explicit test_scanner(int const max_num_entries_added);
	~test_scanner();


	bool is_process_running() const;
	void start_process(ion::uri const &uri_to_be_scanned);
	void add_entry_to_playlist(ion::uri const &new_uri, ion::metadata_t const &new_metadata);


protected slots:
	void try_read_stdout_line();
	void started();
	void finished(int exit_code, QProcess::ExitStatus exit_status);


protected:
	QProcess backend_process;
	int num_entries_added;
};


#endif

