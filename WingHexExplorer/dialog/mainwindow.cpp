#include "mainwindow.h"
#include "QHexView/document/buffer/qfilebuffer.h"
#include "QHexView/document/buffer/qmemorybuffer.h"
#include "QHexView/document/qhexcursor.h"
#include "QHexView/document/qhexmetadata.h"
#include "aboutsoftwaredialog.h"
#include "driverselectordialog.h"
#include "finddialog.h"
#include "logger.h"
#include "metadialog.h"
#include "pluginwindow.h"
#include "settings.h"
#include "sponsordialog.h"
#include <DAnchors>
#include <DInputDialog>
#include <DMenuBar>
#include <DMessageManager>
#include <DSettingsDialog>
#include <DSettingsWidgetFactory>
#include <DTitlebar>
#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QScrollBar>
#include <QShortcut>
#include <QStorageInfo>
#include <QThread>

#define FILEMAXBUFFER 0x6400000 // 100MB

#define CheckEnabled                                                           \
  if (hexfiles.count() == 0)                                                   \
    return;

MainWindow::MainWindow(DMainWindow *parent) {
  Q_UNUSED(parent)

  // init mainwindow
  setMinimumSize(QSize(1200, 800));

  auto _title = titlebar();
  auto picon = ICONRES("icon");
  setWindowIcon(picon);
  _title->setIcon(picon);
  _title->setTitle("WingHexExplorer");

  tabs = new DTabBar;
  tabs->setTabsClosable(true);
  tabs->setMovable(true);
  tabs->setStartDragDistance(3);
  tabs->setTabLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  _title->setCustomWidget(tabs);

  connect(tabs, &DTabBar::currentChanged, this,
          &MainWindow::on_tabs_currentChanged);
  connect(tabs, &DTabBar::tabCloseRequested, this,
          &MainWindow::on_tabCloseRequested);
  connect(tabs, &DTabBar::tabAddRequested, this,
          &MainWindow::on_tabAddRequested);
  connect(tabs, &DTabBar::tabMoved, this, &MainWindow::on_tabMoved);

  w = new QWidget(this);
  setCentralWidget(w);
  vlayout = new QVBoxLayout(w);

  auto menu = new DMenu(this);
  toolmenu = menu;

#define AddMenuAction(Icon, Title, Slot, Owner)                                \
  a = new QAction(Owner);                                                      \
  a->setText(Title);                                                           \
  a->setIcon(ICONRES(Icon));                                                   \
  connect(a, &QAction::triggered, this, &Slot);                                \
  Owner->addAction(a);

#define AddToolSubMenuAction(Icon, Title, Slot)                                \
  AddMenuAction(Icon, Title, Slot, tm)

#define AddMenuShortcutAction(Icon, Title, Slot, Owner, ShortCut)              \
  AddMenuAction(Icon, Title, Slot, Owner);                                     \
  a->setShortcut(ShortCut);                                                    \
  a->setShortcutVisibleInContextMenu(true);

#define AddToolSubMenuShortcutAction(Icon, Title, Slot, ShortCut)              \
  AddMenuShortcutAction(Icon, Title, Slot, tm, ShortCut)

  QAction *a;

  auto tm = new DMenu(this);
  tm->setTitle(tr("File"));
  tm->setIcon(ICONRES("file"));
  AddToolSubMenuShortcutAction("new", tr("New"), MainWindow::on_newfile,
                               QKeySequence::New);

  AddToolSubMenuShortcutAction("open", tr("OpenF"), MainWindow::on_openfile,
                               QKeySequence::Open);

  auto keyOpenDriver =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_O);
  auto keygoto =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_G);
  auto keyGeneral =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_G);
  auto keyplugin = QKeySequence(Qt::KeyboardModifier::ControlModifier |
                                Qt::KeyboardModifier::AltModifier | Qt::Key_P);
  auto keymetadata =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_M);
  auto keymetadatadel =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::AltModifier | Qt::Key_M);
  auto keymetadatacls =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier |
                   Qt::KeyboardModifier::AltModifier | Qt::Key_M);
  auto keybookmark =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_B);
  auto keybookmarkdel =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::AltModifier | Qt::Key_B);
  auto keybookmarkcls =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier |
                   Qt::KeyboardModifier::AltModifier | Qt::Key_B);

  auto keyfillnop =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_9);
  auto keyfillzero =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_0);
  auto keyfill = QKeySequence(Qt::KeyboardModifier::ControlModifier |
                              Qt::KeyboardModifier::AltModifier | Qt::Key_F);

  AddToolSubMenuShortcutAction("opendriver", tr("OpenD"),
                               MainWindow::on_opendriver, keyOpenDriver);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("save", tr("Save"), MainWindow::on_savefile,
                               QKeySequence::Save);
  AddToolSubMenuShortcutAction("saveas", tr("SaveAs"),
                               MainWindow::on_saveasfile, QKeySequence::SaveAs);
  AddToolSubMenuAction("export", tr("Export"), MainWindow::on_exportfile);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("exit", tr("Exit"), MainWindow::on_exit,
                               QKeySequence::Quit);
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Edit"));
  tm->setIcon(ICONRES("edit"));
  tm->setEnabled(false);
  editmenu = tm;
  AddToolSubMenuShortcutAction("undo", tr("Undo"), MainWindow::on_undofile,
                               QKeySequence::Undo);
  AddToolSubMenuShortcutAction("redo", tr("Redo"), MainWindow::on_redofile,
                               QKeySequence::Redo);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("cut", tr("Cut"), MainWindow::on_cutfile,
                               QKeySequence::Cut);
  AddToolSubMenuShortcutAction("copy", tr("Copy"), MainWindow::on_copyfile,
                               QKeySequence::Copy);
  AddToolSubMenuShortcutAction("paste", tr("Paste"), MainWindow::on_pastefile,
                               QKeySequence::Paste);
  AddToolSubMenuShortcutAction("del", tr("Delete"), MainWindow::on_delete,
                               QKeySequence::Delete);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("find", tr("Find"), MainWindow::on_findfile,
                               QKeySequence::Find);
  AddToolSubMenuShortcutAction("jmp", tr("Goto"), MainWindow::on_gotoline,
                               keygoto);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("fill", tr("Fill"), MainWindow::on_fill,
                               keyfill);
  AddToolSubMenuShortcutAction("fillNop", tr("FillNop"), MainWindow::on_fillnop,
                               keyfillnop);
  AddToolSubMenuShortcutAction("fillZero", tr("FillZero"),
                               MainWindow::on_fillzero, keyfillzero);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("metadata", tr("MetaData"),
                               MainWindow::on_metadata, keymetadata);
  AddToolSubMenuShortcutAction("metadatadel", tr("DeleteMetaData"),
                               MainWindow::on_metadatadel, keymetadatadel);
  AddToolSubMenuShortcutAction("metadatacls", tr("ClearMetaData"),
                               MainWindow::on_metadatacls, keymetadatacls);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("bookmark", tr("BookMark"),
                               MainWindow::on_bookmark, keybookmark);
  AddToolSubMenuShortcutAction("bookmarkdel", tr("DeleteBookMark"),
                               MainWindow::on_bookmarkdel, keybookmarkdel);
  AddToolSubMenuShortcutAction("bookmarkcls", tr("ClearBookMark"),
                               MainWindow::on_bookmarkcls, keybookmarkcls);
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Setting"));
  tm->setIcon(ICONRES("setting"));
  AddToolSubMenuShortcutAction("general", tr("General"),
                               MainWindow::on_setting_general, keyGeneral);
  AddToolSubMenuShortcutAction("settingplugin", tr("Plugin"),
                               MainWindow::on_setting_plugin, keyplugin);
  AddToolSubMenuAction("layout", tr("RestoreLayout"),
                       MainWindow::on_restoreLayout);
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Plugin"));
  tm->setIcon(ICONRES("plugin"));
  plgmenu = tm;
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Author"));
  tm->setIcon(ICONRES("author"));
  AddToolSubMenuAction("soft", tr("About"), MainWindow::on_about);
  AddToolSubMenuAction("sponsor", tr("Sponsor"), MainWindow::on_sponsor);

  menu->addMenu(tm);

  titlebar()->setMenu(menu);

  hexeditorMenu = new DMenu(this);
  hexeditorMenu->setEnabled(false);

