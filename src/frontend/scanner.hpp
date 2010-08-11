#ifndef ION_FRONTEND_SCANNER_HPP
#define ION_FRONTEND_SCANNER_HPP

#include <QProcess>
#include <deque>
#include <ion/metadata.hpp>
#include <ion/uri.hpp>
#include "settings.hpp"
#include <ion/simple_playlist.hpp>


namespace ion
{
namespace frontend
{


// TODO: this is simple_playlist specific; remove that dependency
class scanner:
	public QObject
{
	Q_OBJECT
public:
	scanner(QObject *parent, QString const &backend_filepath);
	~scanner();

	void start_scan(ion::simple_playlist &playlist, ion::uri const &uri_to_be_scanned);
	QProcess * get_scan_process() { return scan_process; }


protected slots:
	void try_read_metadata();
	void process_finished(int exit_code, QProcess::ExitStatus exit_status);


protected:
	void start_process();


	typedef std::pair < ion::simple_playlist *, ion::uri > scan_entry_t;
	typedef std::deque < scan_entry_t > scan_queue_t;
	scan_queue_t scan_queue;
	QProcess *scan_process;
	QString backend_filepath;
	ion::simple_playlist *current_playlist;
};


}
}


#endif

