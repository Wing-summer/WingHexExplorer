#include "pluginsystem.h"
#include "class/logger.h"
#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QPluginLoader>
#include <QtConcurrent>
#include <QtCore>

PluginSystem::PluginSystem(QObject *parent)
    : QObject(parent), curpluginctl(nullptr) {
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
  for (auto &item : loadedplgs) {
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
    Logger::info(tr("LoadingPlugin") + fileinfo.fileName());
    QList<WingPluginInfo> loadedplginfos;
    try {
      auto p = qobject_cast<IWingPlugin *>(loader.instance());
      if (p) {
        lp = LP::signature;
        if (p->signature() != WINGSUMMER) {
          Logger::critical(tr("ErrLoadPluginSign"));
          loader.unload();
          return;
        }
        lp = LP::sdkVersion;
        if (p->sdkVersion() != SDKVERSION) {
          Logger::critical(tr("ErrLoadPluginSDKVersion"));
          loader.unload();
          return;
        }
        lp = LP::pluginName;
        if (!p->pluginName().trimmed().length()) {
          Logger::critical(tr("ErrLoadPluginNoName"));
          loader.unload();
          return;
        }
        lp = LP::puid;
        auto puid = IWingPlugin::GetPUID(p);
        if (puid != p->puid()) {
          Logger::critical(tr("ErrLoadPluginPUID"));
          loader.unload();
          return;
        }

        if (loadedpuid.contains(puid)) {
          Logger::critical(tr("ErrLoadLoadedPlugin"));
          loader.unload();
          return;
        }

        lp = LP::plugin2MessagePipe;
        p->plugin2MessagePipe(WingPluginMessage::PluginLoading, emptyparam);

        if (!p->init(loadedplginfos)) {
          Logger::critical(tr("ErrLoadInitPlugin"));
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

        Logger::warning(tr("PluginName :") + info.pluginName);
        Logger::warning(tr("PluginAuthor :") + info.pluginAuthor);
        Logger::warning(tr("PluginWidgetRegister"));
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
        QHash<QDockWidget *, Qt::DockWidgetArea> dws;
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

        p->plugin2MessagePipe(WingPluginMessage::PluginLoaded, emptyparam);

      } else {
        Logger::critical(loader.errorString());
        loader.unload();
      }
    } catch (...) {
      auto m = QMetaEnum::fromType<LP>();
      Logger::critical(tr("ErrLoadPluginLoc") + m.valueToKey(int(lp)));
      loader.unload();
    }

    Logger::_log("");
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
  Logger::info(tr("FoundPluginCount") + QString::number(plgs.count()));

  for (auto &item : plgs) {
    loadPlugin(item);
  }

  Logger::info(tr("PluginLoadingFinished"));

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
        Logger::critical(tr("[PluginTimeout]") + plugin->pluginName() +
                         " --> " + oldctl->pluginName());
        initControl(plugin);
        oldctl->plugin2MessagePipe(
            WingPluginMessage::ConnectTimeout,
            QList<QVariant>({plugin->pluginName(), plugin->puid()}));
      } else {
        Logger::critical(tr("[PluginRequestError]") + plugin->pluginName());
        mutex.unlock();
        return false;
      }
    }
  } else {
    Logger::warning(tr("[PluginRequestSuccess]") + plugin->pluginName());
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
    connect(timer, &QTimer::timeout, this, [=] {
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
