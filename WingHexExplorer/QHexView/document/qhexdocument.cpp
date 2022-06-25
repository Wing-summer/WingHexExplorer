#include "qhexdocument.h"
#include "buffer/qfilebuffer.h"
#include "commands/baseaddrcommand.h"
#include "commands/bookmark/bookmarkaddcommand.h"
#include "commands/bookmark/bookmarkclearcommand.h"
#include "commands/bookmark/bookmarkremovecommand.h"
#include "commands/bookmark/bookmarkreplacecommand.h"
#include "commands/hex/insertcommand.h"
#include "commands/hex/removecommand.h"
#include "commands/hex/replacecommand.h"
#include "commands/meta/metashowcommand.h"
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFile>

/*======================*/
// added by wingsummer

void QHexDocument::addUndoCommand(QUndoCommand *command) {
  if (command)
    m_undostack.push(command);
}

void QHexDocument::SetMetaVisible(bool b) {
  m_undostack.push(new MetaShowCommand(this, ShowType::All, b));
}

void QHexDocument::SetMetabgVisible(bool b) {
  m_undostack.push(new MetaShowCommand(this, ShowType::BgColor, b));
}

void QHexDocument::SetMetafgVisible(bool b) {
  m_undostack.push(new MetaShowCommand(this, ShowType::FgColor, b));
}

void QHexDocument::SetMetaCommentVisible(bool b) {
  m_undostack.push(new MetaShowCommand(this, ShowType::Comment, b));
}

void QHexDocument::setMetabgVisible(bool b) {
  m_metabg = b;
  emit documentChanged();
  emit metabgVisibleChanged(b);
}

void QHexDocument::setMetafgVisible(bool b) {
  m_metafg = b;
  emit documentChanged();
  emit metafgVisibleChanged(b);
}

void QHexDocument::setMetaCommentVisible(bool b) {
  m_metacomment = b;
  emit documentChanged();
  emit metaCommentVisibleChanged(b);
}

bool QHexDocument::metabgVisible() { return m_metabg; }

bool QHexDocument::metafgVisible() { return m_metafg; }

bool QHexDocument::metaCommentVisible() { return m_metacomment; }

bool QHexDocument::isDocSaved() {
  return m_undostack.isClean() && !m_pluginModed;
}
void QHexDocument::setDocSaved(bool b) {
  if (b) {
    m_undostack.setClean();
  }
  m_pluginModed = !b;
  emit documentSaved(b);
}
bool QHexDocument::isReadOnly() { return m_readonly; }
bool QHexDocument::isKeepSize() { return m_keepsize; }
bool QHexDocument::isLocked() { return m_islocked; }
bool QHexDocument::setLockedFile(bool b) {
  if (m_readonly)
    return false;
  m_islocked = b;
  m_cursor->setInsertionMode(QHexCursor::OverwriteMode);
  setDocSaved(false);
  return true;
}
bool QHexDocument::setKeepSize(bool b) {
  if (m_readonly)
    return false;
  m_keepsize = b;
  if (b)
    m_cursor->setInsertionMode(QHexCursor::OverwriteMode);
  setDocSaved(false);
  return true;
}

void QHexDocument::getBookMarks(QList<BookMarkStruct> &bookmarks) {
  bookmarks.clear();
  bookmarks.append(this->bookmarks);
}

void QHexDocument::AddBookMark(qint64 pos, QString comment) {
  m_undostack.push(new BookMarkAddCommand(this, pos, comment));
}

void QHexDocument::ModBookMark(qint64 pos, QString comment) {
  m_undostack.push(
      new BookMarkReplaceCommand(this, pos, comment, bookMarkComment(pos)));
}

void QHexDocument::ClearBookMark() {
  m_undostack.push(new BookMarkClearCommand(this, getAllBookMarks()));
}

bool QHexDocument::addBookMark(qint64 pos, QString comment) {
  if (!existBookMark(pos)) {
    BookMarkStruct b{pos, comment};
    bookmarks.append(b);
    setDocSaved(false);
    emit bookMarkChanged(BookMarkModEnum::Insert, -1, pos, comment);
    return true;
  }
  return false;
}

QString QHexDocument::bookMarkComment(qint64 pos) {
  if (pos > 0 && pos < m_buffer->length()) {
    for (auto item : bookmarks) {
      if (item.pos == pos) {
        return item.comment;
      }
    }
  }
  return QString();
}

