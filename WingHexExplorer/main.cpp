#include "./dialog/mainwindow.h"
#include <DApplication>
#include <DApplicationSettings>
#include <DFontSizeManager>
#include <DProgressBar>
#include <DTitlebar>
#include <DWidgetUtil>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDate>
#include <QLayout>
#include <QUrl>
#include <appmanager.h>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[]) {
  QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
  DApplication a(argc, argv);
  QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
  a.setOrganizationName("WingCloud");
  a.setApplicationName(QT_TRANSLATE_NOOP("main", "WingHexExplorer"));
  a.setApplicationVersion("1.0");
  a.setProductIcon(QIcon(":/images/icon.png"));
  a.setProductName(QT_TRANSLATE_NOOP("main", "WingHexExplorer"));
  a.setApplicationDescription(QT_TRANSLATE_NOOP("main", "AppDescription"));
  a.setVisibleMenuShortcutText(true);

  a.loadTranslator();
  a.setApplicationDisplayName("WingHexExplorer");

  QDBusConnection dbus = QDBusConnection::sessionBus();
  QString service("com.Wingsummer.WingHexExplorer");
  QString com("/com/Wingsummer/WingHexExplorer");

  QCommandLineParser parser;
  parser.process(a);

  QStringList urls;
  QStringList arguments = parser.positionalArguments();

  for (const QString &path : arguments) {
    QFileInfo info(path);
    urls << info.absoluteFilePath();
  }

  // 参考：Deepin 自带的文本编辑器的单例通信方式
  // Start editor process if not found any editor use DBus.
  if (dbus.registerService(service)) {

    // 保存程序的窗口主题设置
    DApplicationSettings as;
    Q_UNUSED(as)

    auto manager = AppManager::instance();

    MainWindow w;

    for (auto item : urls) {
      w.openFile(item);
    }

    manager->mWindow = &w;

    w.show();

    dbus.registerObject(com, manager, QDBusConnection::ExportScriptableSlots);
    Dtk::Widget::moveToCenter(&w);
    return a.exec();
  }
  // Just send dbus message to exist editor process.
  else {
    QDBusInterface notification(service, com, service,
                                QDBusConnection::sessionBus());

    QList<QVariant> args;
    args << urls;
    notification.callWithArgumentList(QDBus::AutoDetect, "openFiles", args);
    return 0;
  }
}
