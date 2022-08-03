#include "./dialog/mainwindow.h"
#include "class/appmanager.h"
#include "winghexapplication.h"
#include <DApplication>
#include <DApplicationSettings>
#include <DFontSizeManager>
#include <DMessageBox>
#include <DProgressBar>
#include <DTitlebar>
#include <DWidgetUtil>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDate>
#include <QFileInfo>
#include <QLayout>
#include <QUrl>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[]) {

  //解决 root/ubuntu 主题样式走形
  qputenv("XDG_CURRENT_DESKTOP", "Deepin");
  QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  // 程序内强制添加 -platformtheme
  // deepin 参数喂给 Qt 让 Qt 正确使用 Deepin 主题修复各种奇怪样式问题
  QVector<char *> fakeArgs(argc + 2);
  char fa1[] = "-platformtheme";
  char fa2[] = "deepin";
  fakeArgs[0] = argv[0];
  fakeArgs[1] = fa1;
  fakeArgs[2] = fa2;

  for (int i = 1; i < argc; i++)
    fakeArgs[i + 2] = argv[i];
  int fakeArgc = argc + 2;

  WingHexApplication a(fakeArgc, fakeArgs.data());
  QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);

  auto s = a.applicationDirPath() + "/lang/default.qm";
  QTranslator translator;
  if (!translator.load(s)) {
    DMessageBox::critical(nullptr, "Error", "Error Loading Translation File!",
                          DMessageBox::Ok);
    return -1;
  }
  a.installTranslator(&translator);

  a.setOrganizationName("WingCloud");
  a.setApplicationName(QObject::tr("WingHexExplorer"));
  a.setApplicationVersion("1.4.6");
  a.setProductIcon(QIcon(":/images/icon.png"));
  a.setProductName(QObject::tr("WingHexExplorer"));
  a.setApplicationDescription(QObject::tr("AppDescription"));
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
    manager->mWindow = &w;
    manager->openFiles(urls);
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
