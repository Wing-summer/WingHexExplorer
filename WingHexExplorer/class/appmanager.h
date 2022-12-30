#ifndef APPMANAGER_H
#define APPMANAGER_H

#include "./dialog/mainwindow.h"
#include <QObject>

class AppManager : public QObject {
  Q_OBJECT

public:
  static AppManager *instance();
  explicit AppManager(QObject *parent = nullptr);
  static MainWindow *mWindow;
  static ErrFile openFile(QString file);

private:
  static AppManager *m_instance;
signals:

public slots:
  static void openFiles(QStringList files);
};

#endif // APPMANAGER_H
