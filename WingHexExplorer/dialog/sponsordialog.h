#ifndef SPONSORDIALOG_H
#define SPONSORDIALOG_H

#include <DDialog>
#include <DMainWindow>
#include <QObject>

DWIDGET_USE_NAMESPACE
class SponsorDialog : public DDialog {
  Q_OBJECT
public:
  explicit SponsorDialog(DMainWindow *parent = nullptr,
                         QString message = QString(), QPixmap img = QPixmap());

signals:

public slots:
};

#endif // SPONSORDIALOG_H
