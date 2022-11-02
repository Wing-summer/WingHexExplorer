#include "appmanager.h"
#include <DMessageManager>
#include <QMessageBox>

AppManager *AppManager::m_instance = nullptr;
MainWindow *AppManager::mWindow = nullptr;

AppManager::AppManager(QObject *parent) : QObject(parent) {}

AppManager *AppManager::instance() {
  if (m_instance == nullptr) {
    m_instance = new AppManager;
  }

  return m_instance;
}

ErrFile AppManager::openFile(QString file, bool readonly) {
  auto res = ErrFile::Error;
  int oldindex = 0;
  if (mWindow) {
    res = mWindow->openWorkSpace(file, readonly, &oldindex);
    if (res != ErrFile::Success) {
      if (res == ErrFile::AlreadyOpened || res == ErrFile::WorkSpaceUnSaved) {
        mWindow->setFilePage(oldindex);
      } else {
        res = mWindow->openFile(file, readonly, &oldindex);
        if (res != ErrFile::Success) {
          if (res == ErrFile::AlreadyOpened ||
              res == ErrFile::WorkSpaceUnSaved) {
            mWindow->setFilePage(oldindex);
          }
        }
      }
    }
  }
  return res;
}

void AppManager::openFiles(QStringList files) {
  if (mWindow) {
    bool err = false;
    QStringList errof;
    for (auto file : files) {
      if (openFile(file) == ErrFile::Permission) {
        if (openFile(file, true)) {
          err = true;
          errof << file;
        }
      }
    }

    mWindow->show();
    mWindow->raise();
    mWindow->activateWindow();

    if (err) {
      QMessageBox::critical(mWindow, tr("Error"),
                            tr("ErrOpenFileBelow") + "\n" + errof.join('\n'));
    }
  }
}
