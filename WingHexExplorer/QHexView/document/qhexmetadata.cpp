#include "qhexmetadata.h"
#include "commands/metaaddcommand.h"
#include "commands/metacommand.h"
#include "commands/metaremovecommand.h"
#include "commands/metareplacecommand.h"

QHexMetadata::QHexMetadata(QObject *parent) : QObject(parent) {}

const QHexLineMetadata &QHexMetadata::get(quint64 line) const {
  auto it = m_metadata.find(line);
  return it.value();
}

/*==================================*/
// added by wingsummer

void QHexMetadata::undo() { m_undo.undo(); }
void QHexMetadata::redo() { m_undo.redo(); }
bool QHexMetadata::canUndo() { return m_undo.canUndo(); }
bool QHexMetadata::canRedo() { return m_undo.canRedo(); }

QList<QHexMetadataAbsoluteItem> QHexMetadata::getallMetas() {
  return m_absoluteMetadata;
}

void QHexMetadata::modifyMetadata(QHexMetadataAbsoluteItem newmeta,
                                  QHexMetadataAbsoluteItem oldmeta,
                                  bool reundo) {
  removeMetadata(oldmeta);
  metadata(newmeta.begin, newmeta.end, newmeta.foreground, newmeta.background,
           newmeta.comment, false);
  if (!reundo)
    m_undo.push(new MetaReplaceCommand(this, newmeta, oldmeta));
}

void QHexMetadata::removeMetadata(QHexMetadataAbsoluteItem item, bool reundo) {
  quint64 firstRow = quint64(item.begin / m_lineWidth);
  quint64 lastRow = quint64(item.end / m_lineWidth);

  for (auto i = firstRow; i <= lastRow; i++) {
    QList<QHexMetadataItem> delmeta;
    auto it = m_metadata.find(i);
    if (it != m_metadata.end()) {
      for (auto iitem : *it) {
        if (iitem.foreground == item.foreground &&
            iitem.background == item.background &&
            iitem.comment == item.comment) {
          delmeta.push_back(iitem);
        }
      }
      for (auto iitem : delmeta) {
        it->remove(iitem);
      }
      m_absoluteMetadata.removeOne(item);
    }
  }

  if (!reundo)
    m_undo.push(new MetaRemoveCommand(this, item));
}

bool QHexMetadata::removeMetadata(qint64 offset,
                                  QList<QHexMetadataAbsoluteItem> refer) {
  QList<QHexMetadataAbsoluteItem> delneeded;
  for (auto item : m_absoluteMetadata) {
    if (offset >= item.begin && offset <= item.end) {
      delneeded.push_back(item);
    }
  }
  for (auto item : delneeded) {
    m_absoluteMetadata.removeOne(item);
    quint64 firstRow = quint64(item.begin / m_lineWidth);
    quint64 lastRow = quint64(item.end / m_lineWidth);

    for (auto i = firstRow; i <= lastRow; i++) {
      QList<QHexMetadataItem> delmeta;
      auto it = m_metadata.find(i);
      if (it != m_metadata.end()) {
        for (auto iitem : *it) {
          for (auto ritem : refer) {
            if (iitem.foreground == ritem.foreground &&
                iitem.background == ritem.background &&
                iitem.comment == ritem.comment) {
              delmeta.push_back(iitem);
            }
          }
        }
      }
      for (auto iitem : delmeta) {
        it->remove(iitem);
      }
    }
    m_undo.push(new MetaRemoveCommand(this, item));
  }
  return true;
}

QList<QHexMetadataAbsoluteItem> QHexMetadata::gets(qint64 offset) {
  QList<QHexMetadataAbsoluteItem> items;
  for (auto item : m_absoluteMetadata) {
    if (item.begin <= offset && offset <= item.end) {
      items.push_back(item);
    }
  }
  return items;
}

void QHexMetadata::applyMetas(QList<QHexMetadataAbsoluteItem> metas) {
  for (auto item : metas) {
    metadata(item.begin, item.end, item.foreground, item.background,
             item.comment);
  }
}

/*==================================*/

