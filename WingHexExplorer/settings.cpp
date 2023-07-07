#include "settings.h"
#include "utilities.h"
#include <DMessageManager>
#include <DSettings>
#include <DSettingsGroup>
#include <DSettingsOption>
#include <DSettingsWidgetFactory>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFontDatabase>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTextCodec>

#include "define.h"

Settings *Settings::s_pSetting = nullptr;
Settings::Settings(QWidget *parent) : QObject(parent) {
  QString strConfigPath =
      QString("%1/%2/%3/config.conf")
          .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
               ORGNAME, APPNAME);

  m_backend = new QSettingBackend(strConfigPath);

  settings = DSettings::fromJsonFile(":/resources/settings.json");
  settings->setBackend(m_backend);

#define BindConfigSignal(Var, SettingName, Signal)                             \
  auto Var = settings->option(SettingName);                                    \
  connect(Var, &Dtk::Core::DSettingsOption::valueChanged,                      \
          this, [=](QVariant value) Signal);

  BindConfigSignal(fontFamliy, "appearance.font.family",
                   { emit sigAdjustFont(value.toString()); });
  BindConfigSignal(plugin, "editor.plugin.enableplugin", {
    emit sigChangePluginEnabled(value.toBool());
    DMessageManager::instance()->sendMessage(
        m_pSettingsDialog, ICONRES("setting"), tr("RestartTakeEffect"));
  });
  BindConfigSignal(rootplugin, "editor.plugin.rootenableplugin", {
    auto v = value.toBool();
    if (v) {
      DMessageManager::instance()->sendMessage(
          m_pSettingsDialog, ICONRES("setting"), tr("EnabledRootPlugin"));
    }
    emit sigChangeRootPluginEnabled(v);
  });

  BindConfigSignal(hexfontSize, "editor.basic.size",
                   { emit sigAdjustEditorFontSize(value.toInt()); });
  BindConfigSignal(infofontSize, "appearance.font.size",
                   { emit sigAdjustInfoFontSize(value.toInt()); });
  BindConfigSignal(showAddr, "editor.basic.showaddr", {
    emit sigShowAddressNumber(value.toBool());
    DMessageManager::instance()->sendMessage(
        m_pSettingsDialog, ICONRES("setting"), tr("OpenNextTakeEffect"));
  });
  BindConfigSignal(showCol, "editor.basic.showcol", {
    emit sigShowColNumber(value.toBool());
    DMessageManager::instance()->sendMessage(
        m_pSettingsDialog, ICONRES("setting"), tr("OpenNextTakeEffect"));
  });
  BindConfigSignal(showText, "editor.basic.showtext", {
    emit sigShowEncodingText(value.toBool());
    DMessageManager::instance()->sendMessage(
        m_pSettingsDialog, ICONRES("setting"), tr("OpenNextTakeEffect"));
  });
  BindConfigSignal(encoding, "editor.basic.encoding", {
    emit sigChangedEncoding(value.toString());
    DMessageManager::instance()->sendMessage(
        m_pSettingsDialog, ICONRES("setting"), tr("OpenNextTakeEffect"));
  });
  BindConfigSignal(fmax, "editor.basic.findmaxcount", {
    emit sigAdjustFindMaxCount(value.toInt());
    DMessageManager::instance()->sendMessage(
        m_pSettingsDialog, ICONRES("setting"), tr("FindNextTakeEffect"));
  });
  BindConfigSignal(cplim, "editor.basic.copylimit", {
    emit sigAdjustCopyLimit(value.toInt());
    DMessageManager::instance()->sendMessage(
        m_pSettingsDialog, ICONRES("setting"), tr("CopyNextTakeEffect"));
  });
  BindConfigSignal(decstrlim, "editor.basic.decstrlimit", {
    emit sigAdjustDecodeStringLimit(value.toInt());
    DMessageManager::instance()->sendMessage(
        m_pSettingsDialog, ICONRES("setting"), tr("SelectionNextTakeEffect"));
  });

  auto enCoding = settings->option("editor.basic.encoding");
  QMap<QString, QVariant> encodingMap;
  QStringList encodings = Utilities::getEncodings();
  encodingMap.insert("keys", encodings);
  encodingMap.insert("values", encodings);
  encoding->setData("items", encodingMap);

  // only used by new window
  auto windowState = settings->option("appearance.window.windowsize");
  QMap<QString, QVariant> windowStateMap;
  windowStateMap.insert("keys", QStringList() << "window_normal"
                                              << "window_maximum"
                                              << "window_minimum"
                                              << "fullscreen");
  windowStateMap.insert("values", QStringList()
                                      << tr("Normal") << tr("Maximum")
                                      << tr("Minimum") << tr("Fullscreen"));
  windowState->setData("items", windowStateMap);
}

Settings::~Settings() {}

void Settings::setSettingDialog(DSettingsDialog *settingsDialog) {
  m_pSettingsDialog = settingsDialog;
}

// This function is workaround, it will remove after DTK fixed SettingDialog
// theme bug.
void Settings::dtkThemeWorkaround(QWidget *parent, const QString &theme) {
  parent->setStyle(QStyleFactory::create(theme));

  for (auto obj : parent->children()) {
    auto w = qobject_cast<QWidget *>(obj);
    if (!w) {
      continue;
    }

    dtkThemeWorkaround(w, theme);
  }
}

