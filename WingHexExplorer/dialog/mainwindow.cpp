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

#define INFOLOG(msg) "<font color=\"green\">" + msg + "</font>"
#define ERRLOG(msg) "<font color=\"red\">" + msg + "</font>"
#define WARNLOG(msg) "<font color=\"yellow\">" + msg + "</font>"

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
      QKeySequence(Qt::KeyboardModifier::ControlModifier + Qt::Key_G);
  auto keyGeneral =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_G);
  auto keyplugin = QKeySequence(Qt::KeyboardModifier::ControlModifier |
                                Qt::KeyboardModifier::AltModifier | Qt::Key_P);
  auto keymetadata =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::AltModifier | Qt::Key_M);

  auto keybookmark =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_B);

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
  AddToolSubMenuShortcutAction("metadata", tr("MetaData"),
                               MainWindow::on_metadata, keymetadata);
  AddToolSubMenuShortcutAction("bookmark", tr("BookMark"),
                               MainWindow::on_bookmark, keybookmark);
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Setting"));
  tm->setIcon(ICONRES("setting"));
  AddToolSubMenuShortcutAction("general", tr("General"),
                               MainWindow::on_setting_general, keyGeneral);
  AddToolSubMenuShortcutAction("settingplugin", tr("Plugin"),
                               MainWindow::on_setting_plugin, keyplugin);
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

  contextMenu = new DMenu(this);

#define AddContextMenuAction(Icon, Title, Slot, ShortCut)                      \
  AddMenuShortcutAction(Icon, Title, Slot, contextMenu, ShortCut)

  AddContextMenuAction("undo", tr("Undo"), MainWindow::on_undofile,
                       QKeySequence::Undo);
  AddContextMenuAction("redo", tr("Redo"), MainWindow::on_redofile,
                       QKeySequence::Redo);
  contextMenu->addSeparator();
  AddContextMenuAction("cut", tr("Cut"), MainWindow::on_cutfile,
                       QKeySequence::Cut);
  AddContextMenuAction("copy", tr("Copy"), MainWindow::on_copyfile,
                       QKeySequence::Copy);
  AddContextMenuAction("paste", tr("Paste"), MainWindow::on_pastefile,
                       QKeySequence::Paste);
  AddContextMenuAction("del", tr("Delete"), MainWindow::on_delete,
                       QKeySequence::Delete);
  contextMenu->addSeparator();
  AddContextMenuAction("find", tr("Find"), MainWindow::on_findfile,
                       QKeySequence::Find);
  AddContextMenuAction("jmp", tr("Goto"), MainWindow::on_gotoline, keygoto);
  contextMenu->addSeparator();
  AddContextMenuAction("metadata", tr("MetaData"), MainWindow::on_metadata,
                       keymetadata);
  AddContextMenuAction("bookmark", tr("BookMark"), MainWindow::on_bookmark,
                       keybookmark);

  toolbar = new DToolBar(this);

#define AddToolBarAction(Icon, Owner, Slot)                                    \
  a = new QAction(Owner);                                                      \
  a->setIcon(ICONRES(Icon));                                                   \
  connect(a, &QAction::triggered, this, &Slot);                                \
  Owner->addAction(a);

