#ifndef APPMANAGER_H
#define APPMANAGER_H

#include "./dialog/mainwindow.h"
#include <QObject>

// 参考：Deepin 自带的文本编辑器的单例通信方式

class AppManager : public QObject {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "com.Wingsummer.WingHexExplorer")

public:
  static AppManager *instance();
  explicit AppManager(QObject *parent = nullptr);
  static MainWindow *mWindow;
  static ErrFile openFile(QString file);

private:
  static AppManager *m_instance;
signals:

public slots:
  Q_SCRIPTABLE void openFiles(QStringList files);
};

#endif // APPMANAGER_H