#define AddContextMenuAction(Icon, Title, Slot, ShortCut)                      \
  AddMenuShortcutAction(Icon, Title, Slot, hexeditorMenu, ShortCut)

  AddContextMenuAction("undo", tr("Undo"), MainWindow::on_undofile,
                       QKeySequence::Undo);
  AddContextMenuAction("redo", tr("Redo"), MainWindow::on_redofile,
                       QKeySequence::Redo);
  hexeditorMenu->addSeparator();
  AddContextMenuAction("cut", tr("Cut"), MainWindow::on_cutfile,
                       QKeySequence::Cut);
  AddContextMenuAction("copy", tr("Copy"), MainWindow::on_copyfile,
                       QKeySequence::Copy);
  AddContextMenuAction("paste", tr("Paste"), MainWindow::on_pastefile,
                       QKeySequence::Paste);
  AddContextMenuAction("del", tr("Delete"), MainWindow::on_delete,
                       QKeySequence::Delete);
  hexeditorMenu->addSeparator();
  AddContextMenuAction("find", tr("Find"), MainWindow::on_findfile,
                       QKeySequence::Find);
  AddContextMenuAction("jmp", tr("Goto"), MainWindow::on_gotoline, keygoto);
  hexeditorMenu->addSeparator();
  AddContextMenuAction("fill", tr("Fill"), MainWindow::on_fill, keyfill);
  AddContextMenuAction("fillNop", tr("FillNop"), MainWindow::on_fillnop,
                       keyfillnop);
  AddContextMenuAction("fillZero", tr("FillZero"), MainWindow::on_fillzero,
                       keyfillzero);
  hexeditorMenu->addSeparator();
  AddContextMenuAction("metadata", tr("MetaData"), MainWindow::on_metadata,
                       keymetadata);
  AddContextMenuAction("metadatadel", tr("DeleteMetaData"),
                       MainWindow::on_metadatadel, keymetadatadel);
  AddContextMenuAction("metadatacls", tr("ClearMetaData"),
                       MainWindow::on_metadatacls, keymetadatacls);
  hexeditorMenu->addSeparator();
  AddContextMenuAction("bookmark", tr("BookMark"), MainWindow::on_bookmark,
                       keybookmark);
  AddContextMenuAction("bookmarkdel", tr("DeleteBookMark"),
                       MainWindow::on_bookmarkdel, keybookmarkdel);
  AddContextMenuAction("bookmarkcls", tr("ClearBookMark"),
                       MainWindow::on_bookmarkcls, keybookmarkcls);
  toolbar = new DToolBar(this);

#define AddToolBarAction(Icon, Owner, Slot)                                    \
  a = new QAction(Owner);                                                      \
  a->setIcon(ICONRES(Icon));                                                   \
  connect(a, &QAction::triggered, this, &Slot);                                \
  Owner->addAction(a);

#define AddToolBarTool(Icon, Slot) AddToolBarAction(Icon, toolbar, Slot)

#define AddToolsDB(index)                                                      \
  a->setEnabled(false);                                                        \
  toolbartools.insert(index, a);

  AddToolBarTool("new", MainWindow::on_newfile);
  AddToolBarTool("open", MainWindow::on_openfile);
  AddToolBarTool("opendriver", MainWindow::on_opendriver);
  toolbar->addSeparator();
  AddToolBarTool("save", MainWindow::on_savefile);
  AddToolsDB(ToolBoxIndex::Save);
  AddToolBarTool("saveas", MainWindow::on_saveasfile);
  AddToolsDB(ToolBoxIndex::SaveAs);
  AddToolBarTool("export", MainWindow::on_exportfile);
  AddToolsDB(ToolBoxIndex::Export);
  toolbar->addSeparator();
  AddToolBarTool("undo", MainWindow::on_undofile);
  AddToolsDB(ToolBoxIndex::Undo);
  AddToolBarTool("redo", MainWindow::on_redofile);
  AddToolsDB(ToolBoxIndex::Redo);
  AddToolBarTool("cut", MainWindow::on_cutfile);
  AddToolsDB(ToolBoxIndex::Cut);
  AddToolBarTool("copy", MainWindow::on_copyfile);
  AddToolsDB(ToolBoxIndex::Copy);
  AddToolBarTool("paste", MainWindow::on_pastefile);
  AddToolsDB(ToolBoxIndex::Paste);
  AddToolBarTool("del", MainWindow::on_delete);
  AddToolsDB(ToolBoxIndex::Del);
  toolbar->addSeparator();
  AddToolBarTool("find", MainWindow::on_findfile);
  AddToolsDB(ToolBoxIndex::Find);
  AddToolBarTool("jmp", MainWindow::on_gotoline);
  AddToolsDB(ToolBoxIndex::Goto);
  toolbar->addSeparator();
  AddToolBarTool("fill", MainWindow::on_fill);
  AddToolsDB(ToolBoxIndex::Fill);
  AddToolBarTool("fillNop", MainWindow::on_fillnop);
  AddToolsDB(ToolBoxIndex::FillNop);
  AddToolBarTool("fillZero", MainWindow::on_fillzero);
  AddToolsDB(ToolBoxIndex::FillZero);
  toolbar->addSeparator();
  AddToolBarTool("metadata", MainWindow::on_metadata);
  AddToolsDB(ToolBoxIndex::Meta);
  AddToolBarTool("metadatadel", MainWindow::on_metadatadel);
  AddToolsDB(ToolBoxIndex::DelMeta);
  AddToolBarTool("metadatacls", MainWindow::on_metadatacls);
  AddToolsDB(ToolBoxIndex::ClsMeta);
  toolbar->addSeparator();
  AddToolBarTool("bookmark", MainWindow::on_bookmark);
  AddToolsDB(ToolBoxIndex::BookMark);
  AddToolBarTool("bookmarkdel", MainWindow::on_bookmarkdel);
  AddToolsDB(ToolBoxIndex::DelBookMark);
  AddToolBarTool("bookmarkcls", MainWindow::on_bookmarkcls);
  AddToolsDB(ToolBoxIndex::ClsBookMark);
  this->addToolBar(toolbar);

  hexeditor = new QHexView(this);
  hexeditor->setVisible(false);
  hexeditor->setAddressBase(_showaddr);
  hexeditor->setHeaderVisible(_showheader);
  hexeditor->setAsciiVisible(_showascii);
  vlayout->addWidget(hexeditor);
  hexeditor->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(hexeditor, &QHexView::customContextMenuRequested, this,
          &MainWindow::on_hexeditor_customContextMenuRequested);
  connect(hexeditor, &QHexView::documentSwitched, this,
          &MainWindow::on_documentSwitched);

  status = new DStatusBar(this);
  status->setEnabled(false);
  this->setStatusBar(status);

#define AddNamedStatusLabel(Var, Content)                                      \
  Var = new DLabel(this);                                                      \
  Var->setText(Content);                                                       \
  status->addWidget(Var);

