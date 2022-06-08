#include "testplugin.h"
#include <QMessageBox>
#include <QStringList>

TestPlugin::TestPlugin(QObject *parent){Q_UNUSED(parent)}

TestPlugin::~TestPlugin() {}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(TestPlugin, GenericPlugin)
#endif // QT_VERSION < 0x050000

bool TestPlugin::init(QList<IWingPlugin *> loadedplugins) {
  QString ps;
  for (auto item : loadedplugins) {
    ps.append(item->pluginName());
    ps.append('\n');
  }
  QMessageBox::information(nullptr, "Test", ps);
  testmenu = new QMenu;
  testmenu->setTitle("TestPlugin");
  testmenu->addAction("Hello!");
  return true;
}

void TestPlugin::unload() { testmenu->deleteLater(); }

QString TestPlugin::puid() { return PluginUtils::GetPUID(this); }

QString TestPlugin::pluginName() { return "TestPlugin"; }

QString TestPlugin::pluginAuthor() { return "Wingsummer"; }

QString TestPlugin::comment() {
  return "A Sample Plugin for WingHex Explorer by Wingsummer!";
}

uint TestPlugin::pluginVersion() { return 1; }

QString TestPlugin::signature() { return sign; }

QList<QVariant> TestPlugin::optionalInfos() { return QList<QVariant>(); }

void TestPlugin::plugin2MessagePipe(WingPluginMessage type,
                                    QList<QVariant> msg) {
  Q_UNUSED(msg)
  if (type == WingPluginMessage::PluginLoaded) {
    emit host2MessagePipe(this, WingPluginMessage::PluginCall,
                          QList<QVariant>{int(CallTableIndex::NewFile)});
  }
}

QMenu *TestPlugin::registerMenu() { return testmenu; }

QDockWidget *TestPlugin::registerDockWidget() { return nullptr; }

Qt::DockWidgetArea TestPlugin::registerDockWidgetDockArea() {
  return Qt::DockWidgetArea::NoDockWidgetArea;
}

HookIndex TestPlugin::getHookSubscribe() { return HookIndex::None; }
