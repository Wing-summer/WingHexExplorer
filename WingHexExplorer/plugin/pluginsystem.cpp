#include "pluginsystem.h"
#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QPluginLoader>
#include <QtConcurrent>
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
  InitDispathcer(HookIndex::DocumentSwitched);
}

PluginSystem::~PluginSystem() {
  for (auto item : loadedplgs) {
    item->plugin2MessagePipe(WingPluginMessage::PluginUnLoading, emptyparam);
    item->controller.disconnect();
    item->reader.disconnect();
    item->plugin2MessagePipe(WingPluginMessage::PluginUnLoaded, emptyparam);
    item->unload();
    item->deleteLater();
  }
}

QList<IWingPlugin *> PluginSystem::plugins() { return loadedplgs; }

void PluginSystem::raiseDispatch(HookIndex hookindex, QList<QVariant> params) {
  auto dispatch = dispatcher[hookindex];
  params.push_front(hookindex);
  for (auto item : dispatch) {
    item->plugin2MessagePipe(WingPluginMessage::HookMessage, params);
  }
}

void PluginSystem::loadPlugin(QFileInfo fileinfo) {
  LP lp(LP::begin);

  if (fileinfo.exists()) {
    QPluginLoader loader(fileinfo.absoluteFilePath());
    logger->logMessage(
        INFOLOG(QString(">> ") + tr("LoadingPlugin") + fileinfo.fileName()));
    QList<WingPluginInfo> loadedplginfos;
    try {
      auto p = qobject_cast<IWingPlugin *>(loader.instance());
      if (p) {
        lp = LP::signature;
        if (p->signature() != WINGSUMMER) {
          logger->logMessage(ERRLOG(tr("ErrLoadPluginSign")));
          loader.unload();
          return;
        }
        lp = LP::sdkVersion;
        if (p->sdkVersion() != SDKVERSION) {
          logger->logMessage(ERRLOG(tr("ErrLoadPluginSDKVersion")));
          loader.unload();
          return;
        }
        lp = LP::pluginName;
        if (!p->pluginName().trimmed().length()) {
          logger->logMessage(ERRLOG(tr("ErrLoadPluginNoName")));
          loader.unload();
          return;
        }
        lp = LP::puid;
        auto puid = IWingPlugin::GetPUID(p);
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

        lp = LP::plugin2MessagePipe;
        emit p->plugin2MessagePipe(WingPluginMessage::PluginLoading,
                                   emptyparam);

        if (!p->init(loadedplginfos)) {
          logger->logMessage(ERRLOG(tr("ErrLoadInitPlugin")));
          loader.unload();
          return;
        }

        WingPluginInfo info;
        info.puid = p->puid();
        info.pluginName = p->pluginName();
        info.pluginAuthor = p->pluginAuthor();
        info.pluginComment = p->pluginComment();
        info.pluginVersion = p->pluginVersion();

        loadedplginfos.push_back(info);
        loadedplgs.push_back(p);
        loadedpuid << puid;

        logger->logMessage(WARNLOG(tr("PluginWidgetRegister")));
        lp = LP::registerMenu;
        auto menu = p->registerMenu();
        if (menu) {
          emit this->PluginMenuNeedAdd(menu);
        }
        lp = LP::registerTool;
        auto tb = p->registerToolBar();
        if (tb) {
          emit this->PluginToolBarAdd(tb, p->registerToolBarArea());
        } else {
          auto tbtn = p->registerToolButton();
          if (tbtn) {
            emit this->PluginToolButtonAdd(tbtn);
          }
        }

        lp = LP::registerDockWidget;
        QMap<QDockWidget *, Qt::DockWidgetArea> dws;
        p->registerDockWidget(dws);
        if (dws.count()) {
          emit this->PluginDockWidgetAdd(p->pluginName(), dws);
        }

        emit ConnectBase(p);

        lp = LP::getHookSubscribe;
        auto sub = p->getHookSubscribe();

#define INSERTSUBSCRIBE(HOOK)                                                  \
  if (sub & HOOK)                                                              \
    dispatcher[HOOK].push_back(p);

        INSERTSUBSCRIBE(HookIndex::OpenFileBegin);
        INSERTSUBSCRIBE(HookIndex::OpenFileEnd);
        INSERTSUBSCRIBE(HookIndex::OpenDriverBegin);
        INSERTSUBSCRIBE(HookIndex::OpenDriverEnd);
        INSERTSUBSCRIBE(HookIndex::CloseFileBegin);
        INSERTSUBSCRIBE(HookIndex::CloseFileEnd);
        INSERTSUBSCRIBE(HookIndex::NewFileBegin);
        INSERTSUBSCRIBE(HookIndex::NewFileEnd);
        INSERTSUBSCRIBE(HookIndex::DocumentSwitched);

        emit p->plugin2MessagePipe(WingPluginMessage::PluginLoaded, emptyparam);

      } else {
        logger->logMessage(ERRLOG(loader.errorString()));
        loader.unload();
      }
    } catch (...) {
      auto m = QMetaEnum::fromType<LP>();
      logger->logMessage(
          ERRLOG(QString(tr("ErrLoadPluginLoc") + m.valueToKey(int(lp)))));
      loader.unload();
    }

    logger->logMessage("<br />");
  }
}

bool PluginSystem::LoadPlugin() {
#ifdef QT_DEBUG
  QDir plugindir(QCoreApplication::applicationDirPath() + "/plugin");
  //这是我的插件调试目录，如果调试插件，请更换路径

  plugindir.setNameFilters(QStringList("*.so"));
#else
  QDir plugindir(QCoreApplication::applicationDirPath() + "/plugin");
  plugindir.setNameFilters(QStringList("*.wingplg"));
#endif
  auto plgs = plugindir.entryInfoList();
  logger->logMessage(
      INFOLOG(tr("FoundPluginCount") + QString::number(plgs.count())) +
      "<br />");
  for (auto item : plgs) {
    loadPlugin(item);
  }

  logger->logMessage(INFOLOG(tr("PluginLoadingFinished")) + "<br />");

  return true;
}

bool PluginSystem::requestControl(IWingPlugin *plugin, int timeout) {
  if (plugin == nullptr)
    return false;
  auto res = mutex.tryLock(timeout);
  if (!res) {
    mutex.unlock();
    return false;
  }

  auto oldctl = curpluginctl;
  if (oldctl) {
    if (oldctl == plugin) {
      resetTimeout(plugin);
      mutex.unlock();
      return true;
    } else {
      if (plugintimeout[oldctl]) {
        logger->logMessage(WARNLOG(tr("[PluginTimeout]") +
                                   plugin->pluginName() + " --> " +
                                   oldctl->pluginName()));
        initControl(plugin);
        oldctl->plugin2MessagePipe(
            WingPluginMessage::ConnectTimeout,
            QList<QVariant>({plugin->pluginName(), plugin->puid()}));
      } else {
        logger->logMessage(
            ERRLOG(tr("[PluginRequestError]") + plugin->pluginName()));
        mutex.unlock();
        return false;
      }
    }
  } else {
    logger->logMessage(
        WARNLOG(tr("[PluginRequestSuccess]") + plugin->pluginName()));
    initControl(plugin);
  }
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
    mutex.unlock();
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

IWingPlugin *PluginSystem::currentControlPlugin() { return curpluginctl; }

bool PluginSystem::currentControlTimeout() {
  if (hasControl())
    return plugintimeout[curpluginctl];
  return true;
}