#define AddStatusLabel(Content) AddNamedStatusLabel(l, Content)

#define AddPermanentNamedLable(Var, Content)                                   \
  Var = new DLabel(this);                                                      \
  Var->setText(Content);                                                       \
  status->addPermanentWidget(Var);

#define AddPermanentStatusLabel(Content) AddPermanentNamedLable(l, Content)

#define AddFunctionIconButton(Var, Icon)                                       \
  Var = new DIconButton(this);                                                 \
  Var->setIcon(ICONRES(Icon));                                                 \
  Var->setIconSize(QSize(20, 20));                                             \
  status->addPermanentWidget(Var);

  DLabel *l;

  AddFunctionIconButton(iSetBaseAddr, "mAddr");
  connect(iSetBaseAddr, &DIconButton::clicked, [=] {
    DInputDialog d;
    auto num = d.getText(this, tr("addressBase"), tr("inputAddressBase"));
    bool b = false;
    qulonglong qnum = num.toULongLong(&b, 0);
    if (b) {
      hexeditor->setAddressBase(qnum);
    } else {
      if (num.length() > 0) {
        auto d = DMessageManager::instance();
        d->sendMessage(this, ICONRES("mAddr"), tr("ErrBaseAddress"));
      }
    }
  });

  AddFunctionIconButton(iColInfo, "mColInfo");
  connect(iColInfo, &DIconButton::clicked,
          [=] { hexeditor->setAddressVisible(!hexeditor->addressVisible()); });
  AddFunctionIconButton(iHeaderInfo, "mLineInfo");
  connect(iHeaderInfo, &DIconButton::clicked,
          [=] { hexeditor->setHeaderVisible(!hexeditor->headerVisible()); });
  AddFunctionIconButton(iAsciiString, "mStr");
  connect(iAsciiString, &DIconButton::clicked,
          [=] { hexeditor->setAsciiVisible(!hexeditor->asciiVisible()); });

  AddPermanentStatusLabel(QString(2, ' '));
  AddStatusLabel(tr("loc:"));
  l->setMinimumWidth(50);
  l->setAlignment(Qt::AlignCenter);

  AddNamedStatusLabel(lblloc, "(0,0)");
  connect(hexeditor, &QHexView::cursorLocationChanged, this,
          &MainWindow::on_locChanged);
  connect(hexeditor, &QHexView::documentChanged, this,
          &MainWindow::on_documentChanged);
  connect(hexeditor, &QHexView::documentStatusChanged, this,
          &MainWindow::on_documentStatusChanged);

  AddStatusLabel(tr("sel:"));
  l->setMinimumWidth(50);
  l->setAlignment(Qt::AlignCenter);

  AddNamedStatusLabel(lblsellen, "0 - 0x0");
  AddStatusLabel(QString(5, ' '));

#define LoadPixMap(Var, Icon) Var.load(":/images/" Icon ".png");

#define AddStausILable(PixMap, Icon, Label, OPixMap, OIcon)                    \
  LoadPixMap(PixMap, Icon);                                                    \
  LoadPixMap(OPixMap, OIcon);                                                  \
  Label = new DLabel(this);                                                    \
  Label->setPixmap(PixMap);                                                    \
  Label->setScaledContents(true);                                              \
  Label->setFixedSize(20, 20);                                                 \
  Label->setAlignment(Qt::AlignCenter);                                        \
  status->addWidget(Label);                                                    \
  AddStatusLabel(QString(' '));

  AddStausILable(infoSaved, "saved", iSaved, infoUnsaved, "unsaved");
  AddStausILable(infoWriteable, "writable", iReadWrite, infoReadonly,
                 "readonly");

  infoUnLock = ICONRES("unlock");
  infoLock = ICONRES("lock");
  infoCanOver = ICONRES("canover");
  infoCannotOver = ICONRES("unover");

  iLocked = new DIconButton(this);
  iLocked->setIcon(infoUnLock);
  iLocked->setIconSize(QSize(20, 20));
  iOver = new DIconButton(this);
  iOver->setIcon(infoCanOver);
  iOver->setIconSize(QSize(20, 20));

  connect(iLocked, &DIconButton::clicked, [=]() {
    if (!hexeditor->setLockedFile(!hexeditor->isLocked())) {
      auto d = DMessageManager::instance();
      d->sendMessage(this, infoLock, tr("ErrUnLock"));
    }
  });

  connect(iOver, &DIconButton::clicked, [=]() {
    if (!hexeditor->setKeepSize(!hexeditor->isKeepSize())) {
      auto d = DMessageManager::instance();
      d->sendMessage(this, infoCannotOver, tr("ErrUnOver"));
    }
  });

  status->addWidget(iLocked);
  status->addWidget(iOver);

  setDockNestingEnabled(true);

  findresultMenu = new DMenu(this);
  AddMenuAction("del", tr("ClearFindResult"), MainWindow::on_clearfindresult,
                findresultMenu);

  // dockwidgets init
  auto dw = new DDockWidget(this);
  findresult = new DTableWidget(0, 3, this);
  findresult->setEditTriggers(DTableWidget::EditTrigger::NoEditTriggers);
  findresult->setSelectionBehavior(
      QAbstractItemView::SelectionBehavior::SelectRows);
  findresult->setHorizontalHeaderLabels(
      QStringList({tr("file"), tr("addr"), tr("value")}));
  findresult->setColumnWidth(0, 600);
  findresult->setColumnWidth(1, 250);
  findresult->setColumnWidth(2, 350);
  findresult->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(findresult, &QTableWidget::customContextMenuRequested,
          [=]() { findresultMenu->popup(cursor().pos()); });
  connect(findresult, &QTableWidget::itemDoubleClicked, [=] {
    auto item = findresult->item(findresult->currentRow(), 0);
    auto filename = hexfiles.at(_currentfile).filename;
    if (filename != item->text()) {
      int i = 0;
      for (auto item : hexfiles) {
        if (filename == item.filename) {
          break;
        }
        i++;
      }
      setFilePage(i);
    }
    hexeditor->document()->cursor()->moveTo(
        item->data(Qt::UserRole).toLongLong());
  });
  dw->setWindowTitle(tr("FindResult"));
  dw->setObjectName("FindResult");
  dw->setWidget(findresult);
  this->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, dw);

  dw = new DDockWidget(this);
  pluginInfo = new QTextBrowser(this);
  dw->setWindowTitle(tr("Log"));
  dw->setObjectName("Log");
  pluginInfo->setFocusPolicy(Qt::StrongFocus);
  pluginInfo->setOpenExternalLinks(true);
  dw->setWidget(pluginInfo);
  this->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, dw);

  logger = new Logger(this);
  connect(logger, &Logger::log,
          [=](QString msg) { pluginInfo->insertHtml(msg); });
  logger->logMessage(INFOLOG(tr("LoggerInitFinish")));

  auto dw2 = new DDockWidget(this);
  numshowtable = new DTableWidget(8, 1, this);
  numshowtable->setEditTriggers(DTableWidget::EditTrigger::NoEditTriggers);
  numshowtable->setSelectionBehavior(
      QAbstractItemView::SelectionBehavior::SelectRows);
  numshowtable->setHorizontalHeaderLabels(
      QStringList({tr("Type"), tr("Value")}));
  numshowtable->setColumnWidth(0, 350);
  numshowtable->setFocusPolicy(Qt::StrongFocus);
  numshowtable->setVerticalHeaderLabels(
      QStringList({"byte", "char", "ushort", "short", "uint32", "int32",
                   "uint64", "int64"}));
  numsitem = new QTableWidgetItem[NumTableIndexCount];
  for (int i = 0; i < NumTableIndexCount; i++) {
    auto item = numsitem + i;
    item->setText("-");
    item->setTextAlignment(Qt::AlignCenter);
    numshowtable->setItem(i, 0, item);
  }
  dw2->setObjectName("Number");
  dw2->setWindowTitle(tr("Number"));
  dw2->setMinimumWidth(450);
  dw2->setWidget(numshowtable);
  this->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, dw2);
  this->tabifyDockWidget(dw, dw2);

  dw = new DDockWidget(this);
  bookmarks = new DListWidget(this);
  bookmarks->setFocusPolicy(Qt::StrongFocus);
  connect(bookmarks, &DListWidget::itemDoubleClicked, [=]() {
    hexeditor->document()->gotoBookMark(bookmarks->currentRow());
  });
  dw->setWidget(bookmarks);
  dw->setObjectName("BookMark");
  dw->setWindowTitle(tr("BookMark"));
  this->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, dw);
  this->tabifyDockWidget(dw2, dw);
  dw2->raise();

  connect(DGuiApplicationHelper::instance(),
          &DGuiApplicationHelper::themeTypeChanged, this,
          &MainWindow::setTheme);

  gotobar = new GotoBar(w);
  vlayout->insertWidget(1, gotobar);
  gotobar->setVisible(false);
  connect(gotobar, &GotoBar::jumpToLine, this, &MainWindow::on_gotobar);

