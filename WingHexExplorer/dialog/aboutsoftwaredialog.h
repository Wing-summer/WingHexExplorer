#ifndef ABOUTSOFTWAREDIALOG_H
#define ABOUTSOFTWAREDIALOG_H

#include <DDialog>
#include <DMainWindow>
#include <QObject>

DWIDGET_USE_NAMESPACE
class AboutSoftwareDialog : public DDialog {

  Q_OBJECT
public:
  explicit AboutSoftwareDialog(DMainWindow *parent = nullptr,
                               QPixmap img = QPixmap(),
                               QStringList searchPaths = QStringList(),
                               QString source = QString());

signals:

public slots:
};

#endif // ABOUTSOFTWAREDIALOG_H
