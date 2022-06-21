#include "pluginwindow.h"
#include "utilities.h"
#include <DLabel>
#include <QListWidgetItem>

PluginWindow::PluginWindow(DMainWindow *parent) : DDialog(parent) {
  this->setFixedSize(500, 600);
  this->setWindowTitle(tr("plugin"));
  plglist = new DListWidget(this);
  plglist->setFixedHeight(200);
  this->addContent(plglist);
  txtb = new DTextBrowser(this);
  this->addContent(txtb);
  connect(plglist, &QListWidget::itemSelectionChanged, this,
          &PluginWindow::on_list_selchanged);
}

PluginWindow::~PluginWindow() {}

void PluginWindow::setPluginSystem(PluginSystem *pluginsys) {
  m_pluginsys = pluginsys;
  auto pico = ICONRES("plugin");
  for (auto item : pluginsys->plugins()) {
    plglist->addItem(new QListWidgetItem(pico, item->pluginName()));
  }
}

void PluginWindow::on_list_selchanged() {
  txtb->clear();
#define Info(mem, info) txtb->append(mem + " : " + info)

  auto plg = m_pluginsys->plugins().at(plglist->currentRow());
  Info(tr("pluginName"), plg->pluginName());
  Info(tr("pluginAuthor"), plg->pluginAuthor());
  Info(tr("pluginVersion"), QString::number(plg->pluginVersion()));
  Info(tr("pluginComment"), plg->pluginComment());
  Info(tr("PUID"), plg->puid());
  int i = 0;
  Info(tr("optionalInfos"), "");
  for (auto item : plg->optionalInfos()) {
    Info(QString::number(i), item.toString());
  }
}
