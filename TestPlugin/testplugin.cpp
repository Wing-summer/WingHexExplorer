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
  testmenu = new QMenu;
  testmenu->setTitle("TestPlugin");
  auto action = new QAction("ClickMe!", this);
  connect(action, &QAction::triggered, [=] {
    QMessageBox::information(nullptr, "戳我", "经典：Hello World!");
  });
  testmenu->addAction(action);

  dw = new QDockWidget;
  auto l = new QLabel("Hello World!", dw);
  dw->setWidget(l);
  dw->setObjectName("testplg");

  return true;
}

void TestPlugin::unload() {
  testmenu->deleteLater();
  dw->deleteLater();
}

QString TestPlugin::puid() { return PluginUtils::GetPUID(this); }

QString TestPlugin::pluginName() { return "TestPlugin"; }

QString TestPlugin::pluginAuthor() { return "Wingsummer"; }

QString TestPlugin::comment() {
  return "A Sample Plugin for WingHex Explorer by Wingsummer!";
}

uint TestPlugin::pluginVersion() { return 1; }

QString TestPlugin::signature() { return WINGSUMMER; }

QList<QVariant> TestPlugin::optionalInfos() { return QList<QVariant>(); }

void TestPlugin::plugin2MessagePipe(WingPluginMessage type,
                                    QList<QVariant> msg) {
  Q_UNUSED(msg)
  if (type == WingPluginMessage::PluginLoaded) {
    emit host2MessagePipe(this, WingPluginMessage::GetHexViewShadow,
                          QList<QVariant>());
    return;
  }
  if (type == WingPluginMessage::GetHexViewShadow) {
    auto hvs = extractHexViewShadow(msg);
    if (hvs) {
      if (hvs->shadowControl(this)) {
        hvs->newFile();
        hvs->switchDocument(0);
        auto str = QString("HelloWorld!").toUtf8();
        hvs->insert(0, str);
        hvs->shadowRelease(this);
      }
    }
  }
}

QMenu *TestPlugin::registerMenu() { return testmenu; }

QDockWidget *TestPlugin::registerDockWidget() { return dw; }

Qt::DockWidgetArea TestPlugin::registerDockWidgetDockArea() {
  return Qt::DockWidgetArea::LeftDockWidgetArea;
}

HookIndex TestPlugin::getHookSubscribe() { return HookIndex::None; }