#define ConnectShortCut(ShortCut, Slot)                                        \
  s = new QShortcut(ShortCut, this);                                           \
  connect(s, &QShortcut::activated, this, &Slot);

  QShortcut *s;
  ConnectShortCut(QKeySequence::New, MainWindow::on_newfile);
  ConnectShortCut(QKeySequence::Open, MainWindow::on_openfile);
  ConnectShortCut(QKeySequence::Save, MainWindow::on_savefile);
  ConnectShortCut(QKeySequence::SaveAs, MainWindow::on_saveasfile);
  ConnectShortCut(QKeySequence::Undo, MainWindow::on_undofile);
  ConnectShortCut(QKeySequence::Redo, MainWindow::on_redofile);
  ConnectShortCut(QKeySequence::Cut, MainWindow::on_cutfile);
  ConnectShortCut(QKeySequence::Copy, MainWindow::on_copyfile);
  ConnectShortCut(QKeySequence::Paste, MainWindow::on_pastefile);
  ConnectShortCut(QKeySequence::Find, MainWindow::on_findfile);
  ConnectShortCut(keygoto, MainWindow::on_gotoline);
  ConnectShortCut(keyplugin, MainWindow::on_setting_plugin);
  ConnectShortCut(keyGeneral, MainWindow::on_setting_general);
  ConnectShortCut(keyOpenDriver, MainWindow::on_opendriver);
  ConnectShortCut(keymetadata, MainWindow::on_metadata);
  ConnectShortCut(keyfill, MainWindow::on_fill);
  ConnectShortCut(keyfillnop, MainWindow::on_fillnop);
  ConnectShortCut(keyfillzero, MainWindow::on_fillzero);

  logger->logMessage(INFOLOG(tr("SettingLoading")));

  // setting
  _font = this->font();
  _hexeditorfont = QHexView::getHexeditorFont();
  m_settings = new Settings(this);
  connect(m_settings, &Settings::sigAdjustFont, [=](QString name) {
    _font.setFamily(name);
    numshowtable->setFont(_font);
    findresult->setFont(_font);
    pluginInfo->setFont(_font);
  });
  connect(m_settings, &Settings::sigShowColNumber,
          [=](bool b) { _showheader = b; });
  connect(m_settings, &Settings::sigAdjustEditorFontSize, [=](int fontsize) {
    _hexeditorfont.setPointSize(fontsize);
    hexeditor->setFont(_hexeditorfont);
  });
  connect(m_settings, &Settings::sigAdjustInfoFontSize, [=](int fontsize) {
    _font.setPointSize(fontsize);
    numshowtable->setFont(_font);
    findresult->setFont(_font);
  });
  connect(m_settings, &Settings::sigShowEncodingText,
          [=](bool b) { _showascii = b; });
  connect(m_settings, &Settings::sigShowAddressNumber,
          [=](bool b) { _showaddr = b; });
  connect(m_settings, &Settings::sigChangeWindowState,
          [=](QString mode) { _windowmode = mode; });
  connect(m_settings, &Settings::sigChangePluginEnabled,
          [=](bool b) { _enableplugin = b; });

  m_settings->applySetting();
  hexeditor->setAddressVisible(_showaddr);
  hexeditor->setHeaderVisible(_showheader);
  hexeditor->setAsciiVisible(_showascii);

  if (_windowmode == "window_normal") {
    setWindowState(Qt::WindowState::WindowActive);
  } else if (_windowmode == "window_maximum") {
    setWindowState(Qt::WindowState::WindowMaximized);
  } else if (_windowmode == "window_minimum") {
    setWindowState(Qt::WindowState::WindowMinimized);
  } else {
    setWindowState(Qt::WindowState::WindowFullScreen);
  }

  if (_enableplugin) {
    logger->logMessage(INFOLOG(tr("PluginLoading")));
    // init plugin system
    plgsys = new PluginSystem(this);
    // connect(plgsys, &PluginSystem::PluginCall, this,
    // &MainWindow::PluginCall);

    connect(plgsys, &PluginSystem::ConnectShadow, this,
            &MainWindow::connectShadow);

    connect(plgsys, &PluginSystem::PluginMenuNeedAdd, this,
            &MainWindow::PluginMenuNeedAdd);
    connect(plgsys, &PluginSystem::PluginDockWidgetAdd, this,
            &MainWindow::PluginDockWidgetAdd);
    plgsys->LoadPlugin();
  } else {
    logger->logMessage(ERRLOG(tr("UnLoadPluginSetting")));
  }

  m_settings->saveWindowState(this, true);
  m_settings->loadWindowState(this);
}

MainWindow::~MainWindow() {
  delete[] numsitem;
  if (findresitem)
    delete[] findresitem;
  for (auto item : hexfiles) {
    item.doc->deleteLater();
    item.render->deleteLater();
  }
}

void MainWindow::PluginMenuNeedAdd(QMenu *menu) {
  if (menu != nullptr) {
    logger->logMessage(WARNLOG(tr("MenuName :") + menu->title()));
    plgmenu->addMenu(menu);
  }
}

void MainWindow::PluginDockWidgetAdd(QDockWidget *dockw,
                                     Qt::DockWidgetArea align) {
  if (dockw != nullptr) {
    logger->logMessage(WARNLOG(tr("DockWidgetName :") + dockw->windowTitle()));
    dockw->setParent(this);
    addDockWidget(align, dockw);
  }
}

