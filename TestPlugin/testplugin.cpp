#include "testplugin.h"
#include <QLabel>
#include <QMessageBox>
#include <QStringList>

TestPlugin::TestPlugin(QObject *parent){Q_UNUSED(parent)}

TestPlugin::~TestPlugin() {}

bool TestPlugin::init(QList<IWingPlugin *> loadedplugins) {
  if (loadedplugins.length() > 0) {
    QString ps;
    for (auto item : loadedplugins) {
      ps.append(item->pluginName());
      ps.append('\n');
    }
    QMessageBox::information(nullptr, "Test", ps);
  }

  PluginMenuInitBegin(testmenu, "TestPlugin") {
    PluginMenuAddItemLamba(testmenu, "ClickMe!", [=] {
      QMessageBox::information(nullptr, "戳我", "经典：Hello World!");
    });
  }
  PluginMenuInitEnd();

  PluginDockWidgetInit(dw, new QLabel("Hello World!", dw), "testplg",
                       "testplg");

  return true;
}

void TestPlugin::unload() {
  PluginWidgetFree(dw);
  PluginWidgetFree(testmenu);
}

QString TestPlugin::puid() { return PluginUtils::GetPUID(this); }

QString TestPlugin::pluginName() { return "TestPlugin"; }

QString TestPlugin::pluginAuthor() { return "Wingsummer"; }

QString TestPlugin::pluginComment() {
  return "A Sample Plugin for WingHex Explorer by Wingsummer!";
}

uint TestPlugin::pluginVersion() { return 1; }

QString TestPlugin::signature() { return WINGSUMMER; }

QList<QVariant> TestPlugin::optionalInfos() { return QList<QVariant>(); }

void TestPlugin::plugin2MessagePipe(WingPluginMessage type,
                                    QList<QVariant> msg) {
  Q_UNUSED(msg)
  if (type == WingPluginMessage::PluginLoaded) {
    if (shadowControl(this)) {
      newFile();
      switchDocument(0);
      auto str = QString("HelloWorld!").toUtf8();
      insert(0, str);
      shadowRelease(this);
    }
  }
}

QMenu *TestPlugin::registerMenu() { return testmenu; }

QDockWidget *TestPlugin::registerDockWidget() { return dw; }

Qt::DockWidgetArea TestPlugin::registerDockWidgetDockArea() {
  return Qt::DockWidgetArea::LeftDockWidgetArea;
}

HookIndex TestPlugin::getHookSubscribe() { return HookIndex::None; }
