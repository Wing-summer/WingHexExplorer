#ifndef FILEINFODIALOG_H
#define FILEINFODIALOG_H

#include <DDialog>
#include <DMainWindow>
#include <QObject>

DWIDGET_USE_NAMESPACE

class FileInfoDialog : public DDialog {
  Q_OBJECT
public:
  FileInfoDialog(QString filename, bool isRegionFile,
                 DMainWindow *parent = nullptr);
  ~FileInfoDialog();
};

#endif // FILEINFODIALOG_H