BookMarkStruct QHexDocument::bookMark(qint64 pos) {
  if (pos > 0 && pos < m_buffer->length()) {
    for (auto item : bookmarks) {
      if (item.pos == pos) {
        return item;
      }
    }
  }
  return BookMarkStruct{-1, ""};
}

BookMarkStruct QHexDocument::bookMark(int index) {
  if (index >= 0 && index < bookmarks.count()) {
    return bookmarks.at(index);
  } else {
    BookMarkStruct b;
    b.pos = -1;
    return b;
  }
}

void QHexDocument::RemoveBookMark(int index) {
  auto b = bookmarks.at(index);
  m_undostack.push(new BookMarkRemoveCommand(this, b.pos, b.comment));
}

void QHexDocument::removeBookMark(qint64 pos) {
  if (pos >= 0 && pos < m_buffer->length()) {
    int index = 0;
    for (auto item : bookmarks) {
      if (pos == item.pos) {
        bookmarks.removeAt(index);
        setDocSaved(false);
        emit bookMarkChanged(BookMarkModEnum::Remove, index, -1, QString());
        break;
      }
      index++;
    }
  }
}

void QHexDocument::removeBookMark(int index) {
  if (index >= 0 && index < bookmarks.count()) {
    bookmarks.removeAt(index);
    setDocSaved(false);
    emit bookMarkChanged(BookMarkModEnum::Remove, index, -1, QString());
  }
}

bool QHexDocument::modBookMark(qint64 pos, QString comment) {
  if (pos > 0 && pos < m_buffer->length()) {
    int index = 0;
    for (auto &item : bookmarks) {
      if (item.pos == pos) {
        item.comment = comment;
        setDocSaved(false);
        emit bookMarkChanged(BookMarkModEnum::Modify, index, -1, comment);
        return true;
      }
      index++;
    }
  }
  return false;
}

void QHexDocument::clearBookMark() {
  bookmarks.clear();
  setDocSaved(false);
  emit bookMarkChanged(BookMarkModEnum::Clear, -1, -1, QString());
}

void QHexDocument::gotoBookMark(int index) {
  if (index >= 0 && index < bookmarks.count()) {
    auto bookmark = bookmarks.at(index);
    m_cursor->moveTo(qlonglong(bookmark.pos));
  }
}

bool QHexDocument::existBookMark(qint64 pos) {
  for (auto item : bookmarks) {
    if (item.pos == pos) {
      return true;
    }
  }
  return false;
}

bool QHexDocument::existBookMark() {
  return existBookMark(m_cursor->position().offset());
}

bool QHexDocument::existBookMark(int &index) {
  auto curpos = m_cursor->position().offset();
  int i = 0;
  for (auto item : bookmarks) {
    if (item.pos == curpos) {
      index = i;
      return true;
    }
    i++;
  }
  return false;
}

QList<BookMarkStruct> QHexDocument::getAllBookMarks() { return bookmarks; }

void QHexDocument::applyBookMarks(QList<BookMarkStruct> books) {
  bookmarks.append(books);
  setDocSaved(false);
  emit bookMarkChanged(BookMarkModEnum::Apply, -1, -1, QString());
}

void QHexDocument::findAllBytes(qint64 begin, qint64 end, QByteArray b,
                                QList<quint64> &results, int maxCount) {
  results.clear();
  qlonglong p = begin > 0 ? begin : 0;
  qlonglong e = end > begin ? end : -1;
  auto offset = b.count();
  while (1) {
    p = m_buffer->indexOf(b, p);
    if (p < 0 || (e > 0 && p > e) ||
        (maxCount > 0 && results.count() >= maxCount)) {
      break;
    }
    results.append(quint64(p));
    p += offset + 1;
  }
}

bool QHexDocument::removeSelection() {
  if (!m_cursor->hasSelection())
    return false;

  auto res = this->remove(m_cursor->selectionStart().offset(),
                          m_cursor->selectionLength());
  if (res)
    m_cursor->clearSelection();
  return res;
}

bool QHexDocument::cut(bool hex) {
  if (!m_cursor->hasSelection() || m_keepsize)
    return false;

  this->copy(hex);
  return this->removeSelection();
}

void QHexDocument::paste(bool hex) {
  QClipboard *c = qApp->clipboard();
  QByteArray data = c->text().toUtf8();

  if (data.isEmpty())
    return;

  this->removeSelection();

  if (hex)
    data = QByteArray::fromHex(data);

  auto pos = m_cursor->position().offset();
  if (!m_keepsize) {
    this->insert(pos, data);
    m_cursor->moveTo(pos + data.length()); // added by wingsummer
  } else
    this->replace(pos, data);
}

