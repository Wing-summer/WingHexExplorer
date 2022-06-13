#include "logger.h"

Logger *Logger::instance = nullptr;

Logger::Logger(QObject *parent) : QObject(parent) { instance = this; }

void Logger::logMessage(QString msg) { emit log(msg); }

Logger *Logger::getInstance() {
  if (instance == nullptr) {
    instance = new Logger;
  }
  return instance;
}
