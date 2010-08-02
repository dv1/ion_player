#ifndef ION_FRONTEND_QT4_MAIN_WINDOW_HPP
#define ION_FRONTEND_QT4_MAIN_WINDOW_HPP

#include <QWidget>
#include "ui_main_bar.h"
#include "playlist_qt_model.hpp"


namespace ion
{
namespace frontend
{


class main_window:
	public QWidget
{
	Q_OBJECT
public:
	explicit main_window();
	~main_window();

	void set_compact_mode(bool const mode);

protected slots:
	void toggle_playlist();

protected:
	void move_to_top_center();


	Ui::main_bar main_bar_gen;
	bool in_compact_mode;

	enum
	{
		geometry_full = 0,
		geometry_compact = 1
	};

	QRect geometries[2];

	simple_playlist playlist_;
	playlist_qt_model *playlist_qt_model_;
};


}
}


#endif

