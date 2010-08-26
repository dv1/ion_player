#include <QTreeView>
#include <QTabWidget>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <ion/flat_playlist.hpp>
#include "playlist_qt_model.hpp"
#include "playlists_ui.hpp"
#include "audio_frontend.hpp"


namespace ion
{
namespace frontend
{


playlist_ui::playlist_ui(QObject *parent, playlists_t::playlist_t &playlist_, playlists_ui &playlists_ui_):
	QObject(parent),
	playlist_(playlist_),
	playlists_ui_(playlists_ui_),
	view_widget(0),
	playlist_qt_model_(0)
{
	QTabWidget &tab_widget = playlists_ui_.get_tab_widget();

	playlist_renamed_signal_connection = playlist_.get_playlist_renamed_signal().connect(boost::lambda::bind(&playlist_ui::playlist_renamed, this, boost::lambda::_1));

	playlist_qt_model_ = new playlist_qt_model(&tab_widget, playlists_ui_.get_playlists(), playlist_);
	current_uri_changed_signal_connection = playlists_ui_.get_audio_frontend().get_current_uri_changed_signal().connect(boost::lambda::bind(&playlist_qt_model::current_uri_changed, playlist_qt_model_, boost::lambda::_1));

	view_widget = new QTreeView(&tab_widget);
	view_widget->setRootIsDecorated(false);
	view_widget->setAlternatingRowColors(true);
	view_widget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	view_widget->setModel(playlist_qt_model_);
	tab_widget.addTab(view_widget, playlist_.get_name().c_str());


	connect(view_widget, SIGNAL(doubleClicked(QModelIndex const &)), this, SLOT(play_song_in_row(QModelIndex const &)));
}


playlist_ui::~playlist_ui()
{
	current_uri_changed_signal_connection.disconnect();
	playlist_renamed_signal_connection.disconnect();

	QTabWidget &tab_widget = playlists_ui_.get_tab_widget();
	int index = tab_widget.indexOf(view_widget);
	assert(index != -1);
	tab_widget.removeTab(index);

	delete view_widget;
	delete playlist_qt_model_;
}


void playlist_ui::play_selected()
{
	QModelIndexList selected_rows = view_widget->selectionModel()->selectedRows();
	if (!selected_rows.empty())
		play_song_in_row(*selected_rows.begin());
}


void playlist_ui::remove_selected()
{
	//std::vector < ion::uri > uris_to_be_removed;
	uri_set_t uris_to_be_removed;

	QModelIndexList selected_rows = view_widget->selectionModel()->selectedRows();
	BOOST_FOREACH(QModelIndex const &index, selected_rows)
	{
		playlists_t::playlist_t::entry_t const *playlist_entry = playlist_.get_entry(index.row());
		uris_to_be_removed.insert(boost::fusion::at_c < 0 > (*playlist_entry));
	}

	playlist_.remove_entries(uris_to_be_removed);
}


void playlist_ui::play_song_in_row(QModelIndex const &index)
{
	playlists_t::playlist_t::entry_t const *playlist_entry = playlist_.get_entry(index.row());
	if (playlist_entry != 0)
	{
		playlists_ui_.get_playlists().set_active_playlist(&playlist_);
		playlists_ui_.get_audio_frontend().set_current_playlist(&playlist_);
		playlists_ui_.get_audio_frontend().play(boost::fusion::at_c < 0 > (*playlist_entry));
	}
}


void playlist_ui::playlist_renamed(std::string const &new_name)
{
	int index = playlists_ui_.get_tab_widget().indexOf(view_widget);
	if (index != -1)
		playlists_ui_.get_tab_widget().setTabText(index, new_name.c_str());
}






playlists_ui::playlists_ui(QTabWidget &tab_widget, audio_frontend &audio_frontend_, QObject *parent):
	QObject(parent),
	tab_widget(tab_widget),
	audio_frontend_(audio_frontend_)
{
	connect(&tab_widget, SIGNAL(tabCloseRequested(int)), this, SLOT(close_current_playlist(int)));

	playlist_added_signal_connection = playlists_.get_playlist_added_signal().connect(boost::lambda::bind(&playlists_ui::playlist_added, this, boost::lambda::_1));
	playlist_removed_signal_connection = playlists_.get_playlist_removed_signal().connect(boost::lambda::bind(&playlists_ui::playlist_removed, this, boost::lambda::_1));
}


playlists_ui::~playlists_ui()
{
	disconnect(&tab_widget, SIGNAL(tabCloseRequested(int)), this, SLOT(close_current_playlist(int)));
}


playlist_ui* playlists_ui::get_currently_visible_playlist_ui()
{
	QWidget *page_widget = tab_widget.currentWidget();
	BOOST_FOREACH(playlist_ui *ui, playlist_uis)
	{
		if (ui->get_view_widget() == page_widget)
			return ui;
	}

	return 0;
}


playlists_t::playlist_t* playlists_ui::get_currently_visible_playlist()
{
	playlist_ui *ui = get_currently_visible_playlist_ui();
	return (ui == 0) ? 0 : &(ui->get_playlist());
}


void playlists_ui::close_current_playlist(int index)
{
	if (index == -1)
		return;
	if (tab_widget.count() <= 1)
		return;

	QWidget *page_widget = tab_widget.widget(index);
	BOOST_FOREACH(playlist_ui *ui, playlist_uis)
	{
		if (ui->get_view_widget() == page_widget)
		{
			playlists_.remove_playlist(ui->get_playlist());
			break;
		}
	}
}


void playlists_ui::playlist_added(playlists_t::playlist_t &playlist_)
{
	playlist_ui *new_playlist_ui = new playlist_ui(this, playlist_, *this);
	playlist_uis.push_back(new_playlist_ui);
}


void playlists_ui::playlist_removed(playlists_t::playlist_t &playlist_)
{
}


}
}


#include "playlists_ui.moc"

