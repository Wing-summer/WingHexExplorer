#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>

class Logger : public QObject {
  Q_OBJECT
public:
  explicit Logger(QObject *parent = nullptr);
  static Logger *getInstance();
signals:
  void log(QString msg);

private:
  static void messageHandler(QtMsgType type, const QMessageLogContext &,
                             const QString &msg);

private:
  static Logger *instance;
};

#endif // LOGGER_H
