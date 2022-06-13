#include "pluginsystem.h"
#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QPluginLoader>
#include <QtCore>

PluginSystem::PluginSystem(QObject *parent)
    : QObject(parent), curhexshadow(nullptr) {
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

void PluginSystem::loadPlugin(QFileInfo fileinfo) {
  if (fileinfo.exists()) {
    QPluginLoader loader(fileinfo.absoluteFilePath());
    logger->logMessage(
        INFOLOG(QString(">> ") + tr("LoadingPlugin") + fileinfo.fileName()));
    auto p = qobject_cast<IWingPlugin *>(loader.instance());
    if (p) {
      if (p->signature() != WINGSUMMER) {
        logger->logMessage(ERRLOG(tr("ErrLoadPluginSign")));
        loader.unload();
      }
      auto puid = PluginUtils::GetPUID(p);
      if (puid != p->puid()) {
        logger->logMessage(ERRLOG(tr("ErrLoadPluginPUID")));
        loader.unload();
      }
      p->self = p;

      emit p->plugin2MessagePipe(WingPluginMessage::PluginLoading, emptyparam);

      p->init(loadedplgs);

      loadedplgs.push_back(p);

      logger->logMessage(WARNLOG(tr("PluginWidgetRegister")));
      auto menu = p->registerMenu();
      if (menu) {
        emit this->PluginMenuNeedAdd(menu);
      }

      auto dockw = p->registerDockWidget();
      if (dockw) {
        emit this->PluginDockWidgetAdd(dockw, p->registerDockWidgetDockArea());
      }

      connect(p, &IWingPlugin::host2MessagePipe, this,
              &PluginSystem::messagePipe);

      auto hvs = new HexViewShadow(this);
      hexshadows.insert(p, hvs);
      emit ConnectShadow(hvs);
      emit p->plugin2MessagePipe(WingPluginMessage::PluginLoaded, emptyparam);

    } else {
      logger->logMessage(ERRLOG(loader.errorString()));
      loader.unload();
    }
  }
}

bool PluginSystem::LoadPlugin() {
#ifdef QT_DEBUG
  QDir plugindir("/home/wingsummer/QT Project/build-TestPlugin-unknown-Debug");
  //这是我的插件调试目录，如果调试插件，请更换路径

  plugindir.setNameFilters(QStringList("*.so"));
#else
  QDir plugindir(QCoreApplication::applicationDirPath() + "/plugin");
  plugindir.setNameFilters(QStringList("*.wingplg"));
#endif
  auto plgs = plugindir.entryInfoList();
  logger->logMessage(
      INFOLOG(tr("FoundPluginCount") + QString::number(plgs.count())));
  for (auto item : plgs) {
    loadPlugin(item);
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
    sender->plugin2MessagePipe(WingPluginMessage::GetHexViewShadow,
                               QList<QVariant>({QVariant::fromValue(hvs)}));
  }
}

void PluginSystem::shadowDestory(IWingPlugin *plugin) {
  if (shadowRelease(plugin)) {
    auto shv = hexshadows[plugin];
    hexshadowtimeout.remove(shv);
    hexshadowtimer.remove(shv);
    shv->deleteLater();
  }
}
bool PluginSystem::shadowControl(IWingPlugin *plugin) {
  if (plugin == nullptr)
    return false;
  auto res = mutex.tryLock(1500);
  if (!res)
    return false;
  if (hexshadows.contains(plugin)) {
    if (curhexshadow) {
      if (hexshadowtimeout[curhexshadow]) {
        plugin->plugin2MessagePipe(
            WingPluginMessage::HexViewShadowTimeout,
            QList<QVariant>({plugin->pluginName(), plugin->puid()}));
      }
    }
    initShadowControl(plugin);
    return true;
  } else {
    return false;
  }
  mutex.unlock();
}

bool PluginSystem::shadowIsValid(IWingPlugin *plugin) {
  if (plugin == nullptr)
    return false;
  if (hexshadows.contains(plugin)) {
    return curhexshadow == hexshadows[plugin];
  }
  return false;
}
bool PluginSystem::shadowRelease(IWingPlugin *plugin) {
  if (plugin == nullptr)
    return false;
  if (hexshadows.contains(plugin)) {
    auto shadow = hexshadows[plugin];
    shadow->disconnect();
    hexshadowtimer[shadow]->stop();
    hexshadowtimeout[shadow] = false;
    curhexshadow = nullptr;
  }
  return true;
}

void PluginSystem::initShadowControl(IWingPlugin *plugin) {
  if (!hexshadows.contains(plugin))
    return;
  auto shadow = hexshadows[plugin];
  if (!hexshadowtimer.contains(shadow)) {
    auto timer = new QTimer(this);
    hexshadowtimer.insert(shadow, timer);
    connect(timer, &QTimer::timeout, [=] {
      hexshadowtimeout[shadow] = true;
      timer->stop();
    });
  }
  if (hexshadowtimeout.contains(shadow)) {
    hexshadowtimeout[shadow] = false;
  } else {
    hexshadowtimeout.insert(shadow, false);
  }
  if (curhexshadow)
    shadowRelease(plugin);
  emit ConnectShadowSlot(shadow);
  curhexshadow = shadow;
  hexshadowtimer[shadow]->start(5000);
}
