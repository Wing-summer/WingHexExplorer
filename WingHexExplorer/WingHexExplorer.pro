QT += core gui dtkwidget dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = WingHexExplorer
TEMPLATE = app

SOURCES += \
        main.cpp \
    dialog/mainwindow.cpp \
    QHexEdit2/chunks.cpp \
    appmanager.cpp \
    gotobar.cpp \
    settings.cpp \
    dialog/finddialog.cpp \
    dialog/pluginwindow.cpp \
    pluginsystem.cpp \
    dialog/metadialog.cpp \
    dialog/driverselectordialog.cpp \
    dialog/aboutsoftwaredialog.cpp \
    dialog/sponsordialog.cpp \
    QHexView/qhexview.cpp \
    QHexView/document/qhexrenderer.cpp \
    QHexView/document/qhexmetadata.cpp \
    QHexView/document/qhexdocument.cpp \
    QHexView/document/qhexcursor.cpp \
    QHexView/document/commands/hexcommand.cpp \
    QHexView/document/commands/insertcommand.cpp \
    QHexView/document/commands/removecommand.cpp \
    QHexView/document/commands/replacecommand.cpp \
    QHexView/document/buffer/qfilebuffer.cpp \
    QHexView/document/buffer/qhexbuffer.cpp \
    QHexView/document/buffer/qmemorybuffer.cpp \
    QHexView/document/buffer/qmemoryrefbuffer.cpp \
    logger.cpp


RESOURCES +=         resources.qrc

DISTFILES += \
    README.md

HEADERS += \
    dialog/mainwindow.h \
    utilities.h \
    QHexEdit2/chunks.h \
    appmanager.h \
    gotobar.h \
    settings.h \
    dialog/finddialog.h \
   dialog/pluginwindow.h \
    pluginsystem.h \
    iwingplugin.h \
  dialog/metadialog.h \
    dialog/driverselectordialog.h \
    dialog/aboutsoftwaredialog.h \
    dialog/sponsordialog.h \
    QHexView/qhexview.h \
    QHexView/document/qhexrenderer.h \
    QHexView/document/qhexmetadata.h \
    QHexView/document/qhexdocument.h \
    QHexView/document/qhexcursor.h \
    QHexView/document/commands/hexcommand.h \
    QHexView/document/commands/insertcommand.h \
    QHexView/document/commands/removecommand.h \
    QHexView/document/commands/replacecommand.h \
    QHexView/document/buffer/qfilebuffer.h \
    QHexView/document/buffer/qhexbuffer.h \
    QHexView/document/buffer/qmemorybuffer.h \
    QHexView/document/buffer/qmemoryrefbuffer.h \
    logger.h


TRANSLATIONS += \
    lang/zh.ts
