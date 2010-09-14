#include <assert.h>
#include <QFileDialog>
#include <boost/foreach.hpp>
#include <ion/playlists.hpp>
#include "audio_frontend.hpp"
#include "settings.hpp"
#include "settings_dialog.hpp"


namespace ion
{
namespace audio_player
{


module_entries_model::module_entries_model(QObject *parent, audio_frontend &audio_frontend_):
	QAbstractListModel(parent),
	audio_frontend_(audio_frontend_)
{
}


int module_entries_model::columnCount(QModelIndex const &) const
{
	return 2;
}


QVariant module_entries_model::data(QModelIndex const & index, int role) const
{
	if ((role != Qt::DisplayRole) || (index.column() > 1) || (index.row() >= rowCount(QModelIndex())))
		return QVariant();

	audio_frontend::module_entry_sequence_t const &module_entry_sequence = audio_frontend_.get_module_entries().get < audio_frontend::sequence_tag > ();
	audio_frontend::module_entry_sequence_t::const_iterator iter = module_entry_sequence.begin() + index.row();

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


void module_entries_model::reset_module_list()
{
	reset();
}



settings_dialog::settings_dialog(QWidget *parent, settings &settings_, audio_frontend &audio_frontend_, playlists_t const &playlists_, change_backend_callback_t const &change_backend_callback):
	QDialog(parent),
	settings_(settings_),
	audio_frontend_(audio_frontend_),
	playlists_(playlists_),
	change_backend_callback(change_backend_callback)
{
	assert(change_backend_callback);
	settings_dialog_ui.setupUi(this);
	module_entries_model_ = new module_entries_model(this, audio_frontend_);
	settings_dialog_ui.modules_list_view->setModel(module_entries_model_);

	connect(settings_dialog_ui.backend_filedialog, SIGNAL(clicked()), this, SLOT(open_backend_filepath_filedialog())); // TODO: better naming of the button
	connect(settings_dialog_ui.modules_list_view->selectionModel(), SIGNAL(currentChanged(QModelIndex const &, QModelIndex const &)), this, SLOT(selected_module_changed(QModelIndex const &)));

}


void settings_dialog::update_module_ui()
{
	// TODO: it would be better for audio_frontend to emit a signal when the module list has been updated (both because of modules and module_ui events)
	module_entries_model_->reset_module_list();
}


int settings_dialog::run_dialog()
{
	// transfer settings from settings structure to dialog GUI
	settings_dialog_ui.always_on_top->setCheckState(settings_.get_always_on_top_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.on_all_workspaces->setCheckState(settings_.get_on_all_workspaces_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.notification_area_icon->setCheckState(settings_.get_systray_icon_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.show_backend_log_dialog->setCheckState(settings_.get_backend_log_dialog_shown_flag() ? Qt::Checked : Qt::Unchecked);
	settings_dialog_ui.backend_filepath->setText(settings_.get_backend_filepath());

	// in case of the singleplay playlist, fill the combobox with the playlist names
	settings_dialog_ui.singleplay_playlist->clear();
	BOOST_FOREACH(playlists_t::playlist_ptr_t const &playlist_, get_playlists(playlists_))
	{
		settings_dialog_ui.singleplay_playlist->addItem(playlist_->get_name().c_str());
	}
	settings_dialog_ui.singleplay_playlist->setEditText(settings_.get_singleplay_playlist()); // and set the current singleplay playlist namie

	// if user rejected the dialog, stop processing
	if (exec() == QDialog::Rejected)
		return QDialog::Rejected;


	// update settings
	settings_.set_always_on_top_flag(settings_dialog_ui.always_on_top->checkState() == Qt::Checked);
	settings_.set_on_all_workspaces_flag(settings_dialog_ui.on_all_workspaces->checkState() == Qt::Checked);
	settings_.set_systray_icon_flag(settings_dialog_ui.notification_area_icon->checkState() == Qt::Checked);
	settings_.set_backend_log_dialog_shown_flag(settings_dialog_ui.show_backend_log_dialog->checkState() == Qt::Checked);

	// singleplay playlist
	if (!settings_dialog_ui.singleplay_playlist->currentText().isNull() || !settings_dialog_ui.singleplay_playlist->currentText().isEmpty())
		settings_.set_singleplay_playlist(settings_dialog_ui.singleplay_playlist->currentText());

	// backend filepath
	if (settings_dialog_ui.backend_filepath->text() != settings_.get_backend_filepath())
	{
		settings_.set_backend_filepath(settings_dialog_ui.backend_filepath->text());
		change_backend_callback();
	}

	// TODO: send module UI properties to backend through the audio frontend

	return QDialog::Accepted;
}


void settings_dialog::open_backend_filepath_filedialog()
{
	QString backend_filepath = QFileDialog::getOpenFileName(this, "Select backend", settings_dialog_ui.backend_filepath->text());
	if (!backend_filepath.isNull() && !backend_filepath.isEmpty())
		settings_dialog_ui.backend_filepath->setText(backend_filepath);
}


void settings_dialog::selected_module_changed(QModelIndex const &new_selection)
{
	if (!new_selection.isValid())
	{
		settings_dialog_ui.module_gui_view->setUrl(QUrl("about:blank"));
		return;
	}


	audio_frontend::module_entry_sequence_t const &module_entry_sequence = audio_frontend_.get_module_entries().get < audio_frontend::sequence_tag > ();
	audio_frontend::module_entry_sequence_t::const_iterator iter = module_entry_sequence.begin() + new_selection.row();

	if (iter == module_entry_sequence.end())
	{
		settings_dialog_ui.module_gui_view->setUrl(QUrl("about:blank"));
		return;
	}

	settings_dialog_ui.module_gui_view->setHtml(iter->html_code.c_str());

	// TODO: transmit ui properties to WebKit
}


settings_dialog::~settings_dialog()
{
}


}
}


#include "settings_dialog.moc"

