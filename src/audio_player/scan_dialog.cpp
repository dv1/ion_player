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


#include <iostream>
#include <iterator>
#include <QHideEvent>
#include <QShowEvent>
#include "scan_dialog.hpp"


namespace ion
{
namespace audio_player
{


scan_queue_model::scan_queue_model(QObject *parent):
	QAbstractListModel(parent),
	scanner_(0)
{
}


int scan_queue_model::columnCount(QModelIndex const &) const
{
	return 1;
}


QVariant scan_queue_model::data(QModelIndex const & index, int role) const
{
	if ((scanner_ == 0) || (role != Qt::DisplayRole) || (index.column() >= columnCount(QModelIndex())) || (index.row() >= rowCount(QModelIndex())))
		return QVariant();

	scanner::queue_sequence_t const &queue_sequence = scanner_->get_queue().get < scanner::sequence_tag > ();
	scanner::queue_sequence_t::const_iterator seq_iter = queue_sequence.begin();
	std::advance(seq_iter, index.row());

	return QString(seq_iter->uri_.get_full().c_str());
}


QModelIndex scan_queue_model::parent(QModelIndex const & index) const
{
	return QModelIndex();
}


int scan_queue_model::rowCount(QModelIndex const &) const
{
	if (scanner_ == 0)
		return 0;
	else
		return scanner_->get_queue().size();
}


void scan_queue_model::set_scanner(scanner *new_scanner)
{
	scanner_ = new_scanner;
	reset();
}


void scan_queue_model::reload_entries()
{
	reset();
}


void scan_queue_model::queue_entry_being_added(QString const &, scanner::playlist_t *, bool const before)
{
	if (scanner_ == 0)
		return;

	if (before)
		beginInsertRows(QModelIndex(), scanner_->get_queue().size(), scanner_->get_queue().size());
	else
		endInsertRows();
}


void scan_queue_model::queue_entry_being_removed(QString const &uri_string, scanner::playlist_t *, bool const before)
{
	if (scanner_ == 0)
		return;

	ion::uri uri_(uri_string.toStdString());

	scanner::queue_by_uri_t const &queue_by_uri = scanner_->get_queue().get < scanner::uri_tag > ();
	scanner::queue_by_uri_t::const_iterator resource_by_uri_iter = queue_by_uri.find(uri_);
	scanner::queue_sequence_t::const_iterator seq_iter = scanner_->get_queue().project < scanner::sequence_tag > (resource_by_uri_iter);
	std::size_t index = std::distance(scanner_->get_queue().get < scanner::sequence_tag > ().begin(), seq_iter);

	if (before)
		beginRemoveRows(QModelIndex(), index, index);
	else
		endRemoveRows();
}





scan_dialog::scan_dialog(QWidget *parent, scanner *scanner_):
	QDialog(parent),
	frozen(true),
	scanner_(0)
{
	scan_dialog_ui.setupUi(this);

	scan_queue_model_ = new scan_queue_model(this);
	scan_dialog_ui.error_log_view->initialize(1000);
	scan_dialog_ui.scan_queue_view->setModel(scan_queue_model_);
	connect(scan_dialog_ui.clear_log_button, SIGNAL(clicked()), scan_dialog_ui.error_log_view, SLOT(clear()));

	set_scanner(scanner_);
}


scan_dialog::~scan_dialog()
{
}


void scan_dialog::set_scanner(scanner *new_scanner)
{
	if (scanner_ != 0)
	{
		disconnect(scanner_, SIGNAL(scan_running(bool)), this, SLOT(scan_running(bool)));
		disconnect(scanner_, SIGNAL(scan_canceled()), scan_queue_model_, SLOT(reload_entries()));
		disconnect(scanner_, SIGNAL(queue_entry_being_added(QString const &, scanner::playlist_t *, bool const)), scan_queue_model_, SLOT(queue_entry_being_added(QString const &, scanner::playlist_t *, bool const)));
		disconnect(scanner_, SIGNAL(queue_entry_being_removed(QString const &, scanner::playlist_t *, bool const)), scan_queue_model_, SLOT(queue_entry_being_removed(QString const &, scanner::playlist_t *, bool const)));
		disconnect(scanner_, SIGNAL(general_scan_error(QString const &)), this, SLOT(general_scan_error(QString const &)));
		disconnect(scanner_, SIGNAL(resource_scan_error(QString const &, QString const &)), this, SLOT(resource_scan_error(QString const &, QString const &)));
		disconnect(scan_dialog_ui.cancel_scan_button, SIGNAL(clicked()), scanner_, SLOT(cancel_scan_slot()));
	}

	scanner_ = new_scanner;

	if (scanner_ != 0)
	{
		connect(scanner_, SIGNAL(scan_running(bool)), this, SLOT(scan_running(bool)));
		connect(scanner_, SIGNAL(scan_canceled()), scan_queue_model_, SLOT(reload_entries()));
		connect(scanner_, SIGNAL(queue_entry_being_added(QString const &, scanner::playlist_t *, bool const)), scan_queue_model_, SLOT(queue_entry_being_added(QString const &, scanner::playlist_t *, bool const)));
		connect(scanner_, SIGNAL(queue_entry_being_removed(QString const &, scanner::playlist_t *, bool const)), scan_queue_model_, SLOT(queue_entry_being_removed(QString const &, scanner::playlist_t *, bool const)));
		connect(scanner_, SIGNAL(general_scan_error(QString const &)), this, SLOT(general_scan_error(QString const &)));
		connect(scanner_, SIGNAL(resource_scan_error(QString const &, QString const &)), this, SLOT(resource_scan_error(QString const &, QString const &)));
		connect(scan_dialog_ui.cancel_scan_button, SIGNAL(clicked()), scanner_, SLOT(cancel_scan_slot()));
	}

	scan_queue_model_->set_scanner(scanner_);
}


void scan_dialog::closeEvent(QCloseEvent *event)
{
	frozen = true;
	QDialog::closeEvent(event);
}


void scan_dialog::hideEvent(QHideEvent *event)
{
	frozen = true;
	QDialog::hideEvent(event);
}


void scan_dialog::showEvent(QShowEvent *event)
{
	frozen = false;
	QDialog::showEvent(event);
}



void scan_dialog::scan_running(bool const state)
{
	if (!state)
		scan_queue_model_->reload_entries();
}


void scan_dialog::general_scan_error(QString const &error_string)
{
	scan_dialog_ui.error_log_view->add_line("scan backend", "general", error_string);
}


void scan_dialog::resource_scan_error(QString const &error_type, QString const &uri_string)
{
	scan_dialog_ui.error_log_view->add_line("scan backend", error_type, uri_string);
}


}
}


#include "scan_dialog.moc"

