#include "appmanager.h"

AppManager *AppManager::m_instance = nullptr;
MainWindow *AppManager::mWindow = nullptr;

AppManager::AppManager(QObject *parent) : QObject(parent) {}

AppManager *AppManager::instance() {
  if (m_instance == nullptr) {
    m_instance = new AppManager;
  }

  return m_instance;
}

void AppManager::openFiles(QStringList files) {
  if (mWindow) {
    for (auto file : files) {
      if (mWindow->openWorkSpace(file) != ErrFile::Success)
        mWindow->openFile(file);
    }
    //通过dbus接口从任务栏激活窗口
    if (!Q_LIKELY(Utilities::activeWindowFromDock(mWindow->winId()))) {
      mWindow->activateWindow();
    }
  }
}