QPair<QWidget *, QWidget *> Settings::createFontComBoBoxHandle(QObject *obj) {
  auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);

  QComboBox *comboBox = new QComboBox;
  QPair<QWidget *, QWidget *> optionWidget =
      DSettingsWidgetFactory::createStandardItem(QByteArray(), option,
                                                 comboBox);

  QFontDatabase fontDatabase;
  comboBox->addItems(fontDatabase.families());

  if (option->value().toString().isEmpty()) {
    option->setValue(
        QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
  }

  // init.
  comboBox->setCurrentText(option->value().toString());

  connect(option, &DSettingsOption::valueChanged, comboBox,
          [=](QVariant var) { comboBox->setCurrentText(var.toString()); });

  option->connect(comboBox, &QComboBox::currentTextChanged, option,
                  [=](const QString &text) { option->setValue(text); });

  return optionWidget;
}

Settings *Settings::instance() {
  if (s_pSetting == nullptr) {
    s_pSetting = new Settings;
  }
  return s_pSetting;
}

void Settings::applySetting() {
#define Apply(Var, SettingName, Signal)                                        \
  auto Var = settings->option(SettingName);                                    \
  if (Var != nullptr)                                                          \
    Signal;

  Apply(plugin, "editor.plugin.enableplugin",
        emit sigChangePluginEnabled(plugin->value().toBool()));
  Apply(rootplugin, "editor.plugin.rootenableplugin",
        emit sigChangeRootPluginEnabled(rootplugin->value().toBool()));
  Apply(fontFamliy, "appearance.font.family",
        emit sigAdjustFont(fontFamliy->value().toString()));
  Apply(hexfontSize, "editor.basic.size",
        emit sigAdjustEditorFontSize(hexfontSize->value().toInt()));
  Apply(infofontSize, "appearance.font.size",
        emit sigAdjustInfoFontSize(infofontSize->value().toInt()));
  Apply(showAddr, "editor.basic.showaddr",
        emit sigShowAddressNumber(showAddr->value().toBool()));
  Apply(showCol, "editor.basic.showcol",
        emit sigShowColNumber(showCol->value().toBool()));
  Apply(showText, "editor.basic.showtext",
        emit sigShowEncodingText(showText->value().toBool()));
  Apply(windowstate, "appearance.window.windowsize",
        emit sigChangeWindowSize(windowstate->value().toString()));
  Apply(encoding, "editor.basic.encoding",
        emit sigChangedEncoding(encoding->value().toString()));
  Apply(fmax, "editor.basic.findmaxcount",
        emit sigAdjustFindMaxCount(fmax->value().toInt()););
  Apply(cplim, "editor.basic.copylimit",
        emit sigAdjustCopyLimit(cplim->value().toInt()););
  Apply(decstrlim, "editor.basic.decstrlimit",
        emit sigAdjustDecodeStringLimit(decstrlim->value().toInt()););
}

DDialog *Settings::createDialog(const QString &title, const QString &content,
                                const bool &bIsConflicts) {
  DDialog *dialog = new DDialog(title, content, m_pSettingsDialog);
  dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnBottomHint);
  dialog->setIcon(QIcon(":/images/general"));

  if (bIsConflicts) {
    dialog->addButton(QString(tr("Cancel")), true, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Replace")), false, DDialog::ButtonRecommend);
  } else {
    dialog->addButton(QString(tr("OK")), true, DDialog::ButtonRecommend);
  }

  return dialog;
}

void Settings::saveWindowState(DMainWindow *wnd, bool isorign) {
  if (wnd != nullptr) {
    QSettings settings(QApplication::organizationName(),
                       QApplication::applicationName());
    settings.setValue("geometry", wnd->saveGeometry());
    if (isorign)
      settings.setValue("windowState", wnd->saveState());
    else
      settings.setValue("orignwindowState", wnd->saveState());
  }
}

void Settings::loadWindowState(DMainWindow *wnd, bool isorign) {
  if (wnd != nullptr) {
    QSettings settings(QApplication::organizationName(),
                       QApplication::applicationName());
    wnd->restoreGeometry(settings.value("geometry").toByteArray());
    if (isorign)
      wnd->restoreState(settings.value("windowState").toByteArray());
    else
      wnd->restoreState(settings.value("orignwindowState").toByteArray());
  }
}

void Settings::saveFileDialogCurrent(QString path) {
  QSettings settings(QApplication::organizationName(),
                     QApplication::applicationName());
  settings.setValue("curpath", path);
}

QString Settings::loadFileDialogCurrent() {
  QSettings settings(QApplication::organizationName(),
                     QApplication::applicationName());
  return settings.value("curpath").toString();
}

void Settings::saveRecent(QStringList recent) {
  QSettings settings(QApplication::organizationName(),
                     QApplication::applicationName());
  settings.setValue("recent", recent);
}

QStringList Settings::loadRecent() {
  QSettings settings(QApplication::organizationName(),
                     QApplication::applicationName());
  return settings.value("recent").toStringList();
}
