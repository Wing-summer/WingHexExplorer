SOURCES += \
        $$PWD/main.cpp \
    $$PWD/dialog/mainwindow.cpp \
    $$PWD/control/gotobar.cpp \
    $$PWD/settings.cpp \
    $$PWD/dialog/finddialog.cpp \
    $$PWD/dialog/pluginwindow.cpp \
    $$PWD/plugin/pluginsystem.cpp \
    $$PWD/dialog/metadialog.cpp \
    $$PWD/dialog/driverselectordialog.cpp \
    $$PWD/dialog/aboutsoftwaredialog.cpp \
    $$PWD/dialog/sponsordialog.cpp \
    $$PWD/settingdialog.cpp \
    $$PWD/dialog/encodingdialog.cpp \
    $$PWD/class/workspacemanager.cpp \
    $$PWD/class/recentfilemanager.cpp \
    $$PWD/class/appmanager.cpp \
    $$PWD/class/logger.cpp \
    $$PWD/mlicense/licensemanager.cpp \
    $$PWD/mlicense/lincensedialog.cpp \
    $$PWD/dialog/openregiondialog.cpp \
    $$PWD/dialog/fileinfodialog.cpp


RESOURCES +=         resources.qrc

DISTFILES += \
    README.md

HEADERS += \
  $$PWD/dialog/mainwindow.h \
    $$PWD/utilities.h \
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
    $$PWD/dialog/encodingdialog.h \
    $$PWD/class/workspacemanager.h \
    $$PWD/class/recentfilemanager.h \
    $$PWD/class/appmanager.h \
    $$PWD/class/logger.h \
    $$PWD/mlicense/licensemanager.h \
    $$PWD/mlicense/lincensedialog.h \
    $$PWD/mlicense/qaesencryption.h \
    $$PWD/dialog/openregiondialog.h \
    $$PWD/dialog/fileinfodialog.h

LIBS += $$PWD/mlicense/libQtAES.a

TRANSLATIONS += \
    $$PWD/lang/zh.ts

DEFINES += USE_INTEL_AES_IF_AVAILABLE
