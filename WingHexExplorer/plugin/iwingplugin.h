#ifndef IWINGPLUGIN_H
#define IWINGPLUGIN_H

#include <QCryptographicHash>
#include <QDockWidget>
#include <QList>
#include <QMenu>
#include <QObject>
#include <QWidget>
#include <QtCore>

enum ErrFile {
  Success,
  Error,
  UnSaved,
  Permission,
  NotExist,
  AlreadyOpened,
  IsNewFile,
  IsDirver,
  WorkSpaceUnSaved
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

enum class WingPluginMessage {
  PluginLoading,
  PluginLoaded,
  PluginUnLoading,
  PluginUnLoaded,
  ErrorMessage,
  ConnectTimeout,
  MessageResponse,
  HookMessage
};

enum class ResponseMsg { UnImplement, Success, ErrorParams, Permission };

enum HookIndex {
  None = 0,
  OpenFileBegin = 1,
  OpenFileEnd = 2,
  OpenDriverBegin = 4,
  OpenDriverEnd = 8,
  CloseFileBegin = 16,
  CloseFileEnd = 32,
  NewFileBegin = 64,
  NewFileEnd = 128
};

Q_DECLARE_METATYPE(WingPluginMessage)
Q_DECLARE_METATYPE(ResponseMsg)
Q_DECLARE_METATYPE(HookIndex)

namespace WingPlugin {
class Reader : public QObject {
  Q_OBJECT
signals:
  // document
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
  int hexLineWidth();

  void copy(bool hex = false);
  QByteArray read(qint64 offset, int len);
  qint64 searchForward(const QByteArray &ba);
  qint64 searchBackward(const QByteArray &ba);
  void findAllBytes(qlonglong begin, qlonglong end, QByteArray b,
                    QList<quint64> &results);

  // render
  bool editableArea(int area);
  quint64 documentLastLine();
  int documentLastColumn();
  int documentWidth();
  int lineHeight();
  QRect getLineRect(quint64 line, quint64 firstline);
  int headerLineCount();
  int borderSize();

  // metadata
  bool lineHasMetadata(quint64 line) const;
  QList<HexMetadataAbsoluteItem> getMetadatas(qint64 offset);
  HexLineMetadata getMetaLine(quint64 line) const;

  // extension
  QList<QString> getOpenFiles();
  QStringList getSupportedEncodings();
  QString currentEncoding();
};

class Controller : public QObject {
  Q_OBJECT
signals:
  // document
  void switchDocument(int index, bool gui = false);
  bool setLockedFile(bool b);
  bool setKeepSize(bool b);
  void setAsciiVisible(bool b);
  void setAddressVisible(bool b);
  void setHeaderVisible(bool b);
  void setAddressBase(quint64 base);
  void setAreaIndent(quint8 value);
  void setHexLineWidth(quint8 value);

  void undo();
  void redo();
  void cut(bool hex = false);
  void paste(bool hex = false);
  void insert(qint64 offset, uchar b);
  void replace(qint64 offset, uchar b);
  void insert(qint64 offset, const QByteArray &data);
  void replace(qint64 offset, const QByteArray &data);
  void remove(qint64 offset, int len);

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
  bool removeMetadata(qint64 offset);
  void clearMeta();
  void color(quint64 line, int start, int length, const QColor &fgcolor,
             const QColor &bgcolor);
  void foreground(quint64 line, int start, int length, const QColor &fgcolor);
  void background(quint64 line, int start, int length, const QColor &bgcolor);
  void comment(quint64 line, int start, int length, const QString &comment);

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
  void setCurrentEncoding(QString encoding);

  // workspace
  bool openWorkSpace(QString filename, bool readonly = false);
};
} // namespace WingPlugin

class IWingPlugin : public QObject {
  Q_OBJECT
public:
  virtual bool init(QList<IWingPlugin *> loadedplugins) = 0;
  virtual ~IWingPlugin() {}
  virtual void unload() = 0;
  virtual QMenu *registerMenu() = 0;
  virtual QDockWidget *registerDockWidget() = 0;
  virtual Qt::DockWidgetArea registerDockWidgetDockArea() = 0;
  virtual QString pluginName() = 0;
  virtual QString pluginAuthor() = 0;
  virtual uint pluginVersion() = 0;
  virtual QString puid() = 0;
  virtual QString signature() = 0;
  virtual QString pluginComment() = 0;
  virtual QList<QVariant> optionalInfos() = 0;
  virtual HookIndex getHookSubscribe() = 0;

public slots:
  virtual void plugin2MessagePipe(WingPluginMessage type,
                                  QList<QVariant> msg) = 0;

signals:
  bool requestControl(IWingPlugin *plugin);
  bool requestRelease(IWingPlugin *plugin);

public:
  WingPlugin::Reader reader;
  WingPlugin::Controller controller;
};

#define WINGSUMMER "wingsummer"
#define PluginDockWidgetInit(dw, widget, title, objname)                       \
  dw = new QDockWidget;                                                        \
  dw->setWidget(widget);                                                       \
  dw->setWindowTitle(title);                                                   \
  dw->setObjectName(objname);

#define PluginWidgetFree(w) w->deleteLater();

#define PluginMenuInitBegin(menu, title)                                       \
  menu = new QMenu;                                                            \
  menu->setTitle(title);                                                       \
  {                                                                            \
    QAction *a;

#define PluginMenuAddItemAction(menu, title, slot)                             \
  a = new QAction(title, this);                                                \
  connect(a, &QAction::triggered, this, &slot);                                \
  menu->addAction(a);

#define PluginMenuAddItemLamba(menu, title, lamba)                             \
  a = new QAction(title, this);                                                \
  connect(a, &QAction::triggered, this, lamba);                                \
  menu->addAction(a);

#define PluginMenuAddItemIconAction(menu, title, icon, slot)                   \
  a = new QAction(icon, title, this);                                          \
  connect(a, &QAction::triggered, this, &slot);                                \
  menu->addAction(a);

#define PluginMenuAddItemIconLamba(menu, title, icon, lamba)                   \
  a = new QAction(icon, title, this);                                          \
  connect(a, &QAction::triggered, this, lamba);                                \
  menu->addAction(a);

#define PluginMenuInitEnd() }

class PluginUtils {
public:
  static QString GetPUID(IWingPlugin *plugin) {
    auto str = QString("%1%2%3%4%5")
                   .arg(WINGSUMMER)
                   .arg(plugin->pluginName())
                   .arg(plugin->pluginAuthor())
                   .arg(plugin->pluginComment())
                   .arg(plugin->pluginVersion());
    return QCryptographicHash::hash(str.toLatin1(), QCryptographicHash::Md5)
        .toHex();
  }

  static QString GetPuid(QString pluginName, QString author, QString comment,
                         uint version) {
    auto str = QString("%1%2%3%4%5")
                   .arg(WINGSUMMER)
                   .arg(pluginName)
                   .arg(author)
                   .arg(comment)
                   .arg(version);
    return QCryptographicHash::hash(str.toLatin1(), QCryptographicHash::Md5)
        .toHex();
  }
};

#define IWINGPLUGIN_INTERFACE_IID "com.wingsummer.iwingplugin"
Q_DECLARE_INTERFACE(IWingPlugin, IWINGPLUGIN_INTERFACE_IID)

#endif // IWINGPLUGIN_H
