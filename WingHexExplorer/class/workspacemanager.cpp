#include "workspacemanager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>

WorkSpaceManager::WorkSpaceManager(QObject *parent) : QObject(parent) {}

bool WorkSpaceManager::loadWorkSpace(QString filename, QString &file,
                                     QList<BookMarkStruct> &bookmarks,
                                     QHash<quint64, QHexLineMetadata> &metas) {
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
                if (!item.isUndefined() && item.isObject()) {
                  auto sitem = item.toObject();
                  auto ipos = sitem.value("pos");
                  if (!ipos.isUndefined() && ipos.isString()) {
                    bool b = false;
                    auto pos = ipos.toString().toULongLong(&b);
                    if (!b)
                      continue;
                    auto ivalues = sitem.value("value");
                    if (!ivalues.isUndefined() && ivalues.isArray()) {
                      QHexLineMetadata linemetas;
                      for (auto v : ivalues.toArray()) {
                        if (!v.isUndefined() && v.isObject()) {
                          auto linem = v.toObject();
                          auto line = linem.value("line");
                          auto start = linem.value("start");
                          auto length = linem.value("length");
                          auto comment = linem.value("comment");
                          auto fgcolor = linem.value("fgcolor");
                          auto bgcolor = linem.value("bgcolor");
                          if (!line.isUndefined() && line.isString() &&
                              !start.isUndefined() && start.isString() &&
                              !length.isUndefined() && length.isString() &&
                              !comment.isUndefined() && comment.isString() &&
                              !fgcolor.isUndefined() && fgcolor.isString() &&
                              !bgcolor.isUndefined() && bgcolor.isString()) {
                            auto nline = line.toString().toULongLong(&b);
                            if (!b)
                              continue;
                            auto nstart = start.toString().toInt(&b);
                            if (!b)
                              continue;
                            auto nlength = length.toString().toInt(&b);
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
                            QHexMetadataItem hmi;
                            hmi.line = nline;
                            hmi.start = nstart;
                            hmi.length = nlength;
                            hmi.comment = comment.toString();
                            hmi.foreground = fcolor;
                            hmi.background = bcolor;
                            linemetas.push_back(hmi);
                          }
                        }
                      }
                      if (!linemetas.size())
                        continue;
                      metas.insert(pos, linemetas);
                    }
                  }
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

bool WorkSpaceManager::saveWorkSpace(
    QString filename, QString file, QList<BookMarkStruct> bookmarklist,
    QHash<quint64, QHexLineMetadata> metalist) {
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
    for (auto meta : metalist.keys()) {
      QJsonObject i;
      QJsonArray linemetas;
      i.insert("pos", QString::number(meta));
      for (auto line : metalist.value(meta)) {
        QJsonObject lineobj;
        lineobj.insert("line", QString::number(line.line));
        lineobj.insert("start", QString::number(line.start));
        lineobj.insert("length", QString::number(line.length));
        lineobj.insert("comment", line.comment);
        lineobj.insert("fgcolor", QString::number(line.foreground.rgba(), 16));
        lineobj.insert("bgcolor", QString::number(line.background.rgba(), 16));
        linemetas.append(lineobj);
      }
      i.insert("value", linemetas);
      metas.append(i);
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
