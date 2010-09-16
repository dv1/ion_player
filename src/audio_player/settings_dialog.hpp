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
namespace audio_player
{


class audio_frontend;
class settings;


class module_entries_model:
	public QAbstractListModel
{
public:
	explicit module_entries_model(QObject *parent, audio_frontend &audio_frontend_);
	~module_entries_model();

	virtual int columnCount(QModelIndex const & parent) const;
	virtual QVariant data(QModelIndex const & index, int role) const;
	virtual QModelIndex parent(QModelIndex const & index) const;
	virtual int rowCount(QModelIndex const & parent) const;

protected:
	audio_frontend &audio_frontend_;
	boost::signals2::connection module_entries_updated_signal_connection;
};


class settings_dialog:
	public QDialog 
{
	Q_OBJECT
public:
	typedef boost::function < void() > change_backend_callback_t;

	explicit settings_dialog(QWidget *parent, settings &settings_, audio_frontend &audio_frontend_, playlists_t const &playlists_, change_backend_callback_t const &change_backend_callback);
	~settings_dialog();


	int run_dialog();


protected slots:
	void open_backend_filepath_filedialog();
	void selected_module_changed(QModelIndex const &new_selection);
	void populate_javascript();


protected:
	void set_module_ui(audio_frontend::module_entry const &module_entry_);


	audio_frontend::module_entry const *current_module_entry;
	settings &settings_;
	audio_frontend &audio_frontend_;
	playlists_t const &playlists_;
	change_backend_callback_t change_backend_callback;
	module_entries_model *module_entries_model_;
	Ui_settings_dialog settings_dialog_ui;
};


}
}


#endif