#define AddToolBarTool(Icon, Slot) AddToolBarAction(Icon, toolbar, Slot)

  AddToolBarTool("new", MainWindow::on_newfile);
  AddToolBarTool("open", MainWindow::on_openfile);
  AddToolBarTool("opendriver", MainWindow::on_opendriver);
  toolbar->addSeparator();
  AddToolBarTool("save", MainWindow::on_savefile);
  AddToolBarTool("saveas", MainWindow::on_saveasfile);
  AddToolBarTool("export", MainWindow::on_exportfile);
  toolbar->addSeparator();
  AddToolBarTool("undo", MainWindow::on_undofile);
  AddToolBarTool("redo", MainWindow::on_redofile);
  AddToolBarTool("cut", MainWindow::on_cutfile);
  AddToolBarTool("copy", MainWindow::on_copyfile);
  AddToolBarTool("paste", MainWindow::on_pastefile);
  AddToolBarTool("del", MainWindow::on_delete);
  toolbar->addSeparator();
  AddToolBarTool("find", MainWindow::on_findfile);
  AddToolBarTool("jmp", MainWindow::on_gotoline);
  toolbar->addSeparator();
  AddToolBarTool("metadata", MainWindow::on_metadata);
  AddToolBarTool("bookmark", MainWindow::on_bookmark);
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
      auto d = DMessageManager::instance();
      d->sendMessage(this, ICONRES("mAddr"), tr("ErrBaseAddress"));
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

  // dockwidgets init
  auto dw = new DDockWidget(this);
  findresult = new DTableWidget(0, 3, this);
  findresult->setEditTriggers(DTableWidget::EditTrigger::NoEditTriggers);
  findresult->setSelectionBehavior(
      QAbstractItemView::SelectionBehavior::SelectRows);
  findresult->setHorizontalHeaderLabels(
      QStringList({tr("file"), tr("addr"), tr("value")}));
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
  dw->setWidget(findresult);
  this->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, dw);

  dw = new DDockWidget(this);
  pluginInfo = new QTextBrowser(this);
  dw->setWindowTitle(tr("Log"));
  pluginInfo->setFocusPolicy(Qt::StrongFocus);
  pluginInfo->setOpenExternalLinks(true);
  dw->setWidget(pluginInfo);
  this->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, dw);

  logger = new Logger(this);
  connect(logger, &Logger::log,
          [=](QString msg) { pluginInfo->insertHtml(msg); });

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

  logger->logMessage(INFOLOG(tr("Loading")));

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

  // init plugin system
  plgsys = new PluginSystem(this);
  connect(plgsys, &PluginSystem::PluginCall, this, &MainWindow::PluginCall);
  connect(plgsys, &PluginSystem::PluginMenuNeedAdd, this,
          &MainWindow::PluginMenuNeedAdd);
  connect(plgsys, &PluginSystem::PluginDockWidgetAdd, this,
          &MainWindow::PluginDockWidgetAdd);
  plgsys->LoadPlugin();
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

void MainWindow::PluginMenuNeedAdd(QMenu *menu) { plgmenu->addMenu(menu); }

void MainWindow::PluginDockWidgetAdd(QDockWidget *dockw,
                                     Qt::DockWidgetArea align) {
  dockw->setParent(this);
  addDockWidget(align, dockw);
}

bool MainWindow::PluginCall(CallTableIndex index, QList<QVariant> params) {
  switch (index) {
  case CallTableIndex::NewFile: {
    newFile();
  } break;
  case CallTableIndex::OpenFile: {
    if (params.count() == 0)
      return false;

    auto filename = params[0].toString();
    auto readonly = false;
    if (params.count() > 1) {
      readonly = params[1].toBool();
    }
    openFile(filename, readonly);

  } break;
  case CallTableIndex::OpenFileGUI:
    on_openfile();
    break;
  case CallTableIndex::Redo:
    on_redofile();
    break;
  case CallTableIndex::Undo:
    on_undofile();
    break;

  case CallTableIndex::HexMetadataAbs: {
    if (params.count() == 0 || hexfiles.count() == 0)
      return false;

    auto meta = hexeditor->document()->metadata();
    auto data = params[0].value<QHexMetadataAbsoluteItem>();
    meta->metadata(data.begin, data.end, data.foreground, data.background,
                   data.comment);

  } break;
  case CallTableIndex::HexMetadata: {
    if (params.count() == 0 || hexfiles.count() == 0)
      return false;

    auto meta = hexeditor->document()->metadata();
    auto data = params[0].value<QHexMetadataItem>();
    meta->metadata(data.line, data.start, data.length, data.foreground,
                   data.background, data.comment);

  } break;
  case CallTableIndex::ClearMetadata: {
    if (hexfiles.count() == 0)
      return false;
    if (params.count() == 0) {
      hexeditor->document()->metadata()->clear();
    } else {
      bool b = false;
      auto line = params[0].toULongLong(&b);
      if (b) {
        hexeditor->document()->metadata()->clear(line);
      }
      return b;
    }
  } break;
  default:
    return false;
    break;
  }
  return true;
}

void MainWindow::setTheme(DGuiApplicationHelper::ColorType theme) {
  auto p = palette();

  if (theme == DGuiApplicationHelper::LightType) {
  } else {
  }
}

void MainWindow::on_hexeditor_customContextMenuRequested(const QPoint &pos) {
  Q_UNUSED(pos)
  contextMenu->popup(QCursor::pos());
}

