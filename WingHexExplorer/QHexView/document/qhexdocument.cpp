#include "qhexdocument.h"
#include "buffer/qfilebuffer.h"
#include "commands/insertcommand.h"
#include "commands/removecommand.h"
#include "commands/replacecommand.h"
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFile>

/*======================*/
// added by wingsummer

bool QHexDocument::isReadOnly() { return m_readonly; }
bool QHexDocument::isKeepSize() { return m_keepsize; }
bool QHexDocument::isLocked() { return m_islocked; }
bool QHexDocument::setLockedFile(bool b) {
  if (m_readonly)
    return false;
  m_islocked = b;
  m_cursor->setInsertionMode(QHexCursor::OverwriteMode);
  return true;
}
bool QHexDocument::setKeepSize(bool b) {
  if (m_readonly)
    return false;
  m_keepsize = b;
  if (b)
    m_cursor->setInsertionMode(QHexCursor::OverwriteMode);
  return true;
}

bool QHexDocument::isSaved() { return m_undostack.isClean(); }

void QHexDocument::getBookMarks(QList<BookMarkStruct> &bookmarks) {
  bookmarks.clear();
  bookmarks.append(this->bookmarks);
}

void QHexDocument::addBookMark(QString comment) {
  BookMarkStruct b{m_cursor->position().offset(), comment};
  bookmarks.append(b);
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

void QHexDocument::removeBookMark(int index) {
  if (index >= 0 && index < bookmarks.count()) {
    bookmarks.removeAt(index);
  }
}

void QHexDocument::clearBookMark() { bookmarks.clear(); }

void QHexDocument::gotoBookMark(int index) {
  if (index >= 0 && index < bookmarks.count()) {
    auto bookmark = bookmarks.at(index);
    m_cursor->moveTo(qlonglong(bookmark.pos));
  }
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
}

void QHexDocument::FindAllBytes(qint64 begin, qint64 end, QByteArray b,
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

  connect(m_metadata, &QHexMetadata::metadataChanged, this,
          &QHexDocument::lineChanged);
  connect(m_metadata, &QHexMetadata::metadataCleared, this,
          &QHexDocument::documentChanged);

  /*=======================*/
  // added by wingsummer
  connect(&m_undostack, &QUndoStack::canUndoChanged, this,
          &QHexDocument::canUndoChanged);
  connect(&m_undostack, &QUndoStack::canRedoChanged, this,
          &QHexDocument::canRedoChanged);
  connect(&m_undostack, &QUndoStack::cleanChanged, this,
          &QHexDocument::documentSaved);
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

bool QHexDocument::removeSelection() {
  if (!m_cursor->hasSelection())
    return false;

  auto res = this->remove(m_cursor->selectionStart().offset(),
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

bool QHexDocument::cut(bool hex) {
  if (!m_cursor->hasSelection() || m_keepsize)
    return false;

  this->copy(hex);
  return this->removeSelection();
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

void QHexDocument::replace(qint64 offset, uchar b) {
  if (m_readonly || m_islocked)
    return;
  this->replace(offset, QByteArray(1, char(b)));
}

void QHexDocument::insert(qint64 offset, const QByteArray &data) {
  if (m_keepsize || m_readonly || m_islocked)
    return;
  m_undostack.push(new InsertCommand(m_buffer, offset, data));
  emit documentChanged();
}

void QHexDocument::replace(qint64 offset, const QByteArray &data) {
  m_undostack.push(new ReplaceCommand(m_buffer, offset, data));
  emit documentChanged();
}

bool QHexDocument::remove(qint64 offset, int len) {
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
