#ifndef ION_FRONTEND_QT4_MAIN_WINDOW_HPP
#define ION_FRONTEND_QT4_MAIN_WINDOW_HPP

#include <QWidget>


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
};


}
}


#endif

