#ifndef QHEXDOCUMENT_H
#define QHEXDOCUMENT_H

#include "buffer/qhexbuffer.h"
#include "qhexcursor.h"
#include "qhexmetadata.h"
#include <QFile>
#include <QMap>
#include <QUndoStack>

/*=========================*/
// added by wingsummer

struct BookMarkStruct {
  qlonglong pos;
  QString comment;
};

enum class BookMarkModEnum { Insert, Modify, Remove, Apply, Clear };

/*=========================*/

class QHexDocument : public QObject {
  Q_OBJECT

private:
  explicit QHexDocument(QHexBuffer *buffer, bool readonly = false,
                        QObject *parent = nullptr); // modified by wingsummer

public:
  bool isEmpty() const;
  bool atEnd() const;
  bool canUndo() const;
  bool canRedo() const;
  qint64 length() const;
  quint64 baseAddress() const;
  QHexCursor *cursor() const;
  QHexMetadata *metadata() const;
  int areaIndent() const;
  void setAreaIndent(quint8 value);
  int hexLineWidth() const;
  void setHexLineWidth(quint8 value);

  /*======================*/
  // added by wingsummer

  void addUndoCommand(QUndoCommand *command);
  bool lineHasBookMark(quint64 line);
  QList<qint64> getsBookmarkPos(quint64 line);

  bool setLockedFile(bool b);
  bool setKeepSize(bool b);
  bool isReadOnly();
  bool isKeepSize();
  bool isLocked();

  //----------------------------------
  bool AddBookMark(qint64 pos, QString comment);
  bool RemoveBookMark(int index);
  bool ModBookMark(qint64 pos, QString comment);
  bool ClearBookMark();
  //----------------------------------

  bool addBookMark(qint64 pos, QString comment);
  bool modBookMark(qint64 pos, QString comment);
  BookMarkStruct bookMark(int index);
  BookMarkStruct bookMark(qint64 pos);
  QString bookMarkComment(qint64 pos);
  QList<BookMarkStruct> getAllBookMarks();
  void applyBookMarks(QList<BookMarkStruct> books);
  bool removeBookMark(int index);
  bool removeBookMark(qint64 pos);
  bool clearBookMark();
  void getBookMarks(QList<BookMarkStruct> &bookmarks);
  void gotoBookMark(int index);
  bool existBookMark(int &index);
  bool existBookMark();
  bool existBookMark(qint64 pos);

  void findAllBytes(qint64 begin, qint64 end, QByteArray b,
                    QList<quint64> &results, int maxCount = -1);
  bool isDocSaved();
  void setDocSaved(bool b = true);

  bool isWorkspace = false;

  bool setMetafgVisible(bool b);
  bool setMetabgVisible(bool b);
  bool setMetaCommentVisible(bool b);

  bool SetMetafgVisible(bool b);
  bool SetMetabgVisible(bool b);
  bool SetMetaCommentVisible(bool b);
  bool SetMetaVisible(bool b);

  bool metafgVisible();
  bool metabgVisible();
  bool metaCommentVisible();

  /*======================*/

public:
  bool RemoveSelection();
  QByteArray read(qint64 offset, int len = 0);
  QByteArray selectedBytes() const;
  char at(int offset) const;
  void SetBaseAddress(quint64 baseaddress);
  void setBaseAddress(quint64 baseaddress);
  void sync();

public slots:
  void undo();
  void redo();
  bool Cut(bool hex = false);
  void copy(bool hex = false);
  void Paste(bool hex = false);
  void Insert(qint64 offset, uchar b);
  void Insert(qint64 offset, const QByteArray &data);
  void Replace(qint64 offset, uchar b);
  void Replace(qint64 offset, const QByteArray &data);
  bool Remove(qint64 offset, int len);
  QByteArray read(qint64 offset, int len) const;
  bool saveTo(QIODevice *device, bool cleanUndo);

  qint64 searchForward(const QByteArray &ba);
  qint64 searchBackward(const QByteArray &ba);

