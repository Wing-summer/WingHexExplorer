#ifndef IWINGPLUGIN_H
#define IWINGPLUGIN_H

#include <QCryptographicHash>
#include <QDockWidget>
#include <QList>
#include <QMenu>
#include <QObject>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>
#include <QtCore>

#define SDKVERSION 6
#define GETPLUGINQM(name)                                                      \
  (QCoreApplication::applicationDirPath() + "/plglang/" + name)
#define PLUGINDIR (QCoreApplication::applicationDirPath() + "/plugin")

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

  FindResult() {}
};

struct BookMark {
  qlonglong pos;
  QString comment;
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

  HexMetadataAbsoluteItem() = default;

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

  HexMetadataItem() = default;

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
  NewFileEnd = 128,
  DocumentSwitched = 256
};

Q_DECLARE_METATYPE(WingPluginMessage)
Q_DECLARE_METATYPE(ResponseMsg)
Q_DECLARE_METATYPE(HookIndex)
Q_DECLARE_METATYPE(ErrFile)

namespace WingPlugin {
class Reader : public QObject {
  Q_OBJECT
signals:
  int currentDoc();
  QString currentDocFilename();

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
  quint64 selectLength();

  bool stringVisible();
  bool addressVisible();
  bool headerVisible();
  quint64 addressBase();
  bool isModified();

  bool isEmpty();
  bool atEnd();
  bool canUndo();
  bool canRedo();

  void copy(bool hex = false);
  QByteArray read(qint64 offset, int len);
  qint64 searchForward(qint64 begin, const QByteArray &ba);
  qint64 searchBackward(qint64 begin, const QByteArray &ba);
  void findAllBytes(qlonglong begin, qlonglong end, QByteArray b,
                    QList<quint64> &results, int maxCount = -1);

  // render
  quint64 documentLastLine();
  int documentLastColumn();

  // metadata
  bool lineHasMetadata(quint64 line) const;
  QList<HexMetadataAbsoluteItem> getMetadatas(qint64 offset);
  HexLineMetadata getMetaLine(quint64 line) const;

  // bookmark
  bool lineHasBookMark(quint64 line);
  QList<qint64> getsBookmarkPos(quint64 line);
  BookMark bookMark(qint64 pos);
  QString bookMarkComment(qint64 pos);
  void getBookMarks(QList<BookMark> &bookmarks);
  bool existBookMark(qint64 pos);

  // extension
  QList<QString> getOpenFiles();
  QStringList getSupportedEncodings();
  QString currentEncoding();
};

class Controller : public QObject {
  Q_OBJECT
signals:
  // document
  bool switchDocument(int index, bool gui = false);
  bool setLockedFile(bool b);
  bool setKeepSize(bool b);
  void setStringVisible(bool b);
  void setAddressVisible(bool b);
  void setHeaderVisible(bool b);
  void setAddressBase(quint64 base);

  void undo();
  void redo();
  bool cut(bool hex = false);
  void paste(bool hex = false);
  bool insert(qint64 offset, uchar b);
  bool replace(qint64 offset, uchar b);
  bool insert(qint64 offset, const QByteArray &data);
  bool replace(qint64 offset, const QByteArray &data);
  bool remove(qint64 offset, int len);

  // cursor
  void moveTo(const HexPosition &pos);
  void moveTo(quint64 line, int column, int nibbleindex = 1);
  void moveTo(qint64 offset);
  void select(quint64 line, int column, int nibbleindex = 1);
  void selectOffset(qint64 offset, int length);
  void setInsertionMode(bool isinsert);
  void enabledCursor(bool b);
  void select(qint64 offset, int length);

  // metadata
  bool metadata(qint64 begin, qint64 end, const QColor &fgcolor,
                const QColor &bgcolor, const QString &comment);
  bool metadata(quint64 line, int start, int length, const QColor &fgcolor,
                const QColor &bgcolor, const QString &comment);
  bool removeMetadata(qint64 offset);
  bool clearMeta();
  bool color(quint64 line, int start, int length, const QColor &fgcolor,
             const QColor &bgcolor);
  bool foreground(quint64 line, int start, int length, const QColor &fgcolor);
  bool background(quint64 line, int start, int length, const QColor &bgcolor);
  bool comment(quint64 line, int start, int length, const QString &comment);
  void applyMetas(QList<HexMetadataAbsoluteItem> metas);
  bool setMetaVisible(bool b);
  void setMetafgVisible(bool b);
  void setMetabgVisible(bool b);
  void setMetaCommentVisible(bool b);

  // mainwindow
  void newFile(bool bigfile = false);
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

  // bookmark
  bool addBookMark(qint64 pos, QString comment);
  bool modBookMark(qint64 pos, QString comment);
  void applyBookMarks(QList<BookMark> books);
  bool removeBookMark(qint64 pos);
  bool clearBookMark();

  // workspace
  bool openWorkSpace(QString filename, bool readonly = false);
  bool setCurrentEncoding(QString encoding);
};
} // namespace WingPlugin

struct WingPluginInfo {
  QString pluginName;
  QString pluginAuthor;
  uint pluginVersion;
  QString puid;
  QString pluginComment;
};

#define WINGSUMMER "wingsummer"

