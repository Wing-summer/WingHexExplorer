SOURCES += \
        $$PWD/main.cpp \
    $$PWD/dialog/mainwindow.cpp \
    $$PWD/QHexEdit2/chunks.cpp \
    $$PWD/class/appmanager.cpp \
    $$PWD/control/gotobar.cpp \
    $$PWD/settings.cpp \
    $$PWD/dialog/finddialog.cpp \
    $$PWD/dialog/pluginwindow.cpp \
    $$PWD/plugin/pluginsystem.cpp \
    $$PWD/dialog/metadialog.cpp \
    $$PWD/dialog/driverselectordialog.cpp \
    $$PWD/dialog/aboutsoftwaredialog.cpp \
    $$PWD/dialog/sponsordialog.cpp \
    $$PWD/class/logger.cpp \
    $$PWD/winghexapplication.cpp \
    $$PWD/settingdialog.cpp \
    $$PWD/plugin/hexviewshadow.cpp \
    $$PWD/dialog/encodingdialog.cpp \
    $$PWD/class/workspacemanager.cpp


RESOURCES +=         resources.qrc

DISTFILES += \
    README.md

HEADERS += \
    $$PWD/dialog/mainwindow.h \
    $$PWD/utilities.h \
    $$PWD/QHexEdit2/chunks.h \
    $$PWD/class/appmanager.h \
    $$PWD/control/gotobar.h \
    $$PWD/settings.h \
    $$PWD/dialog/finddialog.h \
  $$PWD/dialog/pluginwindow.h \
    $$PWD/plugin/pluginsystem.h \
    $$PWD/plugin/iwingplugin.h \
  $$PWD/dialog/metadialog.h \
    $$PWD/dialog/driverselectordialog.h \
    $$PWD/dialog/aboutsoftwaredialog.h \
    $$PWD/dialog/sponsordialog.h \
    $$PWD/class/logger.h \
    $$PWD/winghexapplication.h \
    $$PWD/plugin/hexviewshadow.h \
    $$PWD/dialog/encodingdialog.h \
    $$PWD/class/workspacemanager.h


TRANSLATIONS += \
    $$PWD/lang/zh.ts