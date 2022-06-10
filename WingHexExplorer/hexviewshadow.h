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

class HexViewShadow : public QObject {
  Q_OBJECT

public:
  HexViewShadow(QObject *parent = nullptr) : QObject(parent) {}
signals:
  // document
  void switchDocument(int index);
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

  // shadow
  bool shadowIsValid(IWingPlugin *plugin);
  bool shadowControl(IWingPlugin *plugin, HexViewShadow *shadow);
  bool shadowRelease(IWingPlugin *plugin, HexViewShadow *shadow);
  void shadowDestory(IWingPlugin *plugin);

  // mainwindow
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
};

//#define IWINGPLUGIN_SHADOWINTERFACE_IID "wingsummer.iwingplugin.shadow"
// Q_DECLARE_INTERFACE(HexViewShadow, IWINGPLUGIN_SHADOWINTERFACE_IID)

#endif // HEXVIEWSHADOW_H
