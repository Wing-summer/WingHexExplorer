#ifndef BADPLUGIN_H
#define BADPLUGIN_H

#include "iwingplugin.h"
#include <QList>
#include <QObject>

class BadPlugin : public IWingPlugin {

  Q_OBJECT
#if QT_VERSION >= 0x050000
  Q_PLUGIN_METADATA(IID IWINGPLUGIN_INTERFACE_IID FILE "BadTestPlugin.json")
#endif // QT_VERSION >= 0x050000
  Q_INTERFACES(IWingPlugin)

public:
  BadPlugin(QObject *parent = nullptr);
};

#endif // BADPLUGIN_H
