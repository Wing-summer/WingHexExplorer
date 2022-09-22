#ifndef FILEINFODIALOG_H
#define FILEINFODIALOG_H

#include <DDialog>
#include <DMainWindow>
#include <QObject>

DWIDGET_USE_NAMESPACE

class FileInfoDialog : public DDialog {
  Q_OBJECT
public:
  FileInfoDialog(QString filename, DMainWindow *parent = nullptr);
};

#endif // FILEINFODIALOG_H