void QHexDocument::insert(qint64 offset, uchar b) {
  if (m_keepsize || m_readonly || m_islocked)
    return;
  this->insert(offset, QByteArray(1, char(b)));
}

void QHexDocument::insert(qint64 offset, const QByteArray &data) {
  if (m_keepsize || m_readonly || m_islocked)
    return;
  m_buffer->insert(offset, data);
  emit documentChanged();
}

void QHexDocument::replace(qint64 offset, uchar b) {
  if (m_readonly || m_islocked)
    return;
  this->replace(offset, QByteArray(1, char(b)));
}

void QHexDocument::replace(qint64 offset, const QByteArray &data) {
  m_buffer->replace(offset, data);
  emit documentChanged();
}

bool QHexDocument::remove(qint64 offset, int len) {
  if (m_keepsize || m_readonly || m_islocked)
    return false;
  m_buffer->remove(offset, len);
  emit documentChanged();
  return true;
}

/*======================*/

// modified by wingsummer
QHexDocument::QHexDocument(QHexBuffer *buffer, bool readonly, QObject *parent)
    : QObject(parent), m_baseaddress(0), m_readonly(false), m_keepsize(false),
      m_islocked(false) {
  m_buffer = buffer;
  m_buffer->setParent(this); // Take Ownership
  m_areaindent = DEFAULT_AREA_IDENTATION;
  m_hexlinewidth = DEFAULT_HEX_LINE_LENGTH;

  /*=======================*/
  // added by wingsummer
  m_readonly = readonly;
  if (m_readonly) {
    m_islocked = true;
    m_keepsize = true;
  }
  /*=======================*/

  m_cursor = new QHexCursor(this);
  m_cursor->setLineWidth(m_hexlinewidth);
  m_metadata = new QHexMetadata(&m_undostack, this);
  m_metadata->setLineWidth(m_hexlinewidth);

  connect(m_metadata, &QHexMetadata::metadataChanged, this, [=](quint64 line) {
    setDocSaved(false);
    emit QHexDocument::metaLineChanged(line);
  });
  connect(m_metadata, &QHexMetadata::metadataCleared, this, [=] {
    setDocSaved(false);
    emit QHexDocument::documentChanged();
  });

  /*=======================*/
  // added by wingsummer
  connect(&m_undostack, &QUndoStack::canUndoChanged, this,
          &QHexDocument::canUndoChanged);
  connect(&m_undostack, &QUndoStack::canRedoChanged, this,
          &QHexDocument::canRedoChanged);
  connect(&m_undostack, &QUndoStack::cleanChanged, this, [=](bool b) {
    emit QHexDocument::documentSaved(b && !m_pluginModed);
  });
  /*=======================*/
}

bool QHexDocument::isEmpty() const { return m_buffer->isEmpty(); }
bool QHexDocument::atEnd() const {
  return m_cursor->position().offset() >= m_buffer->length();
}
bool QHexDocument::canUndo() const { return m_undostack.canUndo(); }
bool QHexDocument::canRedo() const { return m_undostack.canRedo(); }
qint64 QHexDocument::length() const { return m_buffer->length(); }
quint64 QHexDocument::baseAddress() const { return m_baseaddress; }
QHexCursor *QHexDocument::cursor() const { return m_cursor; }

int QHexDocument::areaIndent() const { return m_areaindent; }
void QHexDocument::setAreaIndent(quint8 value) { m_areaindent = value; }
int QHexDocument::hexLineWidth() const { return m_hexlinewidth; }
void QHexDocument::setHexLineWidth(quint8 value) {
  m_hexlinewidth = value;
  m_cursor->setLineWidth(value);
  m_metadata->setLineWidth(value);
}

QHexMetadata *QHexDocument::metadata() const { return m_metadata; }
QByteArray QHexDocument::read(qint64 offset, int len) {
  return m_buffer->read(offset, len);
}

bool QHexDocument::RemoveSelection() {
  if (!m_cursor->hasSelection())
    return false;

  auto res = this->Remove(m_cursor->selectionStart().offset(),
                          m_cursor->selectionLength());
  if (res)
    m_cursor->clearSelection();
  return res;
}

QByteArray QHexDocument::selectedBytes() const {
  if (!m_cursor->hasSelection())
    return QByteArray();

  return m_buffer->read(m_cursor->selectionStart().offset(),
                        m_cursor->selectionLength());
}

char QHexDocument::at(int offset) const { return char(m_buffer->at(offset)); }

