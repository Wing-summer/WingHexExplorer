#ifndef PLUGINWINDOW_H
#define PLUGINWINDOW_H

#include "pluginsystem.h"
#include <DDialog>
#include <DListWidget>
#include <DMainWindow>
#include <DTextBrowser>
#include <QObject>
#include <QWidget>

DWIDGET_USE_NAMESPACE

class PluginWindow : public DDialog {
  Q_OBJECT
public:
  PluginWindow(DMainWindow *parent = nullptr);
  void setPluginSystem(PluginSystem *pluginsys);
  ~PluginWindow();

private:
  PluginSystem *m_pluginsys;
  DListWidget *plglist;
  DTextBrowser *txtb;

  void on_list_selchanged();
};

#endif // PLUGINWINDOW_H