  /*================================*/
  // added by wingsummer

  bool cut(bool hex = false);
  void paste(bool hex = false);
  bool insert(qint64 offset, uchar b);
  bool insert(qint64 offset, const QByteArray &data);
  bool replace(qint64 offset, uchar b);
  bool replace(qint64 offset, const QByteArray &data);
  bool remove(qint64 offset, int len);
  bool removeSelection();

  /*================================*/

  /*================================*/
  // modified by wingsummer

public:
  template <typename T>
  static QHexDocument *fromDevice(QIODevice *iodevice, bool readonly = false,
                                  QObject *parent = nullptr);
  template <typename T>
  static QHexDocument *fromFile(QString filename, bool readonly = false,
                                QObject *parent = nullptr);
  template <typename T>
  static QHexDocument *fromMemory(char *data, int size, bool readonly = false,
                                  QObject *parent = nullptr);
  template <typename T>
  static QHexDocument *fromMemory(const QByteArray &ba, bool readonly = false,
                                  QObject *parent = nullptr);
  static QHexDocument *fromLargeFile(QString filename, bool readonly = false,
                                     QObject *parent = nullptr);

signals:

  /*================================*/

  /*================================*/
  // added by wingsummer

  void documentSaved(bool saved);
  void bookMarkChanged(BookMarkModEnum flag, int index, qint64 pos,
                       QString comment);
  void metafgVisibleChanged(bool b);
  void metabgVisibleChanged(bool b);
  void metaCommentVisibleChanged(bool b);

  /*================================*/

  void canUndoChanged(bool canUndo);
  void canRedoChanged(bool canRedo);
  void documentChanged();
  void metaLineChanged(quint64 line);

private:
  QHexBuffer *m_buffer;
  QHexMetadata *m_metadata;
  QUndoStack m_undostack;
  QHexCursor *m_cursor;
  quint64 m_baseaddress;
  quint8 m_areaindent;
  quint8 m_hexlinewidth;

  /*======================*/
  // added by wingsummer

  bool m_readonly;
  bool m_keepsize;
  bool m_islocked;
  QList<BookMarkStruct> bookmarks;
  bool m_pluginModed = false;

  bool m_metafg = true;
  bool m_metabg = true;
  bool m_metacomment = true;

  /*======================*/
};

/*====================================*/
// modified by wingsummer

template <typename T>
QHexDocument *QHexDocument::fromDevice(QIODevice *iodevice, bool readonly,
                                       QObject *parent) {
  bool needsclose = false;

  if (!iodevice->isOpen()) {
    needsclose = true;
    iodevice->open(readonly ? QIODevice::ReadOnly
                            : QIODevice::ReadWrite); // modifed by wingsummer
  }

  QHexBuffer *hexbuffer = new T();
  if (hexbuffer->read(iodevice)) {
    if (needsclose)
      iodevice->close();

    return new QHexDocument(hexbuffer, readonly, parent);
  } else {
    delete hexbuffer;
  }

  return nullptr;
}

template <typename T>
QHexDocument *QHexDocument::fromFile(QString filename, bool readonly,
                                     QObject *parent) {
  QFile f(filename);
  f.open(QFile::ReadOnly);

  QHexDocument *doc = QHexDocument::fromDevice<T>(&f, readonly, parent);
  f.close();
  return doc;
}

template <typename T>
QHexDocument *QHexDocument::fromMemory(char *data, int size, bool readonly,
                                       QObject *parent) {
  QHexBuffer *hexbuffer = new T();
  hexbuffer->read(data, size);
  return new QHexDocument(hexbuffer, readonly, parent);
}

template <typename T>
QHexDocument *QHexDocument::fromMemory(const QByteArray &ba, bool readonly,
                                       QObject *parent) {
  QHexBuffer *hexbuffer = new T();
  hexbuffer->read(ba);
  return new QHexDocument(hexbuffer, readonly, parent);
}

/*================================================*/

#endif // QHEXEDITDATA_H