void MainWindow::connectShadow(HexViewShadow *shadow) {
  if (shadow == nullptr)
    return;

#define ConnectShadowSlot(Signal, Slot) connect(shadow, &Signal, this, &Slot)
#define ConnectShadowLamba(Signal, Function) connect(shadow, &Signal, Function)

  // connect neccessary signal-slot
  ConnectShadowSlot(HexViewShadow::shadowControl, MainWindow::shadowControl);
  ConnectShadowSlot(HexViewShadow::shadowIsValid, MainWindow::shadowIsValid);
  ConnectShadowSlot(HexViewShadow::shadowDestory, MainWindow::shadowDestory);
  ConnectShadowSlot(HexViewShadow::shadowRelease, MainWindow::shadowRelease);

#define PCHECK(T, F)                                                           \
  if (hexfiles.count() > 0)                                                    \
    T;                                                                         \
  F;

#define PCHECKRETURN(T, F)                                                     \
  if (hexfiles.count() > 0)                                                    \
    return T;                                                                  \
  return F;

  // connect property-get signal-slot
  ConnectShadowLamba(HexViewShadow::isLocked,
                     [=] { PCHECKRETURN(hexeditor->isLocked(), true); });
  ConnectShadowLamba(HexViewShadow::isEmpty, [=] {
    PCHECKRETURN(hexeditor->document()->isEmpty(), true);
  });
  ConnectShadowLamba(HexViewShadow::isKeepSize,
                     [=] { PCHECKRETURN(hexeditor->isKeepSize(), true); });
  ConnectShadowLamba(HexViewShadow::isModified,
                     [=] { PCHECKRETURN(hexeditor->isModified(), false); });
  ConnectShadowLamba(HexViewShadow::isReadOnly,
                     [=] { PCHECKRETURN(hexeditor->isReadOnly(), true); });
  ConnectShadowLamba(HexViewShadow::documentLines, [=] {
    PCHECKRETURN(hexeditor->documentLines(), quint64(0));
  });
  ConnectShadowLamba(HexViewShadow::documentBytes, [=] {
    PCHECKRETURN(hexeditor->documentBytes(), quint64(0));
  });
  ConnectShadowLamba(HexViewShadow::currentPos, [=] {
    HexPosition pos;
    PCHECK(
        {
          auto qpos = hexeditor->document()->cursor()->position();
          pos.line = qpos.line;
          pos.column = qpos.column;
          pos.lineWidth = qpos.lineWidth;
          pos.nibbleindex = qpos.nibbleindex;
        },
        return pos);
  });
  ConnectShadowLamba(HexViewShadow::selectionPos, [=] {
    HexPosition pos;
    PCHECK(
        {
          auto cur = hexeditor->document()->cursor();
          pos.line = cur->selectionLine();
          pos.column = cur->selectionColumn();
          pos.nibbleindex = cur->selectionNibble();
          pos.lineWidth = cur->position().lineWidth;
        },
        return pos);
  });
  ConnectShadowLamba(HexViewShadow::currentRow, [=] {
    PCHECKRETURN(hexeditor->currentRow(), quint64(0));
  });
  ConnectShadowLamba(HexViewShadow::currentColumn, [=] {
    PCHECKRETURN(hexeditor->currentColumn(), quint64(0));
  });
  ConnectShadowLamba(HexViewShadow::currentOffset, [=] {
    PCHECKRETURN(hexeditor->currentOffset(), quint64(0));
  });
  ConnectShadowLamba(HexViewShadow::selectlength, [=] {
    PCHECKRETURN(hexeditor->selectlength(), quint64(0));
  });
  ConnectShadowLamba(HexViewShadow::asciiVisible,
                     [=] { PCHECKRETURN(hexeditor->asciiVisible(), true); });
  ConnectShadowLamba(HexViewShadow::headerVisible,
                     [=] { PCHECKRETURN(hexeditor->headerVisible(), true); });
  ConnectShadowLamba(HexViewShadow::addressVisible,
                     [=] { PCHECKRETURN(hexeditor->addressVisible(), true); });
  ConnectShadowLamba(HexViewShadow::addressBase, [=] {
    PCHECKRETURN(hexeditor->addressBase(), quint64(0));
  });
  ConnectShadowLamba(HexViewShadow::atEnd, [=] {
    PCHECKRETURN(hexeditor->document()->atEnd(), false);
  });
  ConnectShadowLamba(HexViewShadow::canUndo, [=] {
    PCHECKRETURN(hexeditor->document()->canUndo(), false);
  });
  ConnectShadowLamba(HexViewShadow::canRedo, [=] {
    PCHECKRETURN(hexeditor->document()->canRedo(), false);
  });
  ConnectShadowLamba(HexViewShadow::areaIndent, [=] {
    PCHECKRETURN(hexeditor->document()->areaIndent(), 0);
  });
  ConnectShadowLamba(HexViewShadow::hexLineWidth, [=] {
    PCHECKRETURN(hexeditor->document()->hexLineWidth(), 0);
  });
  ConnectShadowLamba(HexViewShadow::editableArea, [=](int area) {
    PCHECKRETURN(hexeditor->renderer()->editableArea(area), false);
  });
  ConnectShadowLamba(HexViewShadow::documentLastLine, [=] {
    PCHECKRETURN(hexeditor->renderer()->documentLastLine(), quint64(0));
  });
  ConnectShadowLamba(HexViewShadow::documentLastColumn, [=] {
    PCHECKRETURN(hexeditor->renderer()->documentLastColumn(), 0);
  });
  ConnectShadowLamba(HexViewShadow::documentWidth, [=] {
    PCHECKRETURN(hexeditor->renderer()->documentWidth(), 0);
  });
  ConnectShadowLamba(HexViewShadow::lineHeight, [=] {
    PCHECKRETURN(hexeditor->renderer()->lineHeight(), 0);
  });
  ConnectShadowLamba(HexViewShadow::getLineRect, [=](quint64 line,
                                                     quint64 firstline) {
    PCHECKRETURN(hexeditor->renderer()->getLineRect(line, firstline), QRect());
  });
  ConnectShadowLamba(HexViewShadow::headerLineCount, [=] {
    PCHECKRETURN(hexeditor->renderer()->headerLineCount(), 0);
  });
  ConnectShadowLamba(HexViewShadow::borderSize, [=] {
    PCHECKRETURN(hexeditor->renderer()->borderSize(), 0);
  });
  ConnectShadowLamba(HexViewShadow::copy, [=](bool hex) {
    PCHECK(hexeditor->document()->copy(hex), );
  });
  ConnectShadowLamba(HexViewShadow::read, [=](qint64 offset, int len) {
    PCHECKRETURN(hexeditor->document()->read(offset, len), QByteArray());
  });
  ConnectShadowLamba(
      HexViewShadow::FindAllBytes, [=](QByteArray b, QList<quint64> &results) {
        PCHECK(hexeditor->document()->FindAllBytes(b, results), );
      });
  ConnectShadowLamba(HexViewShadow::searchForward, [=](const QByteArray &ba) {
    PCHECKRETURN(hexeditor->document()->searchForward(ba), qint64(-1));
  });
  ConnectShadowLamba(HexViewShadow::searchBackward, [=](const QByteArray &ba) {
    PCHECKRETURN(hexeditor->document()->searchBackward(ba), qint64(-1));
  });
}

void MainWindow::shadowDestory(IWingPlugin *plugin) {
  plgsys->shadowDestory(plugin);
}
bool MainWindow::shadowIsValid(IWingPlugin *plugin) {
  return plgsys->shadowIsValid(plugin);
}
bool MainWindow::shadowControl(IWingPlugin *plugin, HexViewShadow *shadow) {
  return plgsys->shadowControl(plugin, shadow);
}
bool MainWindow::shadowRelease(IWingPlugin *plugin, HexViewShadow *shadow) {
  return plgsys->shadowRelease(plugin, shadow);
}

void MainWindow::setTheme(DGuiApplicationHelper::ColorType theme) {
  auto p = palette();

  if (theme == DGuiApplicationHelper::LightType) {
  } else {
  }
}

void MainWindow::on_hexeditor_customContextMenuRequested(const QPoint &pos) {
  Q_UNUSED(pos)
  hexeditorMenu->popup(QCursor::pos());
}

void MainWindow::on_tabs_currentChanged(int index) { setFilePage(index); }

void MainWindow::on_tabMoved(int from, int to) { hexfiles.move(from, to); }

