#include "badplugin.h"
#include "QMessageBox"

BadPlugin::BadPlugin(QObject *parent) { Q_UNUSED(parent) }

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(BadTestPlugin, BadPlugin)
#endif // QT_VERSION < 0x050000

bool BadPlugin::init(QList<WingPluginInfo> loadedplugin) {
  Q_UNUSED(loadedplugin);
  return true;
}

BadPlugin::~BadPlugin() {}

void BadPlugin::unload() {}

QMenu *BadPlugin::registerMenu() { return nullptr; }

QDockWidget *BadPlugin::registerDockWidget() { return nullptr; }

QToolButton *BadPlugin::registerToolButton() { return nullptr; }

QString BadPlugin::pluginName() { return "BadPlugin"; }

QString BadPlugin::pluginAuthor() { return "wingsummer"; }

uint BadPlugin::pluginVersion() { return 1; }

QString BadPlugin::puid() { return PluginUtils::GetPUID(this); }

QString BadPlugin::signature() { return WINGSUMMER; }

QString BadPlugin::pluginComment() {
  return "This is a bad plugin , hia, hia~";
}

QList<QVariant> BadPlugin::optionalInfos() { return QList<QVariant>(); }

Qt::DockWidgetArea BadPlugin::registerDockWidgetDockArea() {
  return Qt::DockWidgetArea::NoDockWidgetArea;
}

HookIndex BadPlugin::getHookSubscribe() { return HookIndex::None; }

void BadPlugin::plugin2MessagePipe(WingPluginMessage type,
                                   QList<QVariant> msg) {
  if (type == WingPluginMessage::PluginLoaded) {
    requestControl();
    return;
  }
  if (type == WingPluginMessage::ConnectTimeout) {
    QMessageBox::information(getParentWindow(), "Opps",
                             QString("WTF : %1").arg(msg[0].toString()));
    return;
  }
}
