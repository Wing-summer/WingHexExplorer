#include "pluginsystem.h"
#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QPluginLoader>
#include <QtCore>

PluginSystem::PluginSystem(QObject *parent)
    : QObject(parent), curpluginctl(nullptr) {
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
        return;
      }
      if (!p->pluginName().trimmed().length()) {
        logger->logMessage(ERRLOG(tr("ErrLoadPluginNoName")));
        loader.unload();
        return;
      }
      auto puid = PluginUtils::GetPUID(p);
      if (puid != p->puid()) {
        logger->logMessage(ERRLOG(tr("ErrLoadPluginPUID")));
        loader.unload();
        return;
      }
      if (loadedpuid.contains(puid)) {
        logger->logMessage(ERRLOG(tr("ErrLoadLoadedPlugin")));
        loader.unload();
        return;
      }

      emit p->plugin2MessagePipe(WingPluginMessage::PluginLoading, emptyparam);

      p->init(loadedplgs);

      loadedplgs.push_back(p);
      loadedpuid << puid;

      logger->logMessage(WARNLOG(tr("PluginWidgetRegister")));
      auto menu = p->registerMenu();
      if (menu) {
        emit this->PluginMenuNeedAdd(menu);
      }

      auto dockw = p->registerDockWidget();
      if (dockw) {
        emit this->PluginDockWidgetAdd(dockw, p->registerDockWidgetDockArea());
      }

      emit ConnectBase(p);
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

bool PluginSystem::requestControl(IWingPlugin *plugin, int timeout) {
  if (plugin == nullptr)
    return false;
  auto res = mutex.tryLock(timeout);
  if (!res)
    return false;

  if (curpluginctl) {
    if (plugintimeout[curpluginctl]) {
      plugin->plugin2MessagePipe(
          WingPluginMessage::ConnectTimeout,
          QList<QVariant>({plugin->pluginName(), plugin->puid()}));
    }
  }
  initControl(plugin);
  mutex.unlock();
  return true;
}

bool PluginSystem::requestRelease(IWingPlugin *plugin) {
  if (plugin == nullptr)
    return false;
  if (mutex.tryLock(1500)) {
    plugin->controller.disconnect();
    plugintimer[plugin]->stop();
    plugintimeout[plugin] = false;
    curpluginctl = nullptr;
    return true;
  }
  return false;
}

void PluginSystem::resetTimeout(IWingPlugin *plugin) {
  if (plugin == nullptr)
    return;

  plugintimer[plugin]->start(1000);
  plugintimeout[plugin] = false;
}

void PluginSystem::initControl(IWingPlugin *plugin) {
  if (!plugintimer.contains(plugin)) {
    auto timer = new QTimer(this);
    plugintimer.insert(plugin, timer);
    connect(timer, &QTimer::timeout, [=] {
      plugintimeout[plugin] = true;
      timer->stop();
    });
  }
  if (plugintimeout.contains(plugin)) {
    plugintimeout[plugin] = false;
  } else {
    plugintimeout.insert(plugin, false);
  }
  if (curpluginctl)
    requestRelease(plugin);
  emit ConnectControl(plugin);
  curpluginctl = plugin;
  plugintimer[plugin]->start(1000);
}

bool PluginSystem::hasControl() { return curpluginctl != nullptr; }