void MainWindow::setFilePage(int index) {
  if (index < 0 && hexfiles.count() == 0) {
    _currentfile = -1;
    return;
  }
  if (index >= 0 && index < hexfiles.count()) {
    if (_currentfile >= 0 && _currentfile < hexfiles.count()) {
      auto s = hexeditor->verticalScrollBar()->value();
      hexfiles[_currentfile].vBarValue = s;
    }
    _currentfile = index;
    auto d = hexfiles.at(index);
    if (d.doc == hexeditor->document())
      return;
    hexeditor->switchDocument(d.doc, d.render, d.vBarValue);
    tabs->setCurrentIndex(index);
  }
}

void MainWindow::on_newfile() { newFile(); }

void MainWindow::newFile() {
  QList<QVariant> params;
  QString title = tr("Untitled") + QString("-%1").arg(defaultindex);
  if (_enableplugin) {
    params << HookIndex::NewFileBegin << title;
    plgsys->raiseDispatch(HookIndex::NewFileBegin, params);
  }

  hexeditor->setVisible(true);
  auto p = QHexDocument::fromFile<QMemoryBuffer>(nullptr);
  HexFile hf;
  hf.doc = p;
  hexeditor->setDocument(p);
  hexeditor->setAddressVisible(_showaddr);
  hexeditor->setAsciiVisible(_showascii);
  hexeditor->setHeaderVisible(_showheader);
  hf.render = hexeditor->renderer();
  hf.vBarValue = -1;
  hf.filename = ":" + title;
  hexfiles.push_back(hf);
  tabs->addTab(QIcon::fromTheme("text-plain"), title);
  defaultindex++;
  auto curindex = hexfiles.count() - 1;
  tabs->setCurrentIndex(curindex);
  tabs->setTabToolTip(curindex, title);
  setEditModeEnabled(true);

  if (_enableplugin) {
    params[0].setValue(HookIndex::NewFileEnd);
    plgsys->raiseDispatch(HookIndex::NewFileEnd, params);
  }
}

