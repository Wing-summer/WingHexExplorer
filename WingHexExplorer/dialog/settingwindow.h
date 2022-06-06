#ifndef SETTINGWINDOW_H
#define SETTINGWINDOW_H

#include <DDialog>
#include <DMainWindow>
#include <DSettingsDialog>
#include <QObject>

DWIDGET_USE_NAMESPACE

class SettingWindow : public DSettingsDialog {
  Q_OBJECT
public:
  SettingWindow(DMainWindow *parent = nullptr);
};

#endif // SETTINGWINDOW_H
