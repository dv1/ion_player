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


#include <QHideEvent>
#include <QShowEvent>
#include "scan_dialog.hpp"


namespace ion
{
namespace audio_player
{


scan_queue_model::scan_queue_model(QObject *parent, scanner::scan_queue_t const *scan_queue_):
	QAbstractListModel(parent),
	scan_queue(0)
{
	set_scan_queue(scan_queue_);
}


void scan_queue_model::set_scan_queue(scanner::scan_queue_t const *new_scan_queue)
{
	scan_queue = new_scan_queue;
	reset();
}


void scan_queue_model::reset()
{
	QAbstractListModel::reset();
}


int scan_queue_model::columnCount(QModelIndex const &) const
{
	return 1;
}


QVariant scan_queue_model::data(QModelIndex const & index, int role) const
{
	if ((scan_queue == 0) || (role != Qt::DisplayRole) || (index.column() >= columnCount(QModelIndex())) || (index.row() >= rowCount(QModelIndex())))
		return QVariant();

	return QString((*scan_queue)[index.row()].second.get_full().c_str());
}


QModelIndex scan_queue_model::parent(QModelIndex const & index) const
{
	return QModelIndex();
}


int scan_queue_model::rowCount(QModelIndex const &) const
{
	return (scan_queue != 0) ? scan_queue->size() : 0;
}





scan_dialog::scan_dialog(QWidget *parent, scanner *scanner_):
	QDialog(parent),
	frozen(true),
	scanner_(0)
{
	scan_dialog_ui.setupUi(this);

	scan_queue_model_ = new scan_queue_model(this, (scanner_ != 0) ? &(scanner_->get_scan_queue()) : 0);
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
		disconnect(scanner_, SIGNAL(queue_updated()), this, SLOT(scan_queue_updated()));
		disconnect(scanner_, SIGNAL(general_scan_error(QString const &)), this, SLOT(general_scan_error(QString const &)));
		disconnect(scanner_, SIGNAL(resource_scan_error(QString const &, QString const &)), this, SLOT(resource_scan_error(QString const &, QString const &)));
		disconnect(scan_dialog_ui.cancel_scan_button, SIGNAL(clicked()), scanner_, SLOT(cancel_scan_slot()));
	}

	scanner_ = new_scanner;

	if (scanner_ != 0)
	{
		connect(scanner_, SIGNAL(queue_updated()), this, SLOT(scan_queue_updated()));
		connect(scanner_, SIGNAL(general_scan_error(QString const &)), this, SLOT(general_scan_error(QString const &)));
		connect(scanner_, SIGNAL(resource_scan_error(QString const &, QString const &)), this, SLOT(resource_scan_error(QString const &, QString const &)));
		connect(scan_dialog_ui.cancel_scan_button, SIGNAL(clicked()), scanner_, SLOT(cancel_scan_slot()));
	}

	scan_queue_model_->set_scan_queue((scanner_ != 0) ? &(scanner_->get_scan_queue()) : 0);

	scan_queue_updated();
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
	scan_queue_updated();
	QDialog::showEvent(event);
}


void scan_dialog::scan_queue_updated()
{
	if (!frozen)
		scan_queue_model_->reset();
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

