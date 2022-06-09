#include "pluginsystem.h"
#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QPluginLoader>
#include <QtCore>

PluginSystem::PluginSystem(QObject *parent) : QObject(parent) {
  logger = Logger::getInstance();

  // init plugin dispathcer
#define InitDispathcer(hookindex)                                              \
  dispatcher.insert(hookindex, QList<IWingPlugin *>());

  InitDispathcer(HookIndex::OpenFileBegin);
  InitDispathcer(HookIndex::OpenFileEnd);
  InitDispathcer(HookIndex::OpenDriverBegin);
  InitDispathcer(HookIndex::OpenDriverEnd);
  InitDispathcer(HookIndex::CloseFileBegin);
  InitDispathcer(HookIndex::CloseFileEnd);
  InitDispathcer(HookIndex::NewFileBegin);
  InitDispathcer(HookIndex::NewFileEnd);
}

PluginSystem::~PluginSystem() {
  for (auto item : loadedplgs) {
    item->unload();
    item->deleteLater();
  }
}

QList<IWingPlugin *> PluginSystem::plugins() { return loadedplgs; }

void PluginSystem::raiseDispatch(HookIndex hookindex, QList<QVariant> params) {
  auto dispatch = dispatcher[hookindex];
  for (auto item : dispatch) {
    item->plugin2MessagePipe(WingPluginMessage::HookMessage, params);
  }
}

bool PluginSystem::LoadPlugin() {
  QDir plugindir(QCoreApplication::applicationDirPath() + "/plugin");
  plugindir.setNameFilters(QStringList("*.wingplg"));
  auto plgs = plugindir.entryInfoList();
  logger->logMessage(
      INFOLOG(tr("FoundPluginCount") + QString::number(plgs.count())));
  for (auto item : plgs) {
    if (item.exists()) {
      QPluginLoader loader(item.absoluteFilePath());
      logger->logMessage(
          INFOLOG(QString(">> ") + tr("LoadingPlugin") + item.fileName()));
      auto p = qobject_cast<IWingPlugin *>(loader.instance());
      if (p) {
        if (p->signature() != sign) {
          logger->logMessage(ERRLOG(tr("ErrLoadPluginSign")));
          loader.unload();
        }
        auto puid = PluginUtils::GetPUID(p);
        if (puid != p->puid()) {
          logger->logMessage(ERRLOG(tr("ErrLoadPluginPUID")));
          loader.unload();
        }
        p->self = p;

        emit p->plugin2MessagePipe(WingPluginMessage::PluginLoading,
                                   emptyparam);

        p->init(loadedplgs);

        loadedplgs.push_back(p);

        logger->logMessage(WARNLOG(tr("PluginWidgetRegister")));
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
        logger->logMessage(ERRLOG(loader.errorString()));
        loader.unload();
      }
    }
  }

  return true;
}

void PluginSystem::messagePipe(IWingPlugin *sender, WingPluginMessage type,
                               QList<QVariant> msg) {
  Q_UNUSED(msg)

  if (sender == nullptr)
    return;

  if (type == WingPluginMessage::GetHexViewShadow) {
    HexViewShadow *hvs;
    if (hexshadows.contains(sender)) {
      hvs = hexshadows[sender];
    } else {
      hvs = new HexViewShadow(this);
      hexshadows.insert(sender, hvs);
      emit this->ConnectShadow(hvs);
    }
    sender->plugin2MessagePipe(
        WingPluginMessage::GetHexViewShadow,
        QList<QVariant>({QVariant::fromValue(static_cast<void *>(hvs))}));
  }
}

void PluginSystem::shadowDestory(IWingPlugin *plugin) {}
bool PluginSystem::shadowControl(IWingPlugin *plugin, HexViewShadow *shadow) {}
bool PluginSystem::shadowIsValid(IWingPlugin *plugin) {}
bool PluginSystem::shadowRelease(IWingPlugin *plugin, HexViewShadow *shadow) {}