void QHexDocument::SetBaseAddress(quint64 baseaddress) {
  m_undostack.push(new BaseAddrCommand(this, m_baseaddress, baseaddress));
}

void QHexDocument::setBaseAddress(quint64 baseaddress) {
  if (m_baseaddress == baseaddress)
    return;

  m_baseaddress = baseaddress;
  emit documentChanged();
}

void QHexDocument::sync() { emit documentChanged(); }

void QHexDocument::undo() {
  m_undostack.undo();
  emit documentChanged();
}

void QHexDocument::redo() {
  m_undostack.redo();
  emit documentChanged();
}

bool QHexDocument::Cut(bool hex) {
  if (!m_cursor->hasSelection())
    return true;

  if (m_keepsize)
    return false;

  this->copy(hex);
  return this->RemoveSelection();
}

void QHexDocument::copy(bool hex) {
  if (!m_cursor->hasSelection())
    return;

  QClipboard *c = qApp->clipboard();
  QByteArray bytes = this->selectedBytes();

  if (hex)
    bytes = bytes.toHex(' ').toUpper();

  c->setText(bytes);
}

// modified by wingsummer
void QHexDocument::Paste(bool hex) {
  QClipboard *c = qApp->clipboard();
  QByteArray data = c->text().toUtf8();

  if (data.isEmpty())
    return;

  this->RemoveSelection();

  if (hex)
    data = QByteArray::fromHex(data);

  auto pos = m_cursor->position().offset();
  if (!m_keepsize) {
    this->Insert(pos, data);
    m_cursor->moveTo(pos + data.length()); // added by wingsummer
  } else
    this->Replace(pos, data);
}

void QHexDocument::Insert(qint64 offset, uchar b) {
  if (m_keepsize || m_readonly || m_islocked)
    return;
  this->Insert(offset, QByteArray(1, char(b)));
}

void QHexDocument::Replace(qint64 offset, uchar b) {
  if (m_readonly || m_islocked)
    return;
  this->Replace(offset, QByteArray(1, char(b)));
}

void QHexDocument::Insert(qint64 offset, const QByteArray &data) {
  if (m_keepsize || m_readonly || m_islocked)
    return;
  m_undostack.push(new InsertCommand(m_buffer, offset, data));
  emit documentChanged();
}

void QHexDocument::Replace(qint64 offset, const QByteArray &data) {
  if (m_readonly || m_islocked)
    return;
  m_undostack.push(new ReplaceCommand(m_buffer, offset, data));
  emit documentChanged();
}

bool QHexDocument::Remove(qint64 offset, int len) {
  if (m_keepsize || m_readonly || m_islocked)
    return false;
  m_undostack.push(new RemoveCommand(m_buffer, offset, len));
  emit documentChanged();
  return true;
}

QByteArray QHexDocument::read(qint64 offset, int len) const {
  return m_buffer->read(offset, len);
}

bool QHexDocument::saveTo(QIODevice *device, bool cleanUndo) {
  if (!device->isWritable())
    return false;

  m_buffer->write(device);
  if (cleanUndo)
    m_undostack.setClean(); // added by wingsummer

  m_pluginModed = false;
  return true;
}

qint64 QHexDocument::searchForward(const QByteArray &ba) {
  qint64 startPos = m_cursor->position().offset();
  qint64 findPos = m_buffer->indexOf(ba, startPos);
  if (findPos > -1) {
    m_cursor->clearSelection();
    m_cursor->moveTo(findPos);
    m_cursor->select(ba.length());
  }
  return findPos;
}

qint64 QHexDocument::searchBackward(const QByteArray &ba) {
  qint64 startPos = m_cursor->position().offset() - 1;
  if (m_cursor->hasSelection()) {
    startPos = m_cursor->selectionStart().offset() - 1;
  }
  qint64 findPos = m_buffer->lastIndexOf(ba, startPos);
  if (findPos > -1) {
    m_cursor->clearSelection();
    m_cursor->moveTo(findPos);
    m_cursor->select(ba.length());
  }
  return findPos;
}

QHexDocument *QHexDocument::fromLargeFile(QString filename, bool readonly,
                                          QObject *parent) {

  QFile *f = new QFile(filename);

  QHexBuffer *hexbuffer = new QFileBuffer();
  if (hexbuffer->read(f)) {
    return new QHexDocument(hexbuffer, readonly,
                            parent); // modified by wingsummer
  } else {
    delete hexbuffer;
  }

  return nullptr;
}