ErrFile MainWindow::openFile(QString filename, bool readonly) {
  QList<QVariant> params;
  if (_enableplugin) {
    params << HookIndex::OpenFileBegin << filename << readonly;
    plgsys->raiseDispatch(HookIndex::OpenFileBegin, params);
  }
  QFileInfo info(filename);
  if (info.exists()) {

    if (!info.permission(QFile::ReadUser)) {
      if (_enableplugin) {
        params[0].setValue(HookIndex::OpenFileEnd);
        params << ErrFile::Permission;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      return ErrFile::Permission;
    }

    if (!readonly && !info.permission(QFile::WriteUser)) {
      if (_enableplugin) {
        params[0].setValue(HookIndex::OpenFileEnd);
        params << ErrFile::Permission;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      return ErrFile::Permission;
    }

    for (auto item : hexfiles) {
      if (item.filename == filename) {
        if (_enableplugin) {
          params[0].setValue(HookIndex::OpenFileEnd);
          params << ErrFile::AlreadyOpened;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::AlreadyOpened;
      }
    }

    hexeditor->setVisible(true);

    HexFile hf;
    auto *p =
        info.size() > FILEMAXBUFFER
            ? QHexDocument::fromLargeFile(filename, readonly, this)
            : QHexDocument::fromFile<QMemoryBuffer>(filename, readonly, this);

    if (p == nullptr) {
      if (_enableplugin) {
        params[0].setValue(HookIndex::OpenFileEnd);
        params << ErrFile::Error;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      return ErrFile::Error;
    }

    hf.doc = p;
    hexeditor->setLockedFile(readonly);
    hexeditor->setDocument(p);
    hexeditor->setKeepSize(true);

    hf.render = hexeditor->renderer();
    hf.vBarValue = -1;
    hf.filename = filename;
    hexfiles.push_back(hf);

    QMimeDatabase db;
    auto t = db.mimeTypeForFile(filename);
    auto ico = t.iconName();
    tabs->addTab(QIcon::fromTheme(ico, QIcon(ico)), info.fileName());
    auto index = hexfiles.count() - 1;
    tabs->setCurrentIndex(index);
    tabs->setTabToolTip(index, filename);
    setEditModeEnabled(true);

    if (_enableplugin) {
      params[0].setValue(HookIndex::OpenFileEnd);
      params << ErrFile::Success;
      plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
    }
    return ErrFile::Success;
  }

  if (_enableplugin) {
    params[0].setValue(HookIndex::OpenFileEnd);
    params << ErrFile::NotExist;
    plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
  }
  return ErrFile::NotExist;
}

ErrFile MainWindow::openDriver(QString driver) {
  QList<QVariant> params;
  if (_enableplugin) {
    params << HookIndex::OpenDriverBegin << driver;
    plgsys->raiseDispatch(HookIndex::OpenDriverBegin, params);
  }

  if (Utilities::isRoot()) {
    openFile(driver);
    setEditModeEnabled(true);
    toolbartools[ToolBoxIndex::SaveAs]->setEnabled(false);
    if (_enableplugin) {
      params[0].setValue(HookIndex::OpenDriverEnd);
      params << ErrFile::Success;
      plgsys->raiseDispatch(HookIndex::OpenDriverEnd, params);
    }
    return ErrFile::Success;
  } else {
    QMessageBox::critical(this, tr("Error"), tr("NoRoot"));
    if (_enableplugin) {
      params[0].setValue(HookIndex::OpenDriverEnd);
      params << ErrFile::Permission;
      plgsys->raiseDispatch(HookIndex::OpenDriverEnd, params);
    }
    return ErrFile::Permission;
  }
}

bool MainWindow::isModified(int index) {
  if (index < 0 || index >= hexfiles.count())
    return false;
  auto p = hexfiles.at(index);
  return p.doc->isModfied();
}

ErrFile MainWindow::closeCurrentFile(bool force) {
  return closeFile(_currentfile, force);
}

ErrFile MainWindow::closeFile(int index, bool force) {
  QList<QVariant> params;
  if (_enableplugin) {
    params << HookIndex::CloseFileBegin << index << force;
    plgsys->raiseDispatch(HookIndex::CloseFileBegin, params);
  }

  if (index >= 0 && index < hexfiles.count()) {
    auto p = hexfiles.at(index);
    if (!force) {
      if (isModified(index)) {
        if (_enableplugin) {
          params[0].setValue(HookIndex::CloseFileEnd);
          params << ErrFile::UnSaved;
          plgsys->raiseDispatch(HookIndex::CloseFileEnd, params);
        }
        return ErrFile::UnSaved;
      }
    }
    tabs->removeTab(index);
    hexfiles.removeAt(index);
    if (hexfiles.isEmpty()) {
      hexeditor->setVisible(false);
      // empty the buffer
      hexeditor->setDocument(
          QHexDocument::fromMemory<QMemoryBuffer>(QByteArray(), hexeditor));
    } else {
      auto hf = hexfiles.at(tabs->currentIndex());
      hexeditor->switchDocument(hf.doc, hf.render, hf.vBarValue);
    }
    p.doc->deleteLater();
    p.render->deleteLater();
  }
  if (hexfiles.count() == 0)
    setEditModeEnabled(false);

  if (_enableplugin) {
    params[0].setValue(HookIndex::CloseFileEnd);
    params << ErrFile::Success;
    plgsys->raiseDispatch(HookIndex::CloseFileEnd, params);
  }
  return ErrFile::Success;
}

void MainWindow::gotoFileLine(int index, quint64 offset) {
  auto c = hexfiles.count();
  if (index >= 0 && index < c) {
    auto file = hexfiles[index];
    auto doc = file.doc;
    doc->cursor()->moveTo(qint64(offset));
  }
}

void MainWindow::gotoCurrentLine(quint64 offset) {
  if (hexfiles.count() > 0) {
    hexeditor->document()->cursor()->moveTo(qint64(offset));
  }
}

void MainWindow::on_openfile() {
  auto filename = QFileDialog::getOpenFileName(this, tr("ChooseFile"));
  if (!filename.isEmpty()) {
    auto res = openFile(filename);
    if (res == ErrFile::NotExist) {
      QMessageBox::critical(this, tr("Error"), tr("FileNotExist"));
      return;
    }
    if (res == ErrFile::AlreadyOpened) {
      int i = 0;
      for (auto item : hexfiles) {
        if (filename == item.filename) {
          break;
        }
        i++;
      }
      setFilePage(i);
    }
    if (res == ErrFile::Permission &&
        openFile(filename, true) == ErrFile::Permission) {
      QMessageBox::critical(this, tr("Error"), tr("FilePermission"));
      return;
    }
  }
}

void MainWindow::on_tabCloseRequested(int index) {
  auto res = closeFile(index);
  if (res != ErrFile::Success) {
    auto f = hexfiles.at(index).filename;

    auto r = QMessageBox::question(this, tr("Close"),
                                   tr("ConfirmSave") + f.remove(':'));
    if (r == QMessageBox::Yes) {
      closeFile(index, true);
    }
  }
}

void MainWindow::on_tabAddRequested() { newFile(); }

void MainWindow::on_undofile() { hexeditor->document()->undo(); }

void MainWindow::on_redofile() { hexeditor->document()->redo(); }

void MainWindow::on_cutfile() { hexeditor->document()->cut(); }
void MainWindow::on_copyfile() { hexeditor->document()->copy(); }
void MainWindow::on_pastefile() { hexeditor->document()->paste(); }

void MainWindow::on_opendriver() {
  DriverSelectorDialog ds;
  if (ds.exec()) {
    openDriver(ds.GetResult().device());
  }
}

void MainWindow::on_exportfile() {
  auto filename = QFileDialog::getSaveFileName(this, tr("ChooseExportFile"));
  if (filename.isEmpty())
    return;
  exportFile(filename, _currentfile);
}

void MainWindow::on_exit() { close(); }

void MainWindow::showEvent(QShowEvent *event) { Q_UNUSED(event); }

void MainWindow::closeEvent(QCloseEvent *event) {
  while (hexfiles.count() > 0) {
    auto res = closeFile(0);
    if (res != ErrFile::Success) {
      auto f = hexfiles.at(0).filename;
      setFilePage(0);
      auto r = QMessageBox::question(this, tr("Close"),
                                     tr("ConfirmSave") + f.remove(':'));
      if (r == QMessageBox::Yes) {
        closeFile(0, true);
        tabs->removeTab(0);
      } else {
        on_savefile();
        event->ignore();
        return;
      }
    }
  }
  m_settings->saveWindowState(this);
  event->accept();
}

void MainWindow::on_savefile() {
  CheckEnabled;
  if (saveCurrentFile() == ErrFile::IsNewFile)
    on_saveasfile();
}

void MainWindow::on_delete() {
  CheckEnabled;
  hexeditor->document()->removeSelection();
}

void MainWindow::on_saveasfile() {
  CheckEnabled;
  auto filename = QFileDialog::getSaveFileName(this, tr("ChooseSaveFile"));
  if (filename.isEmpty())
    return;
  saveasFile(filename, _currentfile);
}

void MainWindow::on_findfile() {
  CheckEnabled;
  FindDialog *fd = new FindDialog();
  if (fd->exec()) {
    auto th = QThread ::create([=]() {
      auto res = fd->getResult();
      auto d = hexeditor->document();
      QList<quint64> results;
      d->FindAllBytes(res, results);
      if (findresitem) {
        delete[] findresitem;
        findresult->setRowCount(0);
      }
      auto len = results.length();
      findresitem = new QTableWidgetItem[ulong(len)][3];
      for (auto i = 0; i < len; i++) {
        auto frow = findresitem[i];
        findresult->insertRow(i);
        frow[0].setText(hexfiles.at(_currentfile).filename);
        frow[0].setData(Qt::UserRole, results.at(i));
        frow[1].setText(
            QString::number(results.at(i) + hexeditor->addressBase(), 16));
        frow[2].setText(res.toHex(' '));
        findresult->setItem(i, 0, frow);
        findresult->setItem(i, 1, frow + 1);
        findresult->setItem(i, 2, frow + 2);
      }
    });
    th->start();
  }
}
void MainWindow::on_gotoline() {
  CheckEnabled;
  gotobar->activeInput(int(hexeditor->currentRow()),
                       int(hexeditor->currentColumn()),
                       hexeditor->currentOffset(), hexeditor->documentBytes(),
                       int(hexeditor->documentLines()));
}

void MainWindow::on_gotobar(int pos, bool isline) {
  CheckEnabled;
  if (hexfiles.count() > 0) {
    auto cur = hexeditor->document()->cursor();
    isline ? cur->moveTo(quint64(pos), 0) : cur->moveTo(pos);
  }
}

void MainWindow::on_locChanged() {
  CheckEnabled;
  lblloc->setText(QString("(%1,%2)")
                      .arg(hexeditor->currentRow())
                      .arg(hexeditor->currentColumn()));
  auto sellen = hexeditor->selectlength();
  lblsellen->setText(QString("%1 - 0x%2").arg(sellen).arg(sellen, 0, 16));

  // number analyse
  auto off = qint64(hexeditor->currentOffset());
  auto d = hexeditor->document();
  auto tmp = d->read(off, sizeof(quint64));
  auto len = tmp.length();
  quint64 n = 0;
  for (int i = 0; i < len; i++) {
    n |= (quint64(tmp.at(i)) << (8 * i));
  }

  if (len == sizeof(quint64)) {
    auto s = n;
    numsitem[NumTableIndex::Uint64].setText(QString("0x%1 | %2")
                                                .arg(QString::number(s, 16))
                                                .arg(QString::number(s)));
    auto s1 = qint64(n);
    numsitem[NumTableIndex::Int64].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Uint64].setText("-");
    numsitem[NumTableIndex::Int64].setText("-");
  }

  if (len > int(sizeof(quint32))) {
    auto s = ulong(n);
    numsitem[NumTableIndex::Uint32].setText(QString("0x%1 | %2")
                                                .arg(QString::number(s, 16))
                                                .arg(QString::number(s)));
    auto s1 = long(n);
    numsitem[NumTableIndex::Int32].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Uint32].setText("-");
    numsitem[NumTableIndex::Int32].setText("-");
  }

  if (len > int(sizeof(ushort))) {
    auto s = ushort(n);
    numsitem[NumTableIndex::Ushort].setText(QString("0x%1 | %2")
                                                .arg(QString::number(s, 16))
                                                .arg(QString::number(s)));
    auto s1 = short(n);
    numsitem[NumTableIndex::Short].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Ushort].setText("-");
    numsitem[NumTableIndex::Short].setText("-");
  }
  if (len > int(sizeof(uchar))) {
    auto s1 = tmp.at(0);
    auto s = uchar(s1);
    numsitem[NumTableIndex::Byte].setText(QString("0x%1 | %2")
                                              .arg(QString::number(s, 16))
                                              .arg(QString::number(s)));
    numsitem[NumTableIndex::Char].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Byte].setText("-");
    numsitem[NumTableIndex::Char].setText("-");
  }
}

