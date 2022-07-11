#include "badplugin.h"
#include "QMessageBox"

BadPlugin::BadPlugin(QObject *parent) { Q_UNUSED(parent) }

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(BadTestPlugin, BadPlugin)
#endif // QT_VERSION < 0x050000

int BadPlugin::sdkVersion() { return SDKVERSION; }

bool BadPlugin::init(QList<WingPluginInfo> loadedplugin) {
  Q_UNUSED(loadedplugin);
  return true;
}

BadPlugin::~BadPlugin() {}

void BadPlugin::unload() {}

QString BadPlugin::pluginName() { return "BadPlugin"; }

QString BadPlugin::pluginAuthor() { return "wingsummer"; }

uint BadPlugin::pluginVersion() { return 1; }

QString BadPlugin::signature() { return WINGSUMMER; }

QString BadPlugin::pluginComment() {
  return "This is a bad plugin , hia, hia~";
}

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
