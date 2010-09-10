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

