#ifndef PLUGINSYSTEM_H
#define PLUGINSYSTEM_H

#include "iwingplugin.h"
#include "logger.h"
#include <QDockWidget>
#include <QList>
#include <QMap>
#include <QMenu>
#include <QObject>
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

private:
  const QList<QVariant> emptyparam;

private slots:
  void messagePipe(IWingPlugin *sender, WingPluginMessage type,
                   QList<QVariant> msg);

signals:
  void PluginMenuNeedAdd(QMenu *menu);
  void PluginDockWidgetAdd(QDockWidget *dockw, Qt::DockWidgetArea align);
  ResponseMsg PluginCall(CallTableIndex index, QList<QVariant> params);

private:
  QList<IWingPlugin *> loadedplgs;
  QMap<HookIndex, QList<IWingPlugin *>> dispatcher;
  Logger *logger;
};

#endif // PLUGINSYSTEM_H
