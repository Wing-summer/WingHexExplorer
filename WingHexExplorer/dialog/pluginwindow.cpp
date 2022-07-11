#include "pluginwindow.h"
#include "utilities.h"
#include <DLabel>
#include <QListWidgetItem>
#include <QPushButton>

#define Bool2String(b) (b ? "true" : "false")

PluginWindow::PluginWindow(DMainWindow *parent) : DDialog(parent) {
  this->setFixedSize(500, 750);
  this->setWindowTitle(tr("plugin"));
  txtc = new DTextBrowser(this);
  addContent(txtc);
  addSpacing(5);
  plglist = new DListWidget(this);
  plglist->setFixedHeight(200);
  addContent(plglist);
  addSpacing(5);
  txtb = new DTextBrowser(this);
  txtb->setMinimumHeight(300);
  addContent(txtb);
  addSpacing(10);
  connect(plglist, &QListWidget::itemSelectionChanged, this,
          &PluginWindow::on_list_selchanged);
  auto btn = new QPushButton(tr("Refresh"), this);
  connect(btn, &QPushButton::clicked, this,
          [=] { this->setPluginSystem(m_pluginsys); });
  addContent(btn);
}

PluginWindow::~PluginWindow() {}

void PluginWindow::setPluginSystem(PluginSystem *pluginsys) {
  m_pluginsys = pluginsys;
  plglist->clear();
  auto pico = ICONRES("plugin");
  for (auto item : pluginsys->plugins()) {
    plglist->addItem(new QListWidgetItem(pico, item->pluginName()));
  }

#define CInfo(mem, info) txtc->append(mem + " : " + info)
  txtc->clear();
  if (pluginsys->hasControl()) {
    auto p = pluginsys->currentControlPlugin();
    CInfo(tr("CtlPlg"), p->pluginName());
    CInfo(tr("CtlPlgPuid"), p->puid());
    CInfo(tr("CtlPlgAuthor"), p->pluginAuthor());
    CInfo(tr("ControlTimeout"),
          Bool2String(pluginsys->currentControlTimeout()));
  } else {
    txtc->append(tr("NoPlgControl"));
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
  txtb->append(QString(10, '-'));
  bool hc = plg == m_pluginsys->currentControlPlugin();
  Info(tr("HasControl"), Bool2String(hc));
  Info(tr("ControlTimeout"),
       (hc ? Bool2String(m_pluginsys->currentControlTimeout()) : QString("-")));
}
