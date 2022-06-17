#include "workspacemanager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>

WorkSpaceManager::WorkSpaceManager(QObject *parent) : QObject(parent) {}

bool WorkSpaceManager::loadWorkSpace(QString filename, QString &file,
                                     QList<BookMarkStruct> &bookmarks,
                                     QList<QHexMetadataAbsoluteItem> &metas) {
  QFile f(filename);
  if (f.exists()) {
    QJsonParseError err;
    if (!f.open(QFile::ReadOnly))
      return false;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error == QJsonParseError::NoError) {
      auto jobj = doc.object();
      auto t = jobj.value("type");
      if (!t.isUndefined() && t.isString()) {
        auto type = t.toString();
        if (!QString::compare(type, "workspace", Qt::CaseInsensitive)) {
          auto ff = jobj.value("file");
          if (!ff.isUndefined() && t.isString()) {
            auto fi = ff.toString();
            QFileInfo finfo(f);
            file = fi[0] != '/' ? finfo.absoluteDir().path() + "/" + fi : fi;
            auto values = jobj.value("metas");
            if (!values.isUndefined() && values.isArray()) {
              auto metaitems = values.toArray();
              for (auto item : metaitems) {
                bool b = false;
                auto linem = item.toObject();
                auto begin = linem.value("begin");
                auto end = linem.value("end");
                auto comment = linem.value("comment");
                auto fgcolor = linem.value("fgcolor");
                auto bgcolor = linem.value("bgcolor");
                if (!begin.isUndefined() && begin.isString() &&
                    !end.isUndefined() && end.isString() &&
                    !comment.isUndefined() && comment.isString() &&
                    !fgcolor.isUndefined() && fgcolor.isString() &&
                    !bgcolor.isUndefined() && bgcolor.isString()) {
                  auto nbegin = begin.toString().toUInt(&b);
                  if (!b)
                    continue;
                  auto nend = end.toString().toInt(&b);
                  if (!b)
                    continue;
                  auto nf = fgcolor.toString().toUInt(&b, 16);
                  if (!b)
                    continue;
                  auto nb = bgcolor.toString().toUInt(&b, 16);
                  if (!b)
                    continue;
                  auto fcolor = QColor::fromRgba(nf);
                  auto bcolor = QColor::fromRgba(nb);

                  QHexMetadataAbsoluteItem metaitem;
                  metaitem.begin = nbegin;
                  metaitem.end = nend;
                  metaitem.comment = comment.toString();
                  metaitem.foreground = fcolor;
                  metaitem.background = bcolor;
                  metas.append(metaitem);
                }
              }
            }
            values = jobj.value("bookmarks");
            if (!values.isUndefined() && values.isArray()) {
              for (auto item : values.toArray()) {
                if (!item.isUndefined() && item.isObject()) {
                  auto sitem = item.toObject();
                  auto pos = sitem.value("pos");
                  auto comment = sitem.value("comment");
                  if (!pos.isUndefined() && pos.isString() &&
                      !comment.isUndefined() && comment.isString()) {
                    auto b = false;
                    auto ipos = pos.toString().toLongLong(&b);
                    if (!b)
                      continue;
                    BookMarkStruct book;
                    book.pos = ipos;
                    book.comment = comment.toString();
                    bookmarks.append(book);
                  }
                }
              }
            }
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool WorkSpaceManager::saveWorkSpace(QString filename, QString file,
                                     QList<BookMarkStruct> bookmarklist,
                                     QList<QHexMetadataAbsoluteItem> metalist) {
  QFile f(filename);
  if (f.open(QFile::WriteOnly)) {
    QJsonObject jobj;
    jobj.insert("type", "workspace");

    if (file[0] == '/') {
      QDir dir(QFileInfo(f).absoluteDir());
      QFileInfo fi(file);
      file = dir.relativeFilePath(fi.absoluteFilePath());
    }

    jobj.insert("file", file);

    QJsonArray metas;
    for (auto meta : metalist) {
      QJsonObject obj;
      obj.insert("begin", QString::number(meta.begin));
      obj.insert("end", QString::number(meta.end));
      obj.insert("comment", meta.comment);
      obj.insert("fgcolor", QString::number(meta.foreground.rgba(), 16));
      obj.insert("bgcolor", QString::number(meta.background.rgba(), 16));
      metas.append(obj);
    }
    jobj.insert("metas", metas);

    QJsonArray bookmarks;
    for (auto item : bookmarklist) {
      QJsonObject i;
      i.insert("pos", QString::number(item.pos));
      i.insert("comment", item.comment);
      bookmarks.append(i);
    }
    jobj.insert("bookmarks", bookmarks);

    QJsonDocument jdoc(jobj);
    if (f.write(jdoc.toJson(QJsonDocument::JsonFormat::Indented)) >= 0) {
      f.close();
      return true;
    }
  }
  return false;
}
