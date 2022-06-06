#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>

class Logger : public QObject {
  Q_OBJECT
public:
  explicit Logger(QObject *parent = nullptr);
  void logMessage(QString msg);
signals:
  void log(QString msg);
public slots:
};

#endif // LOGGER_H
