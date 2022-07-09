#include "recentfilemanager.h"
#include "appmanager.h"
#include "settings.h"
#include "utilities.h"
#include <DInputDialog>
#include <DMenu>
#include <DMessageManager>
#include <QFile>

RecentFileManager::RecentFileManager(DMenu *menu, DMainWindow *parent)
    : QObject(parent), m_menu(menu), m_parent(parent) {}

void RecentFileManager::apply() {
  QAction *a;
  a = new QAction(m_menu);
  a->setText(tr("ClearHistory"));
  a->setIcon(ICONRES("clearhis"));
  connect(a, &QAction::triggered, this, &RecentFileManager::clearFile);
  m_menu->addAction(a);

  a = new QAction(m_menu);
  a->setText(tr("RemoveItem"));
  a->setIcon(ICONRES("del"));
  connect(a, &QAction::triggered, [=] {
    if (hitems.count() == 0) {
      DMessageManager::instance()->sendMessage(m_parent, ICONRES("clearhis"),
                                               tr("NoHistoryDel"));
      return;
    }
    bool ok;
    auto d = DInputDialog::getInt(nullptr, tr("Input"), tr("InputIndex"), 0, 0,
                                  m_recents.count(), 1, &ok);
    if (ok) {
      m_menu->removeAction(hitems.at(d));
      m_recents.removeAt(d);
      for (auto it = hitems.begin() + d; it != hitems.end(); it++) {
        (*it)->setIconText(QString::number(d++));
      }
    }
  });
  m_menu->addAction(a);

  m_menu->addSeparator();
  auto s = Settings::instance()->loadRecent();
  int i = 0;
  for (auto item : s) {
    if (QFile::exists(item)) {
      if (m_recents.count() > 10)
        break;
      m_recents << item;
      a = new QAction(m_menu);
      a->setText(QString("%1 : %2").arg(i++).arg(item));
      a->setData(item);
      connect(a, &QAction::triggered, this, &RecentFileManager::trigger);
      hitems.push_back(a);
      m_menu->addAction(a);
    }
  }
}

RecentFileManager::~RecentFileManager() {
  Settings::instance()->saveRecent(m_recents);
}

void RecentFileManager::addRecentFile(QString filename) {
  if (m_recents.count() > 10) {
    m_recents.pop_back();
  }
  if (QFile::exists(filename) && m_recents.indexOf(filename) < 0) {
    auto a = new QAction(m_menu);
    a = new QAction(m_menu);
    a->setData(filename);
    connect(a, &QAction::triggered, this, &RecentFileManager::trigger);
    m_recents.push_front(filename);
    if (hitems.count())
      m_menu->insertAction(hitems.first(), a);
    else
      m_menu->addAction(a);
    hitems.push_front(a);
    auto i = 0;
    for (auto item : hitems) {
      item->setText(QString("%1 : %2").arg(i++).arg(item->data().toString()));
    }
  }
}

void RecentFileManager::trigger() {
  auto send = qobject_cast<QAction *>(sender());
  if (send) {
    auto f = send->data().toString();
    if (QFile::exists(f)) {
      AppManager::openFile(f);
      return;
    }
  }
  auto index = hitems.indexOf(send);
  if (index >= 0) {
    m_menu->removeAction(send);
    hitems.removeAt(index);
    m_recents.removeAt(index);
  }
}

void RecentFileManager::clearFile() {
  if (hitems.count() == 0) {
    DMessageManager::instance()->sendMessage(m_parent, ICONRES("clearhis"),
                                             tr("NoHistoryDel"));
    return;
  }
  for (auto item : hitems) {
    m_menu->removeAction(item);
  }
  m_recents.clear();
  hitems.clear();
  Settings::instance()->saveRecent(m_recents);
  DMessageManager::instance()->sendMessage(m_parent, ICONRES("clearhis"),
                                           tr("HistoryClearFinished"));
}
