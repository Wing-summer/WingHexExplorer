#ifndef UTILITIES_H
#define UTILITIES_H

#include "QHexView/document/qhexdocument.h"
#include "QHexView/document/qhexrenderer.h"
#include "iwingplugin.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QList>

#include <unistd.h>

#define ICONRES(name) QIcon(":/images/" name ".png")

struct HexFile {
  QHexDocument *doc;
  QHexRenderer *render;
  QString filename;
  int vBarValue;
};

class Utilities {
private:
public:
  static inline bool isRoot() { return getuid() == 0; }

  static bool activeWindowFromDock(quintptr winId) {
    bool bRet = true;
    // new interface use application as id
    QDBusInterface dockDbusInterface("com.deepin.dde.daemon.Dock",
                                     "/com/deepin/dde/daemon/Dock",
                                     "com.deepin.dde.daemon.Dock");
    QDBusReply<void> reply = dockDbusInterface.call("ActivateWindow", winId);
    if (!reply.isValid()) {
      qDebug() << "call com.deepin.dde.daemon.Dock failed" << reply.error();
      bRet = false;
    }
    return bRet;
  }

  static QString ProcessBytesCount(qint64 bytescount) {
    QString B[] = {"B", "KB", "MB", "GB", "TB"};
    auto av = bytescount;
    auto r = av;

    for (int i = 0; i < 5; i++) {
      auto lld = lldiv(r, 1024);
      r = lld.quot;
      av = lld.rem;
      if (r == 0) {
        return QString("%1 %2").arg(av).arg(B[i]);
      }
    }

    return QString("%1 TB").arg(av);
  }
};

#endif // UTILITIES_H
