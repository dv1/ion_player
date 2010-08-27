#ifndef ION_FRONTEND_SCANNER_HPP
#define ION_FRONTEND_SCANNER_HPP

#include <QProcess>
#include <ion/scanner_base.hpp>
#include <ion/flat_playlist.hpp>


namespace ion
{
namespace frontend
{


class scanner:
        public QObject,
	public scanner_base < scanner, flat_playlist >
{
        Q_OBJECT
public:
	explicit scanner(QObject *parent, QString const &backend_filepath);
	~scanner();


	bool is_process_running() const;
	void start_process(ion::uri const &uri_to_be_scanned);
	void add_entry_to_playlist(ion::uri const &new_uri, ion::metadata_t const &new_metadata);


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

