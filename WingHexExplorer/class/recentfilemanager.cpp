#include "recentfilemanager.h"
#include "appmanager.h"
#include "settings.h"
#include "utilities.h"
#include <DInputDialog>
#include <DMenu>
#include <QFile>

RecentFileManager::RecentFileManager(DMenu *menu, QObject *parent)
    : QObject(parent), m_menu(menu) {}

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
    auto d = DInputDialog::getInt(nullptr, tr("Input"), tr("InputIndex"), 0, 0,
                                  m_recents.count());
    m_menu->removeAction(hitems.at(d));
    m_recents.removeAt(d);
    for (auto it = hitems.begin() + d; it != hitems.end(); it++) {
      (*it)->setIconText(QString::number(d++));
    }
  });
  m_menu->addAction(a);

  m_menu->addSeparator();
  auto s = Settings::instance()->loadRecent();
  int i = 0;
  for (auto item : s) {
    if (QFile::exists(item)) {
      m_recents << item;

      a = new QAction(m_menu);
      a->setText(item);
      a->setIconText(QString::number(i++));
      connect(a, &QAction::triggered, [=] {
        auto send = qobject_cast<QAction *>(sender());
        if (send) {
          auto f = send->text();
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
      });
      hitems.push_back(a);
      m_menu->addAction(a);
    }
  }
}

RecentFileManager::~RecentFileManager() {
  Settings::instance()->saveRecent(m_recents);
}

void RecentFileManager::addRecentFile(QString filename) {
  if (QFile::exists(filename) && m_recents.indexOf(filename) < 0) {
    auto a = new QAction(m_menu);
    a = new QAction(m_menu);
    a->setText(filename);
    a->setIconText(QString::number(m_recents.count()));
    connect(a, &QAction::triggered, [=] {
      auto send = qobject_cast<QAction *>(sender());
      if (send) {
        auto f = send->text();
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
    });
    m_recents << filename;
    hitems.push_back(a);
    m_menu->addAction(a);
  }
}

void RecentFileManager::clearFile() {
  for (auto item : hitems) {
    m_menu->removeAction(item);
  }
  m_recents.clear();
  hitems.clear();
}
