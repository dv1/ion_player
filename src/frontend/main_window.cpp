#include <QApplication>
#include <QDesktopWidget>
#include <QTreeView>
#include "main_window.hpp"


namespace ion
{
namespace frontend
{


main_window::main_window():
	in_compact_mode(true)
{
	main_bar_gen.setupUi(this);
	move_to_top_center();

	geometries[geometry_full] = geometry();

	main_bar_gen.playlist_tabs->hide();
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
	move_to_top_center();

	{
		int new_w = size().width();
		int new_h = layout()->minimumSize().height();

		resize(new_w, new_h);
	}

	geometries[geometry_compact] = geometry();

	connect(main_bar_gen.open_playlist_button, SIGNAL(clicked()), this, SLOT(toggle_playlist()));
	connect(main_bar_gen.quit_button, SIGNAL(clicked()), this, SLOT(close()));

	playlist_qt_model_ = new playlist_qt_model(this, playlist_);

	QTreeView *list = new QTreeView(this);
	list->setModel(playlist_qt_model_);
	list->setAlternatingRowColors(true);
	main_bar_gen.playlist_tabs->addTab(list, "playlist1");

	playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=1"), ion::metadata_t("{}")));
	playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=2"), ion::metadata_t("{}")));
	playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=3"), ion::metadata_t("{}")));
	playlist_.add_entry(ion::simple_playlist::entry(ion::uri("file://test/sound_samples/mods/test.xm?id=4"), ion::metadata_t("{}")));
}


main_window::~main_window()
{
	delete playlist_qt_model_;
}


void main_window::move_to_top_center()
{
	QRect screen_geometry = QApplication::desktop()->screenGeometry();
	move(screen_geometry.left() + (screen_geometry.width() - size().width()) / 2, 0);
}


void main_window::set_compact_mode(bool const mode)
{
	if (in_compact_mode == mode)
		return;

	geometries[in_compact_mode ? geometry_compact : geometry_full] = geometry();

	hide();
	if (mode)
	{
		main_bar_gen.playlist_tabs->hide();
		setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	}
	else
	{
		main_bar_gen.playlist_tabs->show();
		setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
	}
	// setGeometry must be called before show(), otherwise painting errors can occur
	setGeometry(geometries[mode ? geometry_compact : geometry_full]);
	show();

	in_compact_mode = mode;
}


void main_window::toggle_playlist()
{
	set_compact_mode(!in_compact_mode);
}


}
}


#include "main_window.moc"