void MainWindow::on_setting_general() {
  DSettingsDialog *dialog = new DSettingsDialog(this);
  dialog->widgetFactory()->registerWidget("fontcombobox",
                                          Settings::createFontComBoBoxHandle);

  m_settings->setSettingDialog(dialog);

  dialog->updateSettings(m_settings->settings);

  dialog->exec();

  delete dialog;
  m_settings->settings->sync();
}

void MainWindow::on_documentChanged() {
  CheckEnabled;
  iSaved->setPixmap(isModified(_currentfile) ? infoUnsaved : infoSaved);
}

void MainWindow::on_documentSwitched() {
  CheckEnabled;
  QList<BookMarkStruct> bookmaps;
  bookmarks->clear();
  hexeditor->document()->getBookMarks(bookmaps);
  for (auto item : bookmaps) {
    QListWidgetItem *litem = new QListWidgetItem;
    litem->setIcon(ICONRES("bookmark"));
    litem->setText(item.comment);
    litem->setToolTip(QString(tr("Addr : 0x%1")).arg(item.pos, 0, 16));
    bookmarks->addItem(litem);
  }
}

void MainWindow::on_documentStatusChanged() {
  CheckEnabled;
  iSaved->setPixmap(hexeditor->isModified() ? infoUnsaved : infoSaved);
  iReadWrite->setPixmap(hexeditor->isReadOnly() ? infoReadonly : infoWriteable);
  iLocked->setIcon(hexeditor->isLocked() ? infoLock : infoUnLock);
  iOver->setIcon(hexeditor->isKeepSize() ? infoCannotOver : infoCanOver);
}

ErrFile MainWindow::saveFile(int index) {
  if (index >= 0 && index < hexfiles.count()) {
    auto f = hexfiles.at(index);
    if (f.filename.at(0) == ':')
      return ErrFile::IsNewFile;
    QFile file(f.filename);
    file.open(QFile::WriteOnly);
    if (f.doc->saveTo(&file, true)) {
      return ErrFile::Success;
    }
    return ErrFile::Permission;
  }
  return ErrFile::Error;
}

ErrFile MainWindow::exportFile(QString filename, int index) {
  if (index >= 0 && index < hexfiles.count()) {
    auto f = hexfiles.at(index);
    QFile file(filename);
    file.open(QFile::WriteOnly);
    if (f.doc->saveTo(&file, false)) {
      file.close();
      return ErrFile::Success;
    }
    file.close();
    return ErrFile::Permission;
  }
  return ErrFile::Error;
}

ErrFile MainWindow::saveasFile(QString filename, int index) {
  if (index >= 0 && index < hexfiles.count()) {
    auto f = hexfiles.at(index);
    QFile file(filename);
    file.open(QFile::WriteOnly);
    if (f.doc->saveTo(&file, true)) {
      hexfiles[index].filename = filename;
      file.close();
      return ErrFile::Success;
    }
    file.close();
    return ErrFile::Permission;
  }
  return ErrFile::Error;
}

ErrFile MainWindow::saveCurrentFile() { return saveFile(_currentfile); }

void MainWindow::on_metadata() {
  CheckEnabled;
  if (hexeditor->documentBytes() > 0) {
    MetaDialog m;

    if (m.exec()) {
      auto begin =
          qint64(hexeditor->document()->cursor()->selectionStart().offset());
      auto end =
          qint64(hexeditor->document()->cursor()->selectionEnd().offset());
      hexeditor->document()->metadata()->metadata(
          begin, end, m.foreGroundColor(), m.backGroundColor(), m.comment());
    }
  } else {
    auto d = DMessageManager::instance();
    d->sendMessage(this, ICONRES("metadata"), tr("NoSelection"));
  }
}

void MainWindow::on_metadatadel() {
  CheckEnabled;
  auto doc = hexeditor->document();
  auto meta = doc->metadata();
  auto pos = doc->cursor()->position().offset();
  meta->removeMetadata(pos, meta->gets(pos));
}

void MainWindow::on_metadatacls() {
  CheckEnabled;
  hexeditor->document()->metadata()->clear();
}

void MainWindow::on_setting_plugin() {
  PluginWindow pw(this);
  pw.setPluginSystem(plgsys);
  pw.exec();
}

void MainWindow::on_bookmark() {
  CheckEnabled;
  auto doc = hexeditor->document();
  int index = -1;
  if (doc->existBookMark(index)) {
    auto b = doc->bookMark(index);
    auto comment = DInputDialog::getText(
        this, tr("BookMark"), tr("InputComment"), QLineEdit::Normal, b.comment);
    if (!comment.isEmpty()) {
      auto item = bookmarks->item(index);
      item->setText(comment);
    }
  } else {
    auto comment =
        DInputDialog ::getText(this, tr("BookMark"), tr("InputComment"));
    if (!comment.isEmpty()) {
      hexeditor->document()->addBookMark(comment);
      QListWidgetItem *item = new QListWidgetItem;
      item->setIcon(ICONRES("bookmark"));
      item->setText(comment);
      item->setToolTip(QString(tr("Addr : 0x%1"))
                           .arg(doc->cursor()->position().offset(), 0, 16));
      bookmarks->addItem(item);
    }
  }
}

void MainWindow::on_bookmarkdel() {
  CheckEnabled;
  auto doc = hexeditor->document();
  int index = -1;
  if (doc->existBookMark(index)) {
    doc->removeBookMark(index);
    auto item = bookmarks->item(index);
    bookmarks->removeItemWidget(item);
    delete item; // make the removed item disapeared from the list widgets
  }
}

void MainWindow::on_bookmarkcls() {
  CheckEnabled;
  hexeditor->document()->clearBookMark();
  bookmarks->clear();
}

void MainWindow::setEditModeEnabled(bool b) {
  for (auto item : toolbartools.values()) {
    item->setEnabled(b);
  }
  hexeditorMenu->setEnabled(b);
  editmenu->setEnabled(b);
  status->setEnabled(b);
}

void MainWindow::on_restoreLayout() { m_settings->loadWindowState(this, true); }

void MainWindow::on_fill() {
  auto in = DInputDialog::getText(this, tr("Fill"), tr("PleaseInputFill"));
  if (in.length() != 0) {
    bool b = false;
    auto ch = char(in.toULongLong(&b, 0));
    if (b) {
      auto doc = hexeditor->document();
      if (doc->isEmpty() || hexeditor->selectlength() == 0)
        return;
      auto pos = doc->cursor()->selectionStart().offset();
      doc->replace(pos, QByteArray(int(hexeditor->selectlength()), char(ch)));
    } else {
      DMessageManager::instance()->sendMessage(this, ICONRES("fill"),
                                               tr("FillInputError"));
    }
  }
}

void MainWindow::on_fillnop() {
  auto doc = hexeditor->document();
  if (doc->isEmpty() || hexeditor->selectlength() == 0)
    return;
  auto pos = doc->cursor()->selectionStart().offset();
  doc->replace(pos, QByteArray(int(hexeditor->selectlength()), char(0x90)));
}

void MainWindow::on_fillzero() {
  auto doc = hexeditor->document();
  if (doc->isEmpty() || hexeditor->selectlength() == 0)
    return;
  auto pos = doc->cursor()->selectionStart().offset();
  doc->replace(pos, QByteArray(int(hexeditor->selectlength()), char(0)));
}

void MainWindow::on_clearfindresult() {
  delete[] findresitem;
  findresitem = nullptr;
  findresult->setRowCount(0);
}

void MainWindow::on_sponsor() {
  SponsorDialog d;
  d.exec();
}

void MainWindow::on_about() {
  AboutSoftwareDialog d;
  d.exec();
}
