#ifndef RECENTFILEMANAGER_H
#define RECENTFILEMANAGER_H

#include <DMenu>
#include <QAction>
#include <QList>
#include <QObject>

DWIDGET_USE_NAMESPACE

class RecentFileManager : public QObject {
  Q_OBJECT
public:
  explicit RecentFileManager(DMenu *menu, QObject *parent = nullptr);
  ~RecentFileManager();
  void addRecentFile(QString filename);
  void clearFile();
  void apply();

private:
  DMenu *m_menu;
  QStringList m_recents;
  QList<QAction *> hitems;
};

#endif // RECENTFILEMANAGER_H
