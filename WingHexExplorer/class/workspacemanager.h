#ifndef WORKSPACEMANAGER_H
#define WORKSPACEMANAGER_H

#include "QHexView/document/qhexdocument.h"
#include "QHexView/document/qhexmetadata.h"
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QObject>
#include <QStringList>

class WorkSpaceManager : public QObject {
  Q_OBJECT
public:
  explicit WorkSpaceManager(QObject *parent = nullptr);
  bool static saveWorkSpace(QString filename, QString file,
                            QList<BookMarkStruct> bookmarks,
                            QList<QHexMetadataAbsoluteItem> metas);
  bool static loadWorkSpace(QString filename, QString &file,
                            QList<BookMarkStruct> &bookmarks,
                            QList<QHexMetadataAbsoluteItem> &metas);

signals:

public slots:
};

#endif // WORKSPACEMANAGER_H
