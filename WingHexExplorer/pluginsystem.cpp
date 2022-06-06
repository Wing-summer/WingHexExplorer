#include "pluginsystem.h"
#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QPluginLoader>
#include <QtCore>

PluginSystem::PluginSystem(QObject *parent) : QObject(parent) {}

PluginSystem::~PluginSystem() {
  for (auto item : loadedplgs) {
    item->unload();
    item->deleteLater();
  }
}

QList<IWingPlugin *> PluginSystem::plugins() { return loadedplgs; }

bool PluginSystem::LoadPlugin() {
  QDir plugindir(QCoreApplication::applicationDirPath() + "/plugin");
  plugindir.setNameFilters(QStringList("*.wingplg"));
  auto plgs = plugindir.entryInfoList();
  for (auto item : plgs) {
    if (item.exists()) {
      QPluginLoader loader(item.absoluteFilePath());
      auto p = qobject_cast<IWingPlugin *>(loader.instance());
      if (p) {
        if (p->signature() != sign)
          loader.unload();
        auto puid = PluginUtils::GetPUID(p);
        if (puid != p->puid())
          loader.unload();
        p->self = p;

        emit p->plugin2MessagePipe(WingPluginMessage::PluginLoading,
                                   emptyparam);

        p->init(loadedplgs);

        loadedplgs.push_back(p);

        auto menu = p->registerMenu();
        if (menu) {
          emit this->PluginMenuNeedAdd(menu);
        }

        auto dockw = p->registerDockWidget();
        if (dockw) {
          emit this->PluginDockWidgetAdd(dockw,
                                         p->registerDockWidgetDockArea());
        }

        connect(p, &IWingPlugin::host2MessagePipe, this,
                &PluginSystem::messagePipe);

        emit p->plugin2MessagePipe(WingPluginMessage::PluginLoaded, emptyparam);

      } else {
        loader.unload();
      }
    }
  }

  return true;
}

void PluginSystem::messagePipe(IWingPlugin *sender, WingPluginMessage type,
                               QList<QVariant> msg) {

  if (sender == nullptr)
    return;

  if (type == WingPluginMessage::PluginCall) {
    auto b = false;
    if (msg.count() > 0) {
      auto i = msg[0].toInt(&b);
      msg.pop_front(); // remove the first member : CallTableIndex

      if (b) {
        CallTableIndex index = CallTableIndex(i);
        emit PluginCall(index, msg);
      }
    }
    return;
  }
}
