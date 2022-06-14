#include "settings.h"
#include "dthememanager.h"
#include "utilities.h"
#include <DMessageManager>
#include <DSettings>
#include <DSettingsGroup>
#include <DSettingsOption>
#include <DSettingsWidgetFactory>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFontDatabase>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTextCodec>

Settings *Settings::s_pSetting = nullptr;
Settings::Settings(QWidget *parent) : QObject(parent) {
  QString strConfigPath =
      QString("%1/%2/%3/config.conf")
          .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
          .arg(qApp->organizationName())
          .arg(qApp->applicationName());

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
  BindConfigSignal(infofontSize, "appearance.basic.size",
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
  BindConfigSignal(windowstate, "appearance.window.windowstate", {
    emit sigChangeWindowState(value.toString());
    DMessageManager::instance()->sendMessage(
        m_pSettingsDialog, ICONRES("setting"), tr("RestartTakeEffect"));
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

  auto enCoding = settings->option("editor.basic.encoding");
  QMap<QString, QVariant> encodingMap;
  QStringList encodings = Utilities::GetEncodings();
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
    emit Signal;

  Apply(plugin, "editor.plugin.enableplugin",
        sigChangePluginEnabled(plugin->value().toBool()));
  Apply(rootplugin, "editor.plugin.rootenableplugin",
        sigChangeRootPluginEnabled(rootplugin->value().toBool()));
  Apply(fontFamliy, "appearance.font.family",
        sigAdjustFont(fontFamliy->value().toString()));
  Apply(hexfontSize, "editor.basic.size",
        sigAdjustEditorFontSize(hexfontSize->value().toInt()));
  Apply(infofontSize, "appearance.basic.size",
        sigAdjustInfoFontSize(infofontSize->value().toInt()));
  Apply(showAddr, "editor.basic.showaddr",
        sigShowAddressNumber(showAddr->value().toBool()));
  Apply(showCol, "editor.basic.showcol",
        sigShowColNumber(showCol->value().toBool()));
  Apply(showText, "editor.basic.showtext",
        sigShowEncodingText(showText->value().toBool()));
  Apply(windowstate, "appearance.window.windowsize",
        sigChangeWindowState(windowstate->value().toString()));
  Apply(encoding, "editor.basic.encoding",
        sigChangedEncoding(encoding->value().toString()));
  Apply(fmax, "editor.basic.findmaxcount",
        sigAdjustFindMaxCount(fmax->value().toInt()););
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
    settings.setValue("windowState", wnd->saveState(isorign ? 0 : 1));
  }
}

void Settings::loadWindowState(DMainWindow *wnd, bool isorign) {
  if (wnd != nullptr) {
    QSettings settings(QApplication::organizationName(),
                       QApplication::applicationName());
    wnd->restoreGeometry(settings.value("geometry").toByteArray());
    wnd->restoreState(settings.value("windowState").toByteArray(),
                      isorign ? 0 : 1);
  }
}
