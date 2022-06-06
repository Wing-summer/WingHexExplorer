#ifndef DRIVERSELECTORDIALOG_H
#define DRIVERSELECTORDIALOG_H

#include <DDialog>
#include <DListWidget>
#include <DMainWindow>
#include <DTextBrowser>
#include <QList>
#include <QObject>
#include <QStorageInfo>

DWIDGET_USE_NAMESPACE

class DriverSelectorDialog : public DDialog {
  Q_OBJECT
public:
  DriverSelectorDialog(DMainWindow *parent = nullptr);
  QStorageInfo GetResult();

private:
  DListWidget *drivers;
  DTextBrowser *infob;
  QList<QStorageInfo> m_infos;
  QStorageInfo m_si;

  void on_list_selectionChanged();

  void on_accepted();
  void on_rejected();
};

#endif // DRIVERSELECTORDIALOG_H
