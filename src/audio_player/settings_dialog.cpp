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


#include <assert.h>
#include <QPushButton>
#include <QFileDialog>
#include <QWebFrame>
#include <QWebSettings>
#include <boost/foreach.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <json/writer.h>
#include <ion/playlists.hpp>
#include "audio_frontend.hpp"
#include "settings.hpp"
#include "settings_dialog.hpp"
#include "configuration_html_page_controller.hpp"


namespace ion
{
namespace audio_player
{


module_entries_model::module_entries_model(QObject *parent, audio_common::audio_frontend &audio_frontend_):
	QAbstractListModel(parent),
	audio_frontend_(audio_frontend_)
{
	module_entries_updated_signal_connection = audio_frontend_.get_module_entries_updated_signal().connect(boost::phoenix::bind(&module_entries_model::reset, this));
}


module_entries_model::~module_entries_model()
{
	module_entries_updated_signal_connection.disconnect();
}


int module_entries_model::columnCount(QModelIndex const &) const
{
	return 2;
}


QVariant module_entries_model::data(QModelIndex const & index, int role) const
{
	if ((role != Qt::DisplayRole) || (index.column() > 1) || (index.row() >= rowCount(QModelIndex())))
		return QVariant();

	audio_common::audio_frontend::module_entry_sequence_t const &module_entry_sequence = audio_frontend_.get_module_entries().get < audio_common::audio_frontend::sequence_tag > ();
	audio_common::audio_frontend::module_entry_sequence_t::const_iterator iter = module_entry_sequence.begin() + index.row();

	if (iter == module_entry_sequence.end())
		return QVariant();
	else
	{
		switch (index.column())
		{
			case 0: return QString(iter->id.c_str());
			case 1: return QString(iter->type.c_str());
			default: return QVariant();
		}
	}
}


QModelIndex module_entries_model::parent(QModelIndex const &) const
{
	return QModelIndex();
}


int module_entries_model::rowCount(QModelIndex const & parent) const
{
	return audio_frontend_.get_module_entries().size();
}



settings_dialog::settings_dialog(
	QWidget *parent, settings &settings_,
	audio_common::audio_frontend &audio_frontend_,
	playlists_t const &playlists_,
	change_backend_callback_t const &change_backend_callback,
	settings_accepted_callback_t const &settings_accepted_callback,
	show_log_callback_t const &show_log_callback
):
	QDialog(parent),
	current_module_entry(0),
	settings_(settings_),
	audio_frontend_(audio_frontend_),
	playlists_(playlists_),
	change_backend_callback(change_backend_callback),
	settings_accepted_callback(settings_accepted_callback),
	show_log_callback(show_log_callback)
{
	assert(change_backend_callback);
	settings_dialog_ui.setupUi(this);
	module_entries_model_ = new module_entries_model(this, audio_frontend_);
	settings_dialog_ui.modules_list_view->setModel(module_entries_model_);

	configuration_html_page_controller_ = new configuration_html_page_controller(
		this,
		settings_dialog_ui.module_gui_view->page(),
		boost::phoenix::bind(&settings_dialog::get_properties, this),
		boost::phoenix::bind(&settings_dialog::set_properties, this, boost::phoenix::arg_names::arg1),
		"uiProperties"
	);

	// TODO: the web inspector can't store its settings; perhaps because a QSettings instance is around already?
	QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

	connect(settings_dialog_ui.buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(new_settings_accepted()));
	connect(settings_dialog_ui.buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), configuration_html_page_controller_, SLOT(apply_configuration()));
	connect(settings_dialog_ui.open_backend_filedialog, SIGNAL(clicked()), this, SLOT(open_backend_filepath_filedialog()));
	connect(settings_dialog_ui.show_log, SIGNAL(clicked()), this, SLOT(show_log()));
	connect(settings_dialog_ui.modules_list_view->selectionModel(), SIGNAL(currentChanged(QModelIndex const &, QModelIndex const &)), this, SLOT(selected_module_changed(QModelIndex const &)));
	//connect(settings_dialog_ui.module_gui_view->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populate_javascript()));
	connect(this, SIGNAL(accepted()), this, SLOT(new_settings_accepted()));
	connect(this, SIGNAL(accepted()), configuration_html_page_controller_, SLOT(apply_configuration()));
}


settings_dialog::~settings_dialog()
{
	delete configuration_html_page_controller_;
}


void settings_dialog::open_backend_filepath_filedialog()
{
	QString backend_filepath = QFileDialog::getOpenFileName(this, "Select backend", settings_dialog_ui.backend_filepath->text());
	if (!backend_filepath.isNull() && !backend_filepath.isEmpty())
		settings_dialog_ui.backend_filepath->setText(backend_filepath);
}


void settings_dialog::show_log()
{
	if (show_log_callback)
		show_log_callback();
}


void settings_dialog::selected_module_changed(QModelIndex const &new_selection)
{
	if (!new_selection.isValid())
	{
		settings_dialog_ui.module_gui_view->setUrl(QUrl("about:blank"));
		return;
	}


	audio_common::audio_frontend::module_entry_sequence_t const &module_entry_sequence = audio_frontend_.get_module_entries().get < audio_common::audio_frontend::sequence_tag > ();
	audio_common::audio_frontend::module_entry_sequence_t::const_iterator iter = module_entry_sequence.begin() + new_selection.row();

	if (iter == module_entry_sequence.end())
	{
		settings_dialog_ui.module_gui_view->setUrl(QUrl("about:blank"));
		return;
	}
	
	set_module_ui(*iter);
}


Json::Value settings_dialog::get_properties()
{
	if (current_module_entry == 0)
		return Json::Value();
	else
		return current_module_entry->ui_properties;
}


void settings_dialog::set_properties(Json::Value const &properties)
{
	if (current_module_entry != 0)
		audio_frontend_.update_module_properties(current_module_entry->id, properties);
}


void settings_dialog::new_settings_accepted()
{
	// update settings
	settings_.set_always_on_top_flag(settings_dialog_ui.always_on_top->checkState() == Qt::Checked);
	settings_.set_on_all_workspaces_flag(settings_dialog_ui.on_all_workspaces->checkState() == Qt::Checked);
	settings_.set_systray_icon_flag(settings_dialog_ui.notification_area_icon->checkState() == Qt::Checked);

	// singleplay playlist
	if (!settings_dialog_ui.singleplay_playlist->currentText().isNull() || !settings_dialog_ui.singleplay_playlist->currentText().isEmpty())
		settings_.set_singleplay_playlist(settings_dialog_ui.singleplay_playlist->currentText());

	// backend filepath
	if (settings_dialog_ui.backend_filepath->text() != settings_.get_backend_filepath())
	{
		settings_.set_backend_filepath(settings_dialog_ui.backend_filepath->text());
		change_backend_callback();
	}

	if (settings_accepted_callback)
		settings_accepted_callback();
}


void settings_dialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);

	// transfer settings from settings structure to dialog GUI
	settings_dialog_ui.always_on_top->setCheckState(settings_.get_always_on_top_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.on_all_workspaces->setCheckState(settings_.get_on_all_workspaces_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.notification_area_icon->setCheckState(settings_.get_systray_icon_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.backend_filepath->setText(settings_.get_backend_filepath());

	// in case of the singleplay playlist, fill the combobox with the playlist names
	settings_dialog_ui.singleplay_playlist->clear();
	BOOST_FOREACH(playlists_t::playlist_ptr_t const &playlist_, get_playlists(playlists_))
	{
		settings_dialog_ui.singleplay_playlist->addItem(playlist_->get_name().c_str());
	}
	settings_dialog_ui.singleplay_playlist->setEditText(settings_.get_singleplay_playlist()); // and set the current singleplay playlist name
}


void settings_dialog::set_module_ui(audio_common::audio_frontend::module_entry const &module_entry_)
{
	current_module_entry = &module_entry_;
	settings_dialog_ui.module_gui_view->setHtml(module_entry_.html_code.c_str());
}


}
}


#include "settings_dialog.moc"

