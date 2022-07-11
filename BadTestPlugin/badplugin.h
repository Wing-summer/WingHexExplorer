#ifndef BADPLUGIN_H
#define BADPLUGIN_H

#include "../WingHexExplorer/plugin/iwingplugin.h"
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
  int sdkVersion() override;
  bool init(QList<WingPluginInfo> loadedplugin) override;
  ~BadPlugin() override;
  void unload() override;
  QString pluginName() override;
  QString pluginAuthor() override;
  uint pluginVersion() override;
  QString signature() override;
  QString pluginComment() override;
  void plugin2MessagePipe(WingPluginMessage type, QList<QVariant> msg) override;
};

#endif // BADPLUGIN_H
