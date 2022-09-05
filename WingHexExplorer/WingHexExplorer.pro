QT += core gui dtkwidget dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = WingHexExplorer
TEMPLATE = app

include($$PWD/QHexView/QHexView.pri)
include($$PWD/WingHexExplorer.pri)
include($$PWD/qBreakpad/qBreakpad.pri)

CONFIG += exceptions
