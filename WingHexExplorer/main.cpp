#include "./dialog/mainwindow.h"
#include "class/appmanager.h"
#include "qBreakpad/QBreakpadHandler.h"
#include <DAboutDialog>
#include <DApplication>
#include <DApplicationSettings>
#include <DConfig>
#include <DFontSizeManager>
#include <DMessageBox>
#include <DProgressBar>
#include <DTitlebar>
#include <DWidgetUtil>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QLayout>
#include <QStandardPaths>
#include <QUrl>
#include <dfeaturedisplaydialog.h>

#include "define.h"

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

  auto a = DApplication::globalApplication(fakeArgc, fakeArgs.data());

  QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
  auto s = a->applicationDirPath() + "/lang/default.qm";
  QTranslator translator;
  if (!translator.load(s)) {
    DMessageBox::critical(nullptr, "Error", "Error Loading Translation File!",
                          DMessageBox::Ok);
    return -1;
  }
  a->installTranslator(&translator);

  a->setOrganizationName(QObject::tr("WingCloud"));
  a->setApplicationName(APPNAME);
  a->setApplicationVersion("1.5.5");
  a->setApplicationLicense("AGPL-3.0");
  a->setProductIcon(QIcon(":/images/icon.png"));
  a->setProductName(QObject::tr("WingHexExplorer"));
  a->setApplicationDescription(QObject::tr("AppDescription"));
  a->setVisibleMenuShortcutText(true);
  a->setApplicationHomePage("https://www.cnblogs.com/wingsummer/");
  a->setApplicationAcknowledgementPage("https://www.cnblogs.com/wingsummer/");

  a->loadTranslator();
  a->setApplicationDisplayName(QObject::tr("WingHexExplorer"));

  if (!a->setSingleInstance("com.Wingsummer.WingHexExplorer")) {
    return -1;
  }

  // 单例传参
  auto instance = DGuiApplicationHelper::instance();
  QObject::connect(instance, &DGuiApplicationHelper::newProcessInstance,
                   [=](qint64 pid, const QStringList &arguments) {
                     Q_UNUSED(pid);
                     AppManager::openFiles(arguments.mid(1));
                   });

  QStringList urls;

  {
    QCommandLineParser parser;
    parser.process(*a);

    for (auto &path : parser.positionalArguments()) {
      QFileInfo info(path);
      urls << info.absoluteFilePath();
    }
  }

  // 保存程序的窗口主题设置
  DApplicationSettings as;
  Q_UNUSED(as);

  MainWindow w;
  auto manager = AppManager::instance();
  manager->mWindow = &w;
  QDir dumpdir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
  dumpdir.mkdir("dump");
  auto &breakpad = QBreakpadInstance;
  breakpad.setDumpPath(dumpdir.absolutePath() + "/dump");
  QObject::connect(
      &breakpad, &QBreakpadHandler::appCrashed,
      [&w, manager](const QString &path) {
        auto logfile = w.saveLog();
        QMessageBox msg(manager->mWindow);
        msg.setIcon(QMessageBox::Icon::Critical);
        msg.setText(QObject::tr("AppCrashed"));
        msg.setInformativeText(QObject::tr("Issue2Author"));
        msg.setDetailedText(
            QString("%1:%2\n%3:%4")
                .arg(QObject::tr("dmpPath"), path, QObject::tr("logPath"),
                     logfile.isEmpty() ? QObject::tr("ExportFail") : logfile));
        msg.exec();
      });

  w.show();

  manager->openFiles(urls);
  Dtk::Widget::moveToCenter(&w);
  return a->exec();
}
