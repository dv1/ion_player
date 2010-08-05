#ifndef ION_FRONTEND_PLAYLISTS_HPP
#define ION_FRONTEND_PLAYLISTS_HPP

#include <ion/simple_playlist.hpp>
#include <boost/noncopyable.hpp>


class QTabWidget;


namespace ion
{
namespace frontend
{


class playlists:
	private boost::noncopyable
{
public:
	typedef simple_playlist playlist_t;

	struct entry
	{
		QString const name;
		playlist_t playlist_;
		QTreeView *view_widget;
		playlist_qt_model *playlist_qt_model_;
	};


	explicit playlists(QTabWidget &tab_widget);

	entry& add_entry(QString const &playlist_name);
	void remove_entry(QTreeView *entry_view_widget);


protected:
	QTabWidget &tab_widget;
};


}
}


#endif

