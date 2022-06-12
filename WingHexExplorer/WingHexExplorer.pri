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
    logger.cpp \
    winghexapplication.cpp \
    settingdialog.cpp \
    $$PWD/hexviewshadow.cpp \
    $$PWD/dialog/encodingdialog.cpp


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
    logger.h \
    winghexapplication.h \
    $$PWD/hexviewshadow.h \
    $$PWD/dialog/encodingdialog.h


TRANSLATIONS += \
    lang/zh.ts