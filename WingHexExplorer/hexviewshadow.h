#ifndef HEXVIEWSHADOW_H
#define HEXVIEWSHADOW_H

#include <QByteArray>
#include <QColor>
#include <QObject>
#include <QRect>

enum ErrFile {
  Success,
  Error,
  UnSaved,
  Permission,
  NotExist,
  AlreadyOpened,
  IsNewFile,
  IsDirver
};

struct FindResult {
  int fid;
  QList<int> indices;
};

class IWingPlugin;

struct HexPosition {
  quint64 line;
  int column;
  quint8 lineWidth;
  int nibbleindex;

  HexPosition() = default;
  inline qint64 offset() const {
    return static_cast<qint64>(line * lineWidth) + column;
  }
  inline int operator-(const HexPosition &rhs) const {
    return int(this->offset() - rhs.offset());
  }
  inline bool operator==(const HexPosition &rhs) const {
    return (line == rhs.line) && (column == rhs.column) &&
           (nibbleindex == rhs.nibbleindex);
  }
  inline bool operator!=(const HexPosition &rhs) const {
    return (line != rhs.line) || (column != rhs.column) ||
           (nibbleindex != rhs.nibbleindex);
  }
};

struct HexMetadataAbsoluteItem {
  qint64 begin;
  qint64 end;
  QColor foreground, background;
  QString comment;

  // added by wingsummer
  bool operator==(const HexMetadataAbsoluteItem &item) {
    return begin == item.begin && end == item.end &&
           foreground == item.foreground && background == item.background &&
           comment == item.comment;
  }

  HexMetadataAbsoluteItem(qint64 begin, qint64 end, QColor foreground,
                          QColor background, QString comment) {
    this->begin = begin;
    this->end = end;
    this->foreground = foreground;
    this->background = background;
    this->comment = comment;
  }
};

struct HexMetadataItem {
  quint64 line;
  int start, length;
  QColor foreground, background;
  QString comment;

  // added by wingsummer
  bool operator==(const HexMetadataItem &item) {
    return line == item.line && start == item.start &&
           foreground == item.foreground && background == item.background &&
           comment == item.comment;
  }

  HexMetadataItem(quint64 line, int start, int length, QColor foreground,
                  QColor background, QString comment) {
    this->line = line;
    this->start = start;
    this->length = length;
    this->foreground = foreground;
    this->background = background;
    this->comment = comment;
  }
};

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
typedef QLinkedList<HexMetadataItem> HexLineMetadata;
#else
typedef QList<HexMetadataItem> HexLineMetadata;
#endif

class HexViewShadow : public QObject {
  Q_OBJECT

public:
  HexViewShadow(QObject *parent = nullptr);
signals:
  // document
  void switchDocument(int index, bool gui = false);
  bool setLockedFile(bool b);
  bool setKeepSize(bool b);
  void setAsciiVisible(bool b);
  void setAddressVisible(bool b);
  void setHeaderVisible(bool b);
  void setAddressBase(quint64 base);

  bool isReadOnly();
  bool isKeepSize();
  bool isLocked();
  quint64 documentLines();
  quint64 documentBytes();
  HexPosition currentPos();
  HexPosition selectionPos();
  quint64 currentRow();
  quint64 currentColumn();
  quint64 currentOffset();
  quint64 selectlength();

  bool asciiVisible();
  bool addressVisible();
  bool headerVisible();
  quint64 addressBase();
  bool isModified();

  bool isEmpty();
  bool atEnd();
  bool canUndo();
  bool canRedo();
  int areaIndent();
  void setAreaIndent(quint8 value);
  int hexLineWidth();
  void setHexLineWidth(quint8 value);

  void undo();
  void redo();
  void cut(bool hex = false);
  void copy(bool hex = false);
  void paste(bool hex = false);
  void insert(qint64 offset, uchar b);
  void replace(qint64 offset, uchar b);
  void insert(qint64 offset, const QByteArray &data);
  void replace(qint64 offset, const QByteArray &data);
  void remove(qint64 offset, int len);
  QByteArray read(qint64 offset, int len);
  qint64 searchForward(const QByteArray &ba);
  qint64 searchBackward(const QByteArray &ba);
  void FindAllBytes(QByteArray b, QList<quint64> &results);

  // render
  bool editableArea(int area);
  quint64 documentLastLine();
  int documentLastColumn();
  int documentWidth();
  int lineHeight();
  QRect getLineRect(quint64 line, quint64 firstline);
  int headerLineCount();
  int borderSize();

  // cursor
  void moveTo(const HexPosition &pos);
  void moveTo(quint64 line, int column, int nibbleindex = 1);
  void moveTo(qint64 offset);
  void select(const HexPosition &pos);
  void select(quint64 line, int column, int nibbleindex = 1);
  void select(int length);
  void selectOffset(qint64 offset, int length);
  void setInsertionMode(bool isinsert);
  void setLineWidth(quint8 width);

  // metadata
  void metadata(qint64 begin, qint64 end, const QColor &fgcolor,
                const QColor &bgcolor, const QString &comment);
  void metadata(quint64 line, int start, int length, const QColor &fgcolor,
                const QColor &bgcolor, const QString &comment);
  bool lineHasMetadata(quint64 line) const;
  bool removeMetadata(qint64 offset, QList<HexMetadataItem> refer);
  QList<HexMetadataItem> getMetadatas(qint64 offset);
  void clear(quint64 line);
  void clear();
  HexLineMetadata getMetaLine(quint64 line) const;
  void color(quint64 line, int start, int length, const QColor &fgcolor,
             const QColor &bgcolor);
  void foreground(quint64 line, int start, int length, const QColor &fgcolor);
  void background(quint64 line, int start, int length, const QColor &bgcolor);
  void comment(quint64 line, int start, int length, const QString &comment);

  // shadow
  bool shadowIsValid(IWingPlugin *plugin);
  bool shadowControl(IWingPlugin *plugin);
  bool shadowRelease(IWingPlugin *plugin);
  void shadowDestory(IWingPlugin *plugin);

  // mainwindow
  void newFile();
  ErrFile openFile(QString filename, bool readonly = false);
  ErrFile openDriver(QString driver);
  ErrFile closeFile(int index, bool force = false);
  ErrFile saveFile(int index);
  ErrFile exportFile(QString filename, int index);
  void exportFileGUI();
  ErrFile saveasFile(QString filename, int index);
  void saveasFileGUI();
  ErrFile closeCurrentFile(bool force = false);
  ErrFile saveCurrentFile();
  void openFileGUI();
  void openDriverGUI();
  void findGUI();
  void gotoGUI();
  void fillGUI();
  void fillzeroGUI();
  void fillnopGUI();

  // extension
  QList<QString> getOpenFiles();
};

#endif // HEXVIEWSHADOW_H
