#ifndef ION_FRONTEND_DIRECT_SLIDER_HPP
#define ION_FRONTEND_DIRECT_SLIDER_HPP

#include <QMouseEvent>
#include <QSlider>
#include <QStyle>


namespace ion
{
namespace frontend
{


class direct_slider:
	public QSlider
{
public:
	direct_slider(QWidget *parent):
		QSlider(parent)
	{
	}


protected:
	/*void mouseReleaseEvent(QMouseEvent *event)
	{
		if (minimum() < maximum())
			setValue(QStyle::sliderValueFromPosition(minimum(), maximum(), event->x(), width(), false));
	}*/
};


}
}


#endif