QString QHexMetadata::comments(quint64 line, int column) const {
  if (!this->lineHasMetadata(line))
    return QString();

  QString s;

  const auto &linemetadata = this->get(line);

  for (auto &mi : linemetadata) {
    if (!(mi.start <= column && column < mi.start + mi.length))
      continue;
    if (mi.comment.isEmpty())
      continue;

    if (!s.isEmpty())
      s += "\n";

    s += mi.comment;
  }

  return s;
}

bool QHexMetadata::lineHasMetadata(quint64 line) const {
  return m_metadata.contains(line);
}

void QHexMetadata::clear(quint64 line) {
  auto it = m_metadata.find(line);

  if (it == m_metadata.end())
    return;

  m_metadata.erase(it);
  emit metadataChanged(line);
}

void QHexMetadata::clear() {
  m_absoluteMetadata.clear();
  m_metadata.clear();
  emit metadataCleared();
}

void QHexMetadata::metadata(qint64 begin, qint64 end, const QColor &fgcolor,
                            const QColor &bgcolor, const QString &comment,
                            bool insert) {
  QHexMetadataAbsoluteItem absi{begin, end, fgcolor, bgcolor, comment};
  if (insert) {
    m_undo.push(new MetaAddCommand(this, absi));
  }
  m_absoluteMetadata.append(absi);
  setAbsoluteMetadata(m_absoluteMetadata.back());
}

void QHexMetadata::setAbsoluteMetadata(const QHexMetadataAbsoluteItem &mai) {
  const quint64 firstRow = quint64(mai.begin / m_lineWidth);
  const quint64 lastRow = quint64(mai.end / m_lineWidth);

  for (quint64 row = firstRow; row <= lastRow; ++row) {
    int start, length;
    if (row == firstRow) {
      start = mai.begin % m_lineWidth;
    } else {
      start = 0;
    }
    if (row == lastRow) {
      const int lastChar = mai.end % m_lineWidth;
      length = lastChar - start;
    } else {
      length = m_lineWidth;
    }
    if (length > 0) {
      setMetadata(
          {row, start, length, mai.foreground, mai.background, mai.comment});
    }
  }
}

void QHexMetadata::setLineWidth(quint8 width) {
  if (width != m_lineWidth) {
    m_lineWidth = width;
    // clean m_metadata
    m_metadata.clear();
    // and regenerate with new line width size
    for (int i = 0; i < m_absoluteMetadata.size(); ++i) {
      setAbsoluteMetadata(m_absoluteMetadata[i]);
    }
  }
}

void QHexMetadata::metadata(quint64 line, int start, int length,
                            const QColor &fgcolor, const QColor &bgcolor,
                            const QString &comment) {
  const qint64 begin = qint64(line * m_lineWidth + uint(start));
  const qint64 end = begin + length;
  // delegate to the new interface
  this->metadata(begin, end, fgcolor, bgcolor, comment);
}

void QHexMetadata::color(quint64 line, int start, int length,
                         const QColor &fgcolor, const QColor &bgcolor) {
  this->metadata(line, start, length, fgcolor, bgcolor, QString());
}

void QHexMetadata::foreground(quint64 line, int start, int length,
                              const QColor &fgcolor) {
  this->color(line, start, length, fgcolor, QColor());
}

void QHexMetadata::background(quint64 line, int start, int length,
                              const QColor &bgcolor) {
  this->color(line, start, length, QColor(), bgcolor);
}

void QHexMetadata::comment(quint64 line, int start, int length,
                           const QString &comment) {
  this->metadata(line, start, length, QColor(), QColor(), comment);
}

void QHexMetadata::setMetadata(const QHexMetadataItem &mi) {
  if (!m_metadata.contains(mi.line)) {
    QHexLineMetadata linemetadata;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    linemetadata << mi;
#else
    linemetadata.push_back(mi);
#endif
    m_metadata[mi.line] = linemetadata;
  } else {
    QHexLineMetadata &linemetadata = m_metadata[mi.line];
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    linemetadata << mi;
#else
    linemetadata.push_back(mi);
#endif
  }

  emit metadataChanged(mi.line);
}
