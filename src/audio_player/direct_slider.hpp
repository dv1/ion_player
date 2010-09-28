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


#ifndef ION_AUDIO_PLAYER_DIRECT_SLIDER_HPP
#define ION_AUDIO_PLAYER_DIRECT_SLIDER_HPP

#include <QSlider>


namespace ion
{
namespace audio_player
{


class direct_slider:
	public QSlider
{
public:
	direct_slider(QWidget *parent);

	void set_tooltip_text(QString const &new_text);
	void set_value(int new_position);


protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	void show_tooltip_text(QMouseEvent *event, bool const immediately);
	void hide_tooltip_text();
	int convert_pixel_position_to_value(int const pixel_position);


	bool pressed;
	QString tooltip_text;
};


}
}


#endif

