#include "settings.h"

#include "dthememanager.h"
#include <DSettings>
#include <DSettingsGroup>
#include <DSettingsOption>
#include <DSettingsWidgetFactory>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFontDatabase>
#include <QStandardPaths>
#include <QStyleFactory>

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
  connect(Var, &Dtk::Core::DSettingsOption::valueChanged, this,                \
          [=](QVariant value) { emit Signal; });

  BindConfigSignal(fontFamliy, "editor.font.family",
                   sigAdjustFont(value.toString()));
  BindConfigSignal(fontSize, "editor.font.size",
                   sigAdjustFontSize(value.toInt()));
  BindConfigSignal(showAddr, "edit.font.showaddr",
                   sigShowAddressNumber(value.toBool()));
  BindConfigSignal(showCol, "edit.font.showcol",
                   sigShowColNumber(value.toBool()));
  BindConfigSignal(showText, "edit.font.showtext",
                   sigShowEncodingText(value.toBool()));

  // only used by new window
  auto windowState = settings->option("advance.window.windowstate");
  QMap<QString, QVariant> windowStateMap;
  windowStateMap.insert("keys", QStringList() << "window_normal"
                                              << "window_maximum"
                                              << "fullscreen");
  windowStateMap.insert("values", QStringList() << tr("Normal") << tr("Maximum")
                                                << tr("Fullscreen"));
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