class IWingPlugin : public QObject {
  Q_OBJECT
public:
  virtual int sdkVersion() = 0;
  virtual QString signature() = 0;
  QString puid() { return GetPUID(this); }
  virtual ~IWingPlugin() {}
  virtual QMenu *registerMenu() { return nullptr; }
  virtual QToolButton *registerToolButton() { return nullptr; }
  virtual void
  registerDockWidget(QMap<QDockWidget *, Qt::DockWidgetArea> &rdw) {
    Q_UNUSED(rdw);
  }
  virtual QToolBar *registerToolBar() { return nullptr; }
  virtual Qt::ToolBarArea registerToolBarArea() {
    return Qt::ToolBarArea::TopToolBarArea;
  }
  virtual bool init(QList<WingPluginInfo> loadedplugin) = 0;
  virtual void unload() = 0;
  virtual QString pluginName() = 0;
  virtual QString pluginAuthor() = 0;
  virtual uint pluginVersion() = 0;
  virtual QString pluginComment() = 0;
  virtual HookIndex getHookSubscribe() { return HookIndex::None; }

  static QString GetPUID(IWingPlugin *plugin) {
    auto str = QString("%1%2%3%4")
                   .arg(WINGSUMMER)
                   .arg(plugin->pluginName())
                   .arg(plugin->pluginAuthor())
                   .arg(plugin->pluginVersion());
    return QCryptographicHash::hash(str.toLatin1(), QCryptographicHash::Md5)
        .toHex();
  }

public slots:
  virtual void plugin2MessagePipe(WingPluginMessage type,
                                  QList<QVariant> msg) = 0;

signals:
  bool requestControl(int timeout = 1500);
  bool requestRelease();
  bool hasControl();
  QWidget *getParentWindow();

  // extension
  void toast(QIcon icon, QString message);

public:
  WingPlugin::Reader reader;
  WingPlugin::Controller controller;
};

#define PluginToolButtonInit(tbtn, menu, icon)                                 \
  tbtn = new QToolButton;                                                      \
  tbtn->setMenu(menu);                                                         \
  tbtn->setIcon(icon);                                                         \
  tbtn->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);

#define PluginDockWidgetInit(dw, widget, title, objname)                       \
  dw = new QDockWidget;                                                        \
  dw->setWidget(widget);                                                       \
  dw->setWindowTitle(title);                                                   \
  dw->setObjectName(objname);

#define PluginToolBarInitBegin(toolbar, objname)                               \
  toolbar = new QToolBar;                                                      \
  toolbar->setObjectName(objname);                                             \
  {                                                                            \
    QAction *a;

#define PluginToolBarAddAction(toolbar, Icon, Slot, ToolTip)                   \
  a = new QAction(toolbar);                                                    \
  a->setIcon(Icon);                                                            \
  connect(a, &QAction::triggered, this, &Slot);                                \
  a->setToolTip(ToolTip);                                                      \
  toolbar->addAction(a);

#define PluginToolBarAddLamba(toolbar, Icon, Lamba, ToolTip)                   \
  a = new QAction(toolbar);                                                    \
  a->setIcon(Icon);                                                            \
  connect(a, &QAction::triggered, this, Lamba);                                \
  a->setToolTip(ToolTip);                                                      \
  toolbar->addAction(a);

#define PluginToolBarAddToolBtnBegin(DIcon)                                    \
  {                                                                            \
    auto tbtn = new QToolButton;                                               \
    tbtn->setIcon(DIcon);                                                      \
    auto tmenu = new QMenu;

#define PluginToolBarAddToolBtnAction(Icon, Title, Slot)                       \
  a = new QAction(Icon, Title);                                                \
  connect(a, &QAction::triggered, this, &Slot);                                \
  tmenu->addAction(a);

#define PluginToolBarAddToolBtnLamba(Icon, Title, Lamba)                       \
  a = new QAction(Icon, Title);                                                \
  connect(a, &QAction::triggered, this, Lamba);                                \
  tmenu->addAction(a);

#define PluginToolBarAddToolBtnEnd(toolbar)                                    \
  tbtn->setMenu(tmenu);                                                        \
  tbtn->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);          \
  toolbar->addWidget(tbtn);                                                    \
  }

#define PluginToolBarInitEnd() }

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

#define PluginMenuAddItemCheckAction(menu, title, checked, slot)               \
  a = new QAction(icon, title, this);                                          \
  a->setCheckable(true);                                                       \
  a->setChecked(checked);                                                      \
  connect(a, &QAction::triggered, this, &slot);                                \
  menu->addAction(a);

#define PluginMenuAddItemCheckLamba(menu, title, checked, lamba)               \
  a = new QAction(title, this);                                                \
  a->setCheckable(true);                                                       \
  a->setChecked(checked);                                                      \
  connect(a, &QAction::triggered, this, lamba);                                \
  menu->addAction(a);

#define PluginMenuInitEnd() }

#define USINGCONTROL(Segment)                                                  \
  if (this->requestControl()) {                                                \
    Segment;                                                                   \
    this->requestRelease();                                                    \
  }

#define IWINGPLUGIN_INTERFACE_IID "com.wingsummer.iwingplugin"
Q_DECLARE_INTERFACE(IWingPlugin, IWINGPLUGIN_INTERFACE_IID)

#endif // IWINGPLUGIN_H