void MainWindow::on_tabs_currentChanged(int index) { setFilePage(index); }

void MainWindow::on_tabMoved(int from, int to) { hexfiles.move(from, to); }

void MainWindow::setFilePage(int index) {
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
  hexeditor->setVisible(true);
  auto p = QHexDocument::fromFile<QMemoryBuffer>(nullptr);
  HexFile hf;
  QString title = tr("Untitled") + QString("-%1").arg(defaultindex);
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
}

ErrFile MainWindow::openFile(QString filename, bool readonly) {
  QFileInfo info(filename);
  if (info.exists()) {

    if (!info.permission(QFile::ReadUser))
      return ErrFile::Permission;

    if (!readonly && !info.permission(QFile::WriteUser))
      return ErrFile::Permission;

    for (auto item : hexfiles) {
      if (item.filename == filename)
        return ErrFile::AlreadyOpened;
    }

    hexeditor->setVisible(true);

    HexFile hf;
    auto *p =
        info.size() > FILEMAXBUFFER
            ? QHexDocument::fromLargeFile(filename, readonly, this)
            : QHexDocument::fromFile<QMemoryBuffer>(filename, readonly, this);

    if (p == nullptr)
      return ErrFile::Error;

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

    return ErrFile::Success;
  }
  return ErrFile::NotExist;
}

ErrFile MainWindow::openDriver(QString driver) {
  if (Utilities::isRoot()) {
    openFile(driver);
    return ErrFile::Success;
  } else {
    QMessageBox::critical(this, tr("Error"), tr("NoRoot"));
    return ErrFile::Permission;
  }
}

bool MainWindow::isModified(int index) {
  if (index < 0 || index >= hexfiles.count())
    return false;
  auto p = hexfiles.at(index);
  return p.doc->isModfied();
}

ErrFile MainWindow::closeFile(int index, bool force) {
  if (index >= 0 && index < hexfiles.count()) {
    auto p = hexfiles.at(index);
    if (!force) {
      if (isModified(index))
        return ErrFile::UnSaved;
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

void MainWindow::on_exit() {}

void MainWindow::showEvent(QShowEvent *event) { Q_UNUSED(event); }

void MainWindow::on_savefile() {
  if (saveCurrentFile() == ErrFile::IsNewFile)
    on_saveasfile();
}

void MainWindow::on_delete() { hexeditor->document()->removeSelection(); }

void MainWindow::on_saveasfile() {
  auto filename = QFileDialog::getSaveFileName(this, tr("ChooseSaveFile"));
  if (filename.isEmpty())
    return;
  saveasFile(filename, _currentfile);
}

void MainWindow::on_findfile() {
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
  gotobar->activeInput(int(hexeditor->currentRow()),
                       int(hexeditor->currentColumn()),
                       hexeditor->currentOffset(), hexeditor->documentBytes(),
                       int(hexeditor->documentLines()));
}

void MainWindow::on_gotobar(int pos, bool isline) {
  if (hexfiles.count() > 0) {
    auto cur = hexeditor->document()->cursor();
    isline ? cur->moveTo(quint64(pos), 0) : cur->moveTo(pos);
  }
}

void MainWindow::on_locChanged() {
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
  iSaved->setPixmap(isModified(_currentfile) ? infoUnsaved : infoSaved);
}

void MainWindow::on_documentSwitched() {
  QList<bookMark> bookmaps;
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

void MainWindow::on_setting_plugin() {
  PluginWindow pw(this);
  pw.setPluginSystem(plgsys);
  pw.exec();
}

void MainWindow::on_bookmark() {
  auto doc = hexeditor->document();
  int index = -1;
  if (doc->existBookMark(index)) {
    doc->removeBookMark(index);
    auto item = bookmarks->item(index);
    bookmarks->removeItemWidget(item);
    delete item; // make the removed item disapeared from the list widgets
  } else {
    auto comment =
        DInputDialog ::getText(this, tr("BookMark"), tr("InputComment"));
    if (comment.isEmpty()) {
    } else {
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

void MainWindow::on_sponsor() {
  SponsorDialog d;
  d.exec();
}

void MainWindow::on_about() {
  AboutSoftwareDialog d;
  d.exec();
}
