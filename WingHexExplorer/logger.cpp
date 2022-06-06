#include "logger.h"

Logger::Logger(QObject *parent) : QObject(parent) {}

void Logger::logMessage(QString msg) { emit log(msg); }
