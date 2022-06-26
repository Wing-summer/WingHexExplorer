#ifndef PLUGINSYSTEM_H
#define PLUGINSYSTEM_H

#include "class/logger.h"
#include "plugin/iwingplugin.h"
#include <QDockWidget>
#include <QList>
#include <QMap>
#include <QMenu>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include <QTimerEvent>
#include <QVariant>

class PluginSystem : public QObject {
  Q_OBJECT
public:
  PluginSystem(QObject *parent = nullptr);
  ~PluginSystem();
  bool LoadPlugin();
  void UnloadPlugin();
  QList<IWingPlugin *> plugins();
  void raiseDispatch(HookIndex hookindex, QList<QVariant> params);

  bool requestControl(IWingPlugin *plugin, int timeout = 1500);
  bool requestRelease(IWingPlugin *plugin);
  bool hasControl();
  void resetTimeout(IWingPlugin *plugin);

  void initControl(IWingPlugin *plugin);
  void loadPlugin(QFileInfo filename);

private:
  const QList<QVariant> emptyparam;

signals:
  void PluginMenuNeedAdd(QMenu *menu);
  void PluginDockWidgetAdd(QDockWidget *dockw, Qt::DockWidgetArea align);
  void ConnectBase(IWingPlugin *plugin);
  void ConnectControl(IWingPlugin *plugin);
  void DisconnectControl(IWingPlugin *plugin);

private:
  QStringList loadedpuid;
  QList<IWingPlugin *> loadedplgs;
  QMap<IWingPlugin *, bool> plugintimeout;
  QMap<IWingPlugin *, QTimer *> plugintimer;
  IWingPlugin *curpluginctl;
  QMap<HookIndex, QList<IWingPlugin *>> dispatcher;
  Logger *logger;
  QMutex mutex;
};

#endif // PLUGINSYSTEM_H
