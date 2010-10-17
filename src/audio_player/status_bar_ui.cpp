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

#include <QWidget>
#include <QLabel>
#include <QStatusBar>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include "status_bar_ui.hpp"
#include "scan_indicator_icon.hpp"


namespace ion
{
namespace audio_player
{


class scan_indicator_icon;


status_bar_ui::status_bar_ui(QWidget *parent, QStatusBar *status_bar, audio_common::audio_frontend &audio_frontend_):
	QObject(parent)
{
	current_song_title = new QLabel(parent);
	current_playback_time = new QLabel(parent);
	current_song_length = new QLabel(parent);
	scan_indicator_icon_ = new scan_indicator_icon(parent);
	current_song_title->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	current_playback_time->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	current_song_length->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	scan_indicator_icon_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	status_bar->addPermanentWidget(scan_indicator_icon_);
	status_bar->addPermanentWidget(current_song_title);
	status_bar->addPermanentWidget(current_playback_time);
	status_bar->addPermanentWidget(current_song_length);

	current_uri_changed_signal_connection = audio_frontend_.get_current_uri_changed_signal().connect(boost::phoenix::bind(&status_bar_ui::current_uri_changed, this, boost::phoenix::arg_names::arg1));
	current_metadata_changed_signal_connection = audio_frontend_.get_current_metadata_changed_signal().connect(boost::phoenix::bind(&status_bar_ui::current_metadata_changed, this, boost::phoenix::arg_names::arg1));
}


status_bar_ui::~status_bar_ui()
{
	current_uri_changed_signal_connection.disconnect();
	current_metadata_changed_signal_connection.disconnect();
}


void status_bar_ui::current_uri_changed(uri_optional_t const &)
{
	current_playback_time->setText(get_time_string(0, 0));
}


void status_bar_ui::current_metadata_changed(metadata_optional_t const &new_metadata)
{
	if (new_metadata)
	{
		std::string title = get_metadata_value < std::string > (*new_metadata, "title", "");
		if (title.empty())
			current_song_title->setText("");
		else
			current_song_title->setText(QString::fromUtf8(title.c_str()));

		unsigned int num_ticks = get_metadata_value < unsigned int > (*new_metadata, "num_ticks", 0);
		current_num_ticks_per_second = get_metadata_value < unsigned int > (*new_metadata, "num_ticks_per_second", 0);

		if (num_ticks > 0)
		{
			if (current_num_ticks_per_second > 0)
			{
				unsigned int length_in_seconds = num_ticks / current_num_ticks_per_second;
				unsigned int minutes = length_in_seconds / 60;
				unsigned int seconds = length_in_seconds % 60;

				current_playback_time->setText(get_time_string(0, 0));
				current_song_length->setText(get_time_string(minutes, seconds));
			}
			else
			{
				current_song_length->setText("");
				current_playback_time->setText("");
			}
		}
		else
		{	
			current_song_length->setText("");
			current_playback_time->setText(get_time_string(0, 0));
		}
	}
	else
	{
		current_song_title->setText("");
		current_song_length->setText("");
		current_playback_time->setText("");
	}
}


void status_bar_ui::set_current_time_label(unsigned int const current_position)
{
	unsigned int time_in_seconds = 0;
	if (current_num_ticks_per_second > 0)
	{
		time_in_seconds = current_position / current_num_ticks_per_second;

		unsigned int minutes = time_in_seconds / 60;
		unsigned int seconds = time_in_seconds % 60;

		current_playback_time->setText(get_time_string(minutes, seconds));
	}
	else
		current_playback_time->setText("");
}


QString status_bar_ui::get_time_string(int const minutes, int const seconds) const
{
	return QString("%1:%2")
		.arg(minutes, 2, 10, QChar('0'))
		.arg(seconds, 2, 10, QChar('0'))
		;
}


}
}

