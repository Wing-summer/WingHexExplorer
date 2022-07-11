#ifndef GENERICPLUGIN_H
#define GENERICPLUGIN_H

#include "../WingHexExplorer/plugin/iwingplugin.h"
#include <QList>
#include <QObject>

class TestPlugin : public IWingPlugin {

  Q_OBJECT
#if QT_VERSION >= 0x050000
  Q_PLUGIN_METADATA(IID IWINGPLUGIN_INTERFACE_IID FILE "TestPlugin.json")
#endif // QT_VERSION >= 0x050000

  Q_INTERFACES(IWingPlugin)

public:
  TestPlugin(QObject *parent = nullptr);
  bool init(QList<WingPluginInfo> loadedplugin) override;
  ~TestPlugin() override;
  void unload() override;
  int sdkVersion() override;
  QMenu *registerMenu() override;
  QDockWidget *registerDockWidget() override;
  QString pluginName() override;
  QString pluginAuthor() override;
  uint pluginVersion() override;
  QString signature() override;
  QString pluginComment() override;
  void plugin2MessagePipe(WingPluginMessage type, QList<QVariant> msg) override;
  Qt::DockWidgetArea registerDockWidgetDockArea() override;

private:
  QMenu *testmenu;
  QDockWidget *dw;
};

#endif // GENERICPLUGIN_H
