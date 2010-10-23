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


#ifndef ION_AUDIO_PLAYER_SETTINGS_DIALOG_HPP
#define ION_AUDIO_PLAYER_SETTINGS_DIALOG_HPP

#include <QAbstractListModel>
#include <QDialog>
#include <QVariant>
#include <boost/function.hpp>
#include <boost/signals2/connection.hpp>
#include "ui_settings.h"
#include "misc_types.hpp"


namespace ion
{


namespace audio_common
{
class audio_frontend;
}


namespace audio_player
{


class settings;
class configuration_html_page_controller;


class module_entries_model:
	public QAbstractListModel
{
public:
	explicit module_entries_model(QObject *parent, audio_common::audio_frontend &audio_frontend_);
	~module_entries_model();

	virtual int columnCount(QModelIndex const & parent) const;
	virtual QVariant data(QModelIndex const & index, int role) const;
	virtual QModelIndex parent(QModelIndex const & index) const;
	virtual int rowCount(QModelIndex const & parent) const;

protected:
	audio_common::audio_frontend &audio_frontend_;
	boost::signals2::connection module_entries_updated_signal_connection;
};


class settings_dialog:
	public QDialog 
{
	Q_OBJECT
public:
	typedef boost::function < void() > change_backend_callback_t;
	typedef boost::function < void() > settings_accepted_callback_t;
	typedef boost::function < void() > show_log_callback_t;

	explicit settings_dialog(
		QWidget *parent, settings &settings_,
		audio_common::audio_frontend &audio_frontend_,
		playlists_t const &playlists_,
		change_backend_callback_t const &change_backend_callback,
		settings_accepted_callback_t const &settings_accepted_callback,
		show_log_callback_t const &show_log_callback
		);
	~settings_dialog();


protected slots:
	void open_backend_filepath_filedialog();
	void show_log();
	void selected_module_changed(QModelIndex const &new_selection);
	void new_settings_accepted();


protected:
	void showEvent(QShowEvent *event);
	void set_module_ui(audio_common::audio_frontend::module_entry const &module_entry_);
	Json::Value get_properties();
	void set_properties(Json::Value const &properties);


	audio_common::audio_frontend::module_entry const *current_module_entry;
	settings &settings_;
	audio_common::audio_frontend &audio_frontend_;
	playlists_t const &playlists_;
	change_backend_callback_t change_backend_callback;
	settings_accepted_callback_t settings_accepted_callback;
	show_log_callback_t show_log_callback;
	module_entries_model *module_entries_model_;
	Ui_settings_dialog settings_dialog_ui;
	configuration_html_page_controller *configuration_html_page_controller_;
};


}
}


#endif

