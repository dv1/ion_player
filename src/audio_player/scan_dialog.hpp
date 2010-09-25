#ifndef ION_AUDIO_PLAYER_SCAN_DIALOG_HPP
#define ION_AUDIO_PLAYER_SCAN_DIALOG_HPP

#include <QDialog>
#include "scanner.hpp"
#include "ui_scan_dialog.h"


namespace ion
{
namespace audio_player
{


class scan_queue_model:
	public QAbstractListModel
{
public:
	explicit scan_queue_model(QObject *parent, scanner::scan_queue_t const *scan_queue_);

	void set_scan_queue(scanner::scan_queue_t const *new_scan_queue);
	void reset();

	virtual int columnCount(QModelIndex const & parent) const;
	virtual QVariant data(QModelIndex const & index, int role) const;
	virtual QModelIndex parent(QModelIndex const & index) const;
	virtual int rowCount(QModelIndex const & parent) const;

protected:
	scanner::scan_queue_t const *scan_queue;
};



class scan_dialog:
	public QDialog
{
	Q_OBJECT
public:
	explicit scan_dialog(QWidget *parent, scanner *scanner_);
	~scan_dialog();


	void set_scanner(scanner *new_scanner);


protected slots:
	void scan_queue_updated();
	void general_scan_error(QString const &error_string);
	void resource_scan_error(QString const &error_type, QString const &uri_string);


protected:
	void closeEvent(QCloseEvent *event);
	void hideEvent(QHideEvent *event);
	void showEvent(QShowEvent *event);


	bool frozen;
	scanner *scanner_;
	scan_queue_model *scan_queue_model_;
	Ui_scan_dialog scan_dialog_ui;
};


}
}


#endif
