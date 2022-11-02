#include "logger.h"

#define INFOLOG(msg) "<font color=\"green\">" + msg + "</font>"
#define ERRLOG(msg) "<font color=\"red\">" + msg + "</font>"
#define WARNLOG(msg) "<font color=\"gold\">" + msg + "</font>"

Logger *Logger::instance = nullptr;

Logger::Logger(QObject *parent) : QObject(parent) {
  instance = this;
  qInstallMessageHandler(messageHandler);
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &,
                            const QString &msg) {
  auto output = msg;
  auto len = msg.length();
  // 去掉烦人的双引号字符串输出，比如 "hello world" 这种输出
  if (len) {
    if (msg[0] == '"' && msg[len - 1] == '"') {
      output = msg.mid(1, len - 2);
    }
    switch (type) {
    case QtMsgType::QtDebugMsg:
      emit instance->log(tr("[Debug]") + output);
      break;
    case QtMsgType::QtInfoMsg:
      emit instance->log(INFOLOG(tr("[Info]") + output));
      break;
    case QtMsgType::QtWarningMsg:
      emit instance->log(WARNLOG(tr("[Warn]") + output));
      break;
    case QtMsgType::QtCriticalMsg:
      emit instance->log(ERRLOG(tr("[Error]") + output));
      break;
    default:
      break;
    }
  } else {
    emit instance->log("");
  }
}

Logger *Logger::getInstance() {
  if (instance == nullptr) {
    instance = new Logger;
  }
  return instance;
}
