#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>

#define INFOLOG(msg) "<font color=\"green\">" + msg + "</font><br />"
#define ERRLOG(msg) "<font color=\"red\">" + msg + "</font><br />"
#define WARNLOG(msg) "<font color=\"yellow\">" + msg + "</font><br />"

class Logger : public QObject {
  Q_OBJECT
public:
  explicit Logger(QObject *parent = nullptr);
  static Logger *getInstance();
  void logMessage(QString msg);
signals:
  void log(QString msg);
public slots:
private:
  static Logger *instance;
};

#endif // LOGGER_H
