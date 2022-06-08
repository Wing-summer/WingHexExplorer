#include "winghexapplication.h"
#include <QDateTime>
#include <QDir>
#include <QException>
#include <QMessageBox>
#include <QStandardPaths>
#include <QUnhandledException>

WingHexApplication::WingHexApplication(int &argc, char **argv)
    : DApplication(argc, argv) {}

bool WingHexApplication::notify(QObject *obj, QEvent *event) {
  bool done = true;
  try {
    done = QApplication::notify(obj, event);
  } catch (const QUnhandledException &ex) {
    auto p = this->applicationDirPath();
    auto errlog = "errlog";
    QDir dir(p);
    dir.mkdir(errlog);
    dir.cd(errlog);
    auto time = QDateTime::currentDateTimeUtc();
    auto ferrname = QString::number(time.currentSecsSinceEpoch(), 16) + ".log";
    QFile log(dir.absolutePath() + "/" + ferrname);
    if (log.open(QFile::WriteOnly)) {
      log.write(ex.what());
      log.close();
    } else {
      log.setFileName(
          QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" +
          ferrname);
      if (log.open(QFile::WriteOnly)) {
        log.write(ex.what());
        log.close();
      } else {
        QMessageBox::critical(nullptr, tr("Error"),
                              tr("WriteErrorLogError") + ferrname);
        QMessageBox::information(nullptr, tr("Crash"), ex.what());
      }
    }
  } catch (const QException &ex) {
    auto p = this->applicationDirPath();
    auto errlog = "errlog";
    QDir dir(p);
    dir.mkdir(errlog);
    dir.cd(errlog);
    auto time = QDateTime::currentDateTimeUtc();
    auto ferrname = QString::number(time.currentSecsSinceEpoch(), 16) + ".log";
    QFile log(dir.absolutePath() + "/" + ferrname);
    if (log.open(QFile::WriteOnly)) {
      log.write(ex.what());
      log.close();
    } else {
      log.setFileName(
          QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" +
          ferrname);
      if (log.open(QFile::WriteOnly)) {
        log.write(ex.what());
        log.close();
      } else {
        QMessageBox::critical(nullptr, tr("Error"),
                              tr("WriteErrorLogError") + ferrname);
        QMessageBox::information(nullptr, tr("Crash"), ex.what());
      }
    }
  }
  return done;
}
