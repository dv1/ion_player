#include <QMouseEvent>
#include <QStyle>
#include <QString>
#include <QToolTip>
#include <QStyleOptionSlider>
#include "direct_slider.hpp"


namespace ion
{
namespace audio_player
{


direct_slider::direct_slider(QWidget *parent):
	QSlider(parent),
	pressed(false)
{
	setMouseTracking(true);
}


void direct_slider::set_tooltip_text(QString const &new_text)
{
	tooltip_text = new_text;
}


void direct_slider::set_value(int new_position)
{
	if (!pressed)
		setValue(new_position);
}


void direct_slider::mousePressEvent(QMouseEvent *event)
{
	event->accept();

	pressed = true;

	if (minimum() < maximum())
	{
		int position = convert_pixel_position_to_value(event->x());
		setSliderPosition(position);
		show_tooltip_text(event, true);
	}
}


void direct_slider::mouseMoveEvent(QMouseEvent *event)
{
	event->accept();

	if (pressed)
	{
		if (minimum() < maximum())
		{
			int position = convert_pixel_position_to_value(event->x());
			setSliderPosition(position);
		}

		show_tooltip_text(event, true);
	}
	else
		show_tooltip_text(event, false);
}


void direct_slider::mouseReleaseEvent(QMouseEvent *event)
{
	event->accept();

	if (!pressed)
		return;

	if (minimum() < maximum())
		setValue(convert_pixel_position_to_value(event->x()));

	hide_tooltip_text();

	emit sliderReleased();

	pressed = false;
}


void direct_slider::show_tooltip_text(QMouseEvent *event, bool const immediately)
{
	if (tooltip_text.isNull() || tooltip_text.isEmpty())
		return;

	if (immediately)
		QToolTip::showText(event->globalPos(), tooltip_text, this);
	else
		setToolTip(tooltip_text);
}


void direct_slider::hide_tooltip_text()
{
	setToolTip("");
	QToolTip::hideText();
}


int direct_slider::convert_pixel_position_to_value(int const pixel_position)
{
	QStyleOptionSlider opt;
	initStyleOption(&opt);

	QRect groove_rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
	QRect slider_handle_rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
	int slider_handle_offset = slider_handle_rect.center().x() - slider_handle_rect.left();

	int groove_min, groove_max;
	if (orientation() == Qt::Horizontal)
	{
		groove_min = groove_rect.left();
		groove_max = groove_rect.right() - slider_handle_rect.width() + 1;
	}
	else
	{
		groove_min = groove_rect.top();
		groove_max = groove_rect.bottom() - slider_handle_rect.height() + 1;
	}

	return QStyle::sliderValueFromPosition(minimum(), maximum(), pixel_position - slider_handle_offset - groove_min, groove_max - groove_min, false);
}


}
}

