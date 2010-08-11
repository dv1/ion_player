#include <iostream>
#include <json/reader.h>
#include <ion/command_line_tools.hpp>
#include "scanner.hpp"


namespace ion
{
namespace frontend
{


scanner::scanner(QObject *parent, QString const &backend_filepath):
	QObject(parent),
	backend_filepath(backend_filepath),
	current_playlist(0)
{
	scan_process = new QProcess(this);
	connect(scan_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(process_finished(int, QProcess::ExitStatus)));
	connect(scan_process, SIGNAL(readyRead()), this, SLOT(try_read_metadata()));
}


scanner::~scanner()
{
	if (scan_process->state() != QProcess::NotRunning)
		scan_process->waitForFinished();
}


void scanner::start_scan(ion::simple_playlist &playlist, ion::uri const &uri_to_be_scanned)
{
	scan_queue.push_back(scan_entry_t(&playlist, uri_to_be_scanned));
	if (scan_process->state() != QProcess::Running)
		start_process();
}


void scanner::try_read_metadata()
{
	if (current_playlist == 0)
		return;

	while (scan_process->canReadLine())
	{
		QString line = scan_process->readLine().trimmed();
		std::cerr << "scan backend stdout> " << line.toStdString() << std::endl;

		std::string event_command_name;
		params_t event_params;
		split_command_line(line.toStdString(), event_command_name, event_params);

		if ((event_command_name == "metadata") && (event_params.size() >= 2))
		{
			try
			{
				ion::uri uri_(event_params[0]);

				metadata_t metadata_;
				Json::Reader().parse(event_params[1], metadata_);

				if (!metadata_.isObject()) // TODO: remove the jsoncpp specific calls, make this more generic, like is_valid(metadata_)
				{
					metadata_ = metadata_t(Json::objectValue);
					std::cerr << "Metadata for URI \"" << uri_.get_full() << "\" is invalid!" << std::endl;
				}

				// TODO: improve the "playlist_t::entry" part; maybe something like playlist_traits < playlist_t > ::entry, or just get rid of the explicit entry type entirely
				// add_entry(current_playlist, uri_, metadata_) sounds better
				current_playlist->add_entry(simple_playlist::entry(uri_, metadata_));
			}
			catch (ion::uri::invalid_uri const &invalid_uri_)
			{
				std::cerr << "Caught invalid URI \"" << invalid_uri_.what() << '"' << std::endl;
			}
		}
	}
}


void scanner::process_finished(int exit_code, QProcess::ExitStatus exit_status)
{
	current_playlist = 0;
	start_process();
}


void scanner::start_process()
{
	if (scan_queue.empty())
		return;

	scan_entry_t scan_entry = scan_queue.front();
	current_playlist = scan_entry.first;
	scan_queue.pop_front();
	scan_process->start(backend_filepath, (QStringList() << "-info" << scan_entry.second.get_full().c_str()), QIODevice::ReadOnly);
	scan_process->waitForStarted(30000);
}


}
}


#include "scanner.moc"

