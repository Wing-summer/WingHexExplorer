#include "mainwindow.h"
#include "QHexView/document/buffer/qfilebuffer.h"
#include "QHexView/document/buffer/qmemorybuffer.h"
#include "QHexView/document/qhexcursor.h"
#include "QHexView/document/qhexmetadata.h"
#include "aboutsoftwaredialog.h"
#include "class/recentfilemanager.h"
#include "driverselectordialog.h"
#include "encodingdialog.h"
#include "finddialog.h"
#include "metadialog.h"
#include "plugin/iwingplugin.h"
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
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
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
  auto picon = Utilities::isRoot() ? ICONRES("iconroot") : ICONRES("icon");
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

  DLabel *l;
  w = new QWidget(this);
  setCentralWidget(w);
  l = new DLabel(w);
  l->setFixedSize(300, 300);
  l->setScaledContents(true);
  auto op = new QGraphicsOpacityEffect(l);
  op->setOpacity(0.1);
  l->setGraphicsEffect(op);
  l->move(10, 10);
  l->setPixmap(QPixmap(Utilities::isRoot() ? ":/images/iconroot.png"
                                           : ":/images/icon.png"));
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

  auto keysaveas =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_S);
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
  auto keymetaedit =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_M);
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
  auto keyexport =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_E);

  auto keyfillnop =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_9);
  auto keyfillzero =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_0);
  auto keyfill = QKeySequence(Qt::KeyboardModifier::ControlModifier |
                              Qt::KeyboardModifier::AltModifier | Qt::Key_F);
  auto keyloadplg = QKeySequence(Qt::KeyboardModifier::ControlModifier |
                                 Qt::KeyboardModifier::AltModifier | Qt::Key_L);

  auto keyencoding =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::AltModifier | Qt::Key_E);
  auto keyopenws =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_W);

  auto keycopyhex =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_C);
  auto keycuthex =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_X);
  auto keypastehex =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_V);

#define AddMenuDB(index)                                                       \
  a->setEnabled(false);                                                        \
  toolmenutools.insert(index, a);

  AddToolSubMenuShortcutAction("workspace", tr("OpenWorkSpace"),
                               MainWindow::on_openworkspace, keyopenws);
  AddToolSubMenuShortcutAction("opendriver", tr("OpenD"),
                               MainWindow::on_opendriver, keyOpenDriver);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("save", tr("Save"), MainWindow::on_save,
                               QKeySequence::Save);
  AddMenuDB(ToolBoxIndex::Save);
  AddToolSubMenuShortcutAction("saveas", tr("SaveAs"), MainWindow::on_saveas,
                               keysaveas);
  AddMenuDB(ToolBoxIndex::SaveAs);
  AddToolSubMenuShortcutAction("export", tr("Export"),
                               MainWindow::on_exportfile, keyexport);
  AddMenuDB(ToolBoxIndex::Export);
  AddToolSubMenuAction("savesel", tr("SaveSel"), MainWindow::on_savesel);
  AddMenuDB(ToolBoxIndex::SaveSel);
  tm->addSeparator();

  auto tmm = new DMenu(this);
  tmm->setTitle(tr("RecentFile"));
  tm->addMenu(tmm);
  recentmanager = new RecentFileManager(tmm, this);
  recentmanager->apply();

  AddToolSubMenuShortcutAction("exit", tr("Exit"), MainWindow::on_exit,
                               QKeySequence::Quit);
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Edit"));
  tm->setIcon(ICONRES("edit"));
  AddToolSubMenuShortcutAction("undo", tr("Undo"), MainWindow::on_undofile,
                               QKeySequence::Undo);
  AddMenuDB(ToolBoxIndex::Undo);
  AddToolSubMenuShortcutAction("redo", tr("Redo"), MainWindow::on_redofile,
                               QKeySequence::Redo);
  AddMenuDB(ToolBoxIndex::Redo);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("cut", tr("Cut"), MainWindow::on_cutfile,
                               QKeySequence::Cut);
  AddMenuDB(ToolBoxIndex::Cut);
  AddToolSubMenuShortcutAction("cuthex", tr("CutHex"), MainWindow::on_cuthex,
                               keycuthex);
  AddMenuDB(ToolBoxIndex::CutHex);
  AddToolSubMenuShortcutAction("copy", tr("Copy"), MainWindow::on_copyfile,
                               QKeySequence::Copy);
  AddMenuDB(ToolBoxIndex::Copy);
  AddToolSubMenuShortcutAction("copyhex", tr("CopyHex"), MainWindow::on_copyhex,
                               keycopyhex);
  AddMenuDB(ToolBoxIndex::CopyHex);
  AddToolSubMenuShortcutAction("paste", tr("Paste"), MainWindow::on_pastefile,
                               QKeySequence::Paste);
  AddMenuDB(ToolBoxIndex::Paste);
  AddToolSubMenuShortcutAction("pastehex", tr("PasteHex"),
                               MainWindow::on_pastehex, keypastehex);
  AddMenuDB(ToolBoxIndex::PasteHex);
  AddToolSubMenuShortcutAction("del", tr("Delete"), MainWindow::on_delete,
                               QKeySequence::Delete);
  AddMenuDB(ToolBoxIndex::Del);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("find", tr("Find"), MainWindow::on_findfile,
                               QKeySequence::Find);
  AddMenuDB(ToolBoxIndex::Find);
  AddToolSubMenuShortcutAction("jmp", tr("Goto"), MainWindow::on_gotoline,
                               keygoto);
  AddMenuDB(ToolBoxIndex::Goto);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("fill", tr("Fill"), MainWindow::on_fill,
                               keyfill);
  AddMenuDB(ToolBoxIndex::Fill);
  AddToolSubMenuShortcutAction("fillNop", tr("FillNop"), MainWindow::on_fillnop,
                               keyfillnop);
  AddMenuDB(ToolBoxIndex::FillNop);
  AddToolSubMenuShortcutAction("fillZero", tr("FillZero"),
                               MainWindow::on_fillzero, keyfillzero);
  AddMenuDB(ToolBoxIndex::FillZero);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("encoding", tr("Encoding"),
                               MainWindow::on_encoding, keyencoding);
  AddMenuDB(ToolBoxIndex::Encoding);
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Mark"));
  tm->setIcon(ICONRES("mark"));
  AddToolSubMenuShortcutAction("metadata", tr("MetaData"),
                               MainWindow::on_metadata, keymetadata);
  AddMenuDB(ToolBoxIndex::Meta);
  AddToolSubMenuShortcutAction("metadataedit", tr("MetaDataEdit"),
                               MainWindow::on_metadataedit, keymetaedit);
  AddMenuDB(ToolBoxIndex::MetaEdit);
  AddToolSubMenuShortcutAction("metadatadel", tr("DeleteMetaData"),
                               MainWindow::on_metadatadel, keymetadatadel);
  AddMenuDB(ToolBoxIndex::DelMeta);
  AddToolSubMenuShortcutAction("metadatacls", tr("ClearMetaData"),
                               MainWindow::on_metadatacls, keymetadatacls);
  AddMenuDB(ToolBoxIndex::ClsMeta);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("bookmark", tr("BookMark"),
                               MainWindow::on_bookmark, keybookmark);
  AddMenuDB(ToolBoxIndex::BookMark);
  AddToolSubMenuShortcutAction("bookmarkdel", tr("DeleteBookMark"),
                               MainWindow::on_bookmarkdel, keybookmarkdel);
  AddMenuDB(ToolBoxIndex::DelBookMark);
  AddToolSubMenuShortcutAction("bookmarkcls", tr("ClearBookMark"),
                               MainWindow::on_bookmarkcls, keybookmarkcls);
  AddMenuDB(ToolBoxIndex::ClsBookMark);
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Setting"));
  tm->setIcon(ICONRES("setting"));
  AddToolSubMenuShortcutAction("general", tr("General"),
                               MainWindow::on_setting_general, keyGeneral);
  AddToolSubMenuShortcutAction("settingplugin", tr("Plugin"),
                               MainWindow::on_setting_plugin, keyplugin);
  settingplg = a;
  AddToolSubMenuAction("layout", tr("RestoreLayout"),
                       MainWindow::on_restoreLayout);
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Plugin"));
  tm->setIcon(ICONRES("plugin"));
  AddToolSubMenuShortcutAction("loadplg", tr("LoadPlugin"),
                               MainWindow::on_loadplg, keyloadplg);
  tm->addSeparator();
  plgmenu = tm;
  menu->addMenu(tm);

  tm = new DMenu(this);
  winmenu = tm;
  tm->setTitle(tr("Window"));
  tm->setIcon(ICONRES("win"));
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Author"));
  tm->setIcon(ICONRES("author"));
  AddToolSubMenuAction("soft", tr("About"), MainWindow::on_about);
  AddToolSubMenuAction("sponsor", tr("Sponsor"), MainWindow::on_sponsor);
  AddToolSubMenuAction("wiki", tr("Wiki"), MainWindow::on_wiki);

  menu->addMenu(tm);

  titlebar()->setMenu(menu);

  hexeditorMenu = new DMenu(this);
  hexeditorMenu->setEnabled(false);

#define AddContextMenuAction(Icon, Title, Slot, ShortCut)                      \
  AddMenuShortcutAction(Icon, Title, Slot, hexeditorMenu, ShortCut)

#define AddContextMenuDB(index)                                                \
  a->setEnabled(false);                                                        \
  conmenutools.insert(index, a);

  AddContextMenuAction("cut", tr("Cut"), MainWindow::on_cutfile,
                       QKeySequence::Cut);
  AddContextMenuAction("cuthex", tr("CutHex"), MainWindow::on_cuthex,
                       keycuthex);
  AddContextMenuAction("copy", tr("Copy"), MainWindow::on_copyfile,
                       QKeySequence::Copy);
  AddContextMenuAction("copyhex", tr("CopyHex"), MainWindow::on_copyhex,
                       keycopyhex);
  AddContextMenuAction("paste", tr("Paste"), MainWindow::on_pastefile,
                       QKeySequence::Paste);
  AddContextMenuAction("pastehex", tr("PasteHex"), MainWindow::on_pastehex,
                       keypastehex);
  AddContextMenuAction("del", tr("Delete"), MainWindow::on_delete,
                       QKeySequence::Delete);
  hexeditorMenu->addSeparator();
  AddContextMenuAction("find", tr("Find"), MainWindow::on_findfile,
                       QKeySequence::Find);
  AddContextMenuAction("jmp", tr("Goto"), MainWindow::on_gotoline, keygoto);
  AddContextMenuAction("fill", tr("Fill"), MainWindow::on_fill, keyfill);
  AddContextMenuAction("metadata", tr("MetaData"), MainWindow::on_metadata,
                       keymetadata);
  AddContextMenuAction("bookmark", tr("BookMark"), MainWindow::on_bookmark,
                       keybookmark);
  AddContextMenuAction("encoding", tr("Encoding"), MainWindow::on_encoding,
                       keyencoding);
  toolbar = new DToolBar(this);
  toolbar->setObjectName("MainToolBar");
#define AddToolBarAction(Icon, Owner, Slot, ToolTip)                           \
  a = new QAction(Owner);                                                      \
  a->setIcon(ICONRES(Icon));                                                   \
  connect(a, &QAction::triggered, this, &Slot);                                \
  a->setToolTip(ToolTip);                                                      \
  Owner->addAction(a);

#define AddToolBarTool(Icon, Slot, ToolTip)                                    \
  AddToolBarAction(Icon, toolbar, Slot, ToolTip)

#define AddToolsDB(index)                                                      \
  a->setEnabled(false);                                                        \
  toolbartools.insert(index, a);

  AddToolBarTool("new", MainWindow::on_newfile, tr("New"));
  AddToolBarTool("open", MainWindow::on_openfile, tr("OpenF"));
  AddToolBarTool("workspace", MainWindow::on_openworkspace,
                 tr("OpenWorkSpace"));
  AddToolBarTool("opendriver", MainWindow::on_opendriver, tr("OpenD"));
  toolbar->addSeparator();
  AddToolBarTool("save", MainWindow::on_save, tr("Save"));
  AddToolsDB(ToolBoxIndex::Save);
  AddToolBarTool("saveas", MainWindow::on_saveas, tr("SaveAs"));
  AddToolsDB(ToolBoxIndex::SaveAs);
  AddToolBarTool("export", MainWindow::on_exportfile, tr("Export"));
  AddToolsDB(ToolBoxIndex::Export);
  toolbar->addSeparator();
  AddToolBarTool("undo", MainWindow::on_undofile, tr("Undo"));
  AddToolsDB(ToolBoxIndex::Undo);
  AddToolBarTool("redo", MainWindow::on_redofile, tr("Redo"));
  AddToolsDB(ToolBoxIndex::Redo);

  DToolButton *tbtn;
  DMenu *tmenu;

#define AddToolBtnBegin(DIcon)                                                 \
  tbtn = new DToolButton(this);                                                \
  tbtn->setEnabled(false);                                                     \
  tbtn->setIcon(ICONRES(DIcon));                                               \
  tmenu = new DMenu(this);

#define AddToolBtnBtn(Icon, Title, Slot)                                       \
  a = new QAction(ICONRES(Icon), Title, this);                                 \
  connect(a, &QAction::triggered, this, &Slot);                                \
  tmenu->addAction(a);

#define AddToolBtnEnd(Index)                                                   \
  tbtn->setMenu(tmenu);                                                        \
  tbtn->setPopupMode(DToolButton::ToolButtonPopupMode::InstantPopup);          \
  toolbar->addWidget(tbtn);                                                    \
  toolbtnstools.insert(Index, tbtn);

  AddToolBtnBegin("cut") {
    AddToolBtnBtn("cut", tr("Cut"), MainWindow::on_cutfile);
    AddToolBtnBtn("cuthex", tr("CutHex"), MainWindow::on_cuthex);
  }
  AddToolBtnEnd(ToolBoxIndex::Cut);

  AddToolBtnBegin("copy") {
    AddToolBtnBtn("copy", tr("Copy"), MainWindow::on_copyfile);
    AddToolBtnBtn("copyhex", tr("CopyHex"), MainWindow::on_copyhex)
  }
  AddToolBtnEnd(ToolBoxIndex::Copy);

  AddToolBtnBegin("paste") {
    AddToolBtnBtn("paste", tr("Paste"), MainWindow::on_pastefile);
    AddToolBtnBtn("pastehex", tr("PasteHex"), MainWindow::on_pastehex);
  }
  AddToolBtnEnd(ToolBoxIndex::Paste);

  AddToolBarTool("del", MainWindow::on_delete, tr("Delete"));
  AddToolsDB(ToolBoxIndex::Del);
  toolbar->addSeparator();
  AddToolBarTool("find", MainWindow::on_findfile, tr("Find"));
  AddToolsDB(ToolBoxIndex::Find);
  AddToolBarTool("jmp", MainWindow::on_gotoline, tr("Goto"));
  AddToolsDB(ToolBoxIndex::Goto);
  toolbar->addSeparator();

  AddToolBtnBegin("fill") {
    AddToolBtnBtn("fill", tr("Fill"), MainWindow::on_fill);
    AddToolBtnBtn("fillNop", tr("FillNop"), MainWindow::on_fillnop);
    AddToolBtnBtn("fillZero", tr("FillZero"), MainWindow::on_fillzero);
  }
  AddToolBtnEnd(ToolBoxIndex::Fill);

  AddToolBtnBegin("metadata") {
    AddToolBtnBtn("metadata", tr("MetaData"), MainWindow::on_metadata);
    AddToolBtnBtn("metadataedit", tr("MetaDataEdit"),
                  MainWindow::on_metadataedit);
    AddToolBtnBtn("metadatadel", tr("DeleteMetaData"),
                  MainWindow::on_metadatadel);
    AddToolBtnBtn("metadatacls", tr("ClearMetaData"),
                  MainWindow::on_metadatacls);
  }
  AddToolBtnEnd(ToolBoxIndex::Meta);

  AddToolBtnBegin("bookmark") {
    AddToolBtnBtn("bookmark", tr("BookMark"), MainWindow::on_bookmark);
    AddToolBtnBtn("bookmarkdel", tr("DeleteBookMark"),
                  MainWindow::on_bookmarkdel);
    AddToolBtnBtn("bookmarkcls", tr("ClearBookMark"),
                  MainWindow::on_bookmarkcls);
  }
  AddToolBtnEnd(ToolBoxIndex::BookMark);

  AddToolBarTool("encoding", MainWindow::on_encoding, tr("Encoding"));
  AddToolsDB(ToolBoxIndex::Encoding);
  toolbar->addSeparator();
  AddToolBarTool("general", MainWindow::on_setting_general, tr("General"));
  AddToolBarTool("soft", MainWindow::on_about, tr("About"));
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
  connect(hexeditor, &QHexView::documentBookMarkChanged, this,
          &MainWindow::on_bookmarkChanged);
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

  AddFunctionIconButton(iSetBaseAddr, "mAddr");
  iSetBaseAddr->setToolTip(tr("SetaddressBase"));
  connect(iSetBaseAddr, &DIconButton::clicked, [=] {
    DInputDialog d;
    bool b;
    auto num = d.getText(this, tr("addressBase"), tr("inputAddressBase"),
                         QLineEdit::Normal, QString(), &b);
    if (b) {
      qulonglong qnum = num.toULongLong(&b, 0);
      if (b) {
        hexeditor->setAddressBase(qnum);
      } else {
        if (num.length() > 0) {
          auto d = DMessageManager::instance();
          d->sendMessage(this, ICONRES("mAddr"), tr("ErrBaseAddress"));
        }
      }
    }
  });

  AddFunctionIconButton(iColInfo, "mColInfo");
  iColInfo->setToolTip(tr("SetColInfo"));
  connect(iColInfo, &DIconButton::clicked,
          [=] { hexeditor->setAddressVisible(!hexeditor->addressVisible()); });
  AddFunctionIconButton(iHeaderInfo, "mLineInfo");
  iHeaderInfo->setToolTip(tr("SetHeaderInfo"));
  connect(iHeaderInfo, &DIconButton::clicked,
          [=] { hexeditor->setHeaderVisible(!hexeditor->headerVisible()); });
  AddFunctionIconButton(iAsciiString, "mStr");
  iAsciiString->setToolTip(tr("SetAsciiString"));
  connect(iAsciiString, &DIconButton::clicked,
          [=] { hexeditor->setAsciiVisible(!hexeditor->asciiVisible()); });

  AddPermanentStatusLabel(QString(2, ' '));
  AddStatusLabel(tr("loc:"));
  l->setMinimumWidth(50);
  l->setAlignment(Qt::AlignCenter);

  AddNamedStatusLabel(lblloc, "(0,0)");
  connect(hexeditor, &QHexView::cursorLocationChanged, this,
          &MainWindow::on_locChanged);
  AddStatusLabel(tr("sel:"));
  l->setMinimumWidth(50);
  l->setAlignment(Qt::AlignCenter);

  AddNamedStatusLabel(lblsellen, "0 - 0x0");
  AddStatusLabel(QString(5, ' '));

#define LoadPixMap(Var, Icon) Var.load(":/images/" Icon ".png");

#define AddStausILable(PixMap, Icon, Label, OPixMap, OIcon, GPixMap, GIcon)    \
  LoadPixMap(PixMap, Icon);                                                    \
  LoadPixMap(OPixMap, OIcon);                                                  \
  LoadPixMap(GPixMap, GIcon) Label = new DLabel(this);                         \
  Label->setPixmap(GPixMap);                                                   \
  Label->setScaledContents(true);                                              \
  Label->setFixedSize(20, 20);                                                 \
  Label->setAlignment(Qt::AlignCenter);                                        \
  status->addWidget(Label);                                                    \
  AddStatusLabel(QString(' '));

  AddStausILable(infoSaved, "saved", iSaved, infoUnsaved, "unsaved", infoSaveg,
                 "saveg");
  iSaved->setToolTip(tr("InfoSave"));
  AddStausILable(infoWriteable, "writable", iReadWrite, infoReadonly,
                 "readonly", inforwg, "rwg");
  iReadWrite->setToolTip(tr("InfoReadWrite"));
  AddStausILable(infow, "works", iw, infouw, "unworks", infowg, "worksg");
  iw->setToolTip(tr("InfoWorks"));
  infoUnLock = ICONRES("unlock");
  infoLock = ICONRES("lock");
  infoCanOver = ICONRES("canover");
  infoCannotOver = ICONRES("unover");
  infoLockg = ICONRES("lockg");
  infoOverg = ICONRES("overg");

  iLocked = new DIconButton(this);
  iLocked->setIcon(infoLockg);
  iLocked->setIconSize(QSize(20, 20));
  iLocked->setToolTip(tr("SetLocked"));
  iOver = new DIconButton(this);
  iOver->setIcon(infoOverg);
  iOver->setIconSize(QSize(20, 20));
  iOver->setToolTip(tr("SetOver"));

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
  AddMenuAction("export", tr("ExportFindResult"),
                MainWindow::on_exportfindresult, findresultMenu);
  AddMenuAction("del", tr("ClearFindResult"), MainWindow::on_clearfindresult,
                findresultMenu);

  // dockwidgets init
#define AddDockWin(title)                                                      \
  a = new QAction(this);                                                       \
  a->setText(title);                                                           \
  connect(a, &QAction::triggered, this, [dw] {                                 \
    dw->show();                                                                \
    dw->raise();                                                               \
  });                                                                          \
  winmenu->addAction(a);

#define AddDockWin2(title)                                                     \
  a = new QAction(this);                                                       \
  a->setText(title);                                                           \
  connect(a, &QAction::triggered, this, [dw2] {                                \
    dw2->show();                                                               \
    dw2->raise();                                                              \
  });                                                                          \
  winmenu->addAction(a);

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
  AddDockWin(tr("FindResult"));
  this->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, dw);

  dw = new DDockWidget(this);
  AddDockWin(tr("Log"));
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
  AddDockWin2(tr("Number"));
  numshowtable = new DTableWidget(8, 1, this);
  numshowtable->setEditTriggers(DTableWidget::EditTrigger::NoEditTriggers);
  numshowtable->setSelectionBehavior(
      QAbstractItemView::SelectionBehavior::SelectRows);
  numshowtable->setHorizontalHeaderLabels(QStringList(tr("Value")));
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
  AddDockWin(tr("BookMark"));
  bookmarks = new DListWidget(this);
  bookmarks->setFocusPolicy(Qt::StrongFocus);
  connect(bookmarks, &DListWidget::itemDoubleClicked, [=]() {
    hexeditor->renderer()->enableCursor(true);
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

  // connect hexeditor status
  connect(hexeditor, &QHexView::canUndoChanged, [=](bool b) {
    toolbartools[ToolBoxIndex::Undo]->setEnabled(b);
    toolmenutools[ToolBoxIndex::Undo]->setEnabled(b);
  });
  connect(hexeditor, &QHexView::canRedoChanged, [=](bool b) {
    toolbartools[ToolBoxIndex::Redo]->setEnabled(b);
    toolmenutools[ToolBoxIndex::Redo]->setEnabled(b);
  });
  connect(hexeditor, &QHexView::documentSaved,
          [=](bool b) { iSaved->setPixmap(b ? infoSaved : infoUnsaved); });
  connect(hexeditor, &QHexView::documentKeepSize, this,
          [=](bool b) { iOver->setIcon(b ? infoCannotOver : infoCanOver); });
  connect(hexeditor, &QHexView::documentLockedFile, this,
          [=](bool b) { iLocked->setIcon(b ? infoLock : infoUnLock); });

#define ConnectShortCut(ShortCut, Slot)                                        \
  s = new QShortcut(ShortCut, this);                                           \
  connect(s, &QShortcut::activated, this, &Slot);
  QShortcut *s;
  ConnectShortCut(QKeySequence::New, MainWindow::on_newfile);
  ConnectShortCut(QKeySequence::Open, MainWindow::on_openfile);
  ConnectShortCut(QKeySequence::Save, MainWindow::on_save);
  ConnectShortCut(keysaveas, MainWindow::on_saveas);
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
  ConnectShortCut(keyexport, MainWindow::on_exportfile);
  ConnectShortCut(keyloadplg, MainWindow::on_loadplg);
  ConnectShortCut(keyencoding, MainWindow::on_encoding);
  ConnectShortCut(keyopenws, MainWindow::on_openworkspace);
  ConnectShortCut(keycuthex, MainWindow::on_cuthex);
  ConnectShortCut(keycopyhex, MainWindow::on_copyhex);
  ConnectShortCut(keypastehex, MainWindow::on_pastehex);

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
  connect(m_settings, &Settings::sigChangeRootPluginEnabled,
          [=](bool b) { _rootenableplugin = b; });
  connect(m_settings, &Settings::sigChangedEncoding,
          [=](QString encoding) { _encoding = encoding; });
  connect(m_settings, &Settings::sigAdjustFindMaxCount,
          [=](int count) { _findmax = count; });

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

  auto enplugin = _enableplugin;
  if (_enableplugin) {
    if (!_rootenableplugin && Utilities::isRoot())
      enplugin = false;
  }

  if (enplugin) {
    logger->logMessage(INFOLOG(tr("PluginLoading")));
    winmenu->addSeparator();
    // init plugin system
    plgsys = new PluginSystem(this);
    connect(plgsys, &PluginSystem::ConnectBase, this, &MainWindow::connectBase);
    connect(plgsys, &PluginSystem::ConnectControl, this,
            &MainWindow::connectControl);
    connect(plgsys, &PluginSystem::PluginMenuNeedAdd, this,
            &MainWindow::PluginMenuNeedAdd);
    connect(plgsys, &PluginSystem::PluginDockWidgetAdd, this,
            &MainWindow::PluginDockWidgetAdd);
    plgsys->LoadPlugin();
  } else {
    plgmenu->setEnabled(false);
    logger->logMessage(ERRLOG(tr("UnLoadPluginSetting")));
    settingplg->setEnabled(false);
  }

  m_settings->saveWindowState(this, true);
  m_settings->loadWindowState(this);
  lastusedpath = m_settings->loadFileDialogCurrent();
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
    auto t = dockw->windowTitle();
    if (!t.trimmed().length()) {
      logger->logMessage(ERRLOG(tr("ErrDockWidgetAddNoName")));
      return;
    }
    logger->logMessage(WARNLOG(tr("DockWidgetName :") + t));
    dockw->setParent(this);
    addDockWidget(align, dockw);
    auto a = new QAction(t, winmenu);
    connect(a, &QAction::triggered, this, [dockw] {
      dockw->show();
      dockw->raise();
    });
    winmenu->addAction(a);
  }
}

void MainWindow::connectBase(IWingPlugin *plugin) {
  if (plugin == nullptr)
    return;

#define ConnectBase(Signal, Slot) connect(plugin, &Signal, this, &Slot)
#define ConnectBaseLamba(Signal, Function) connect(plugin, &Signal, Function)
#define ConnectBase2(Signal, Slot)                                             \
  connect(&plugin->reader, &Signal, this, &Slot)
#define ConnectBaseLamba2(Signal, Function)                                    \
  connect(&plugin->reader, &Signal, Function)

  // connect neccessary signal-slot
  ConnectBase(IWingPlugin::requestControl, MainWindow::requestControl);
  ConnectBase(IWingPlugin::requestRelease, MainWindow::requestRelease);

#define PCHECK(T, F)                                                           \
  if (hexfiles.count() > 0 && _pcurfile >= 0)                                  \
    T;                                                                         \
  F;

#define PCHECKRETURN(T, F)                                                     \
  if (hexfiles.count() > 0 && _pcurfile >= 0)                                  \
    return T;                                                                  \
  return F;

  ConnectBaseLamba2(WingPlugin::Reader::isLocked, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->isLocked(), true);
  });

  ConnectBaseLamba2(WingPlugin::Reader::isLocked, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->isLocked(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::isEmpty, [=] {
    PCHECKRETURN(hexeditor->document()->isEmpty(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::isKeepSize, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->isKeepSize(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::isModified, [=] {
    PCHECKRETURN(!hexfiles[_pcurfile].doc->isDocSaved(), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::isReadOnly, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->isReadOnly(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::documentLines, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->documentLines(), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::documentBytes, [=] {
    PCHECKRETURN(quint64(hexfiles[_pcurfile].doc->length()), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::currentPos, [=] {
    HexPosition pos;
    PCHECK(
        {
          auto qpos = hexfiles[_pcurfile].doc->cursor()->position();
          pos.line = qpos.line;
          pos.column = qpos.column;
          pos.lineWidth = qpos.lineWidth;
          pos.nibbleindex = qpos.nibbleindex;
        },
        return pos);
  });
  ConnectBaseLamba2(WingPlugin::Reader::selectionPos, [=] {
    HexPosition pos;
    PCHECK(
        {
          auto cur = hexfiles[_pcurfile].doc->cursor();
          pos.line = cur->selectionLine();
          pos.column = cur->selectionColumn();
          pos.nibbleindex = cur->selectionNibble();
          pos.lineWidth = cur->position().lineWidth;
        },
        return pos);
  });
  ConnectBaseLamba2(WingPlugin::Reader::currentRow, [=] {
    PCHECKRETURN(quint64(hexfiles[_pcurfile].doc->cursor()->currentLine()),
                 quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::currentColumn, [=] {
    PCHECKRETURN(quint64(hexfiles[_pcurfile].doc->cursor()->currentColumn()),
                 quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::currentOffset, [=] {
    PCHECKRETURN(
        quint64(hexfiles[_pcurfile].doc->cursor()->position().offset()),
        quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::selectlength, [=] {
    PCHECKRETURN(quint64(hexfiles[_pcurfile].doc->cursor()->selectionLength()),
                 quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::asciiVisible, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->asciiVisible(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::headerVisible, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->headerVisible(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::addressVisible, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->addressVisible(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::addressBase, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->baseAddress(), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::atEnd, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->atEnd(), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::canUndo, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->canUndo(), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::canRedo, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->canRedo(), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::areaIndent, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->areaIndent(), 0);
  });
  ConnectBaseLamba2(WingPlugin::Reader::hexLineWidth, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->hexLineWidth(), 0);
  });
  ConnectBaseLamba2(WingPlugin::Reader::editableArea, [=](int area) {
    PCHECKRETURN(hexfiles[_pcurfile].render->editableArea(area), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::documentLastLine, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->documentLastLine(), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::documentLastColumn, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->documentLastColumn(), 0);
  });
  ConnectBaseLamba2(WingPlugin::Reader::documentWidth, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->documentWidth(), 0);
  });
  ConnectBaseLamba2(WingPlugin::Reader::lineHeight, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->lineHeight(), 0);
  });
  ConnectBaseLamba2(
      WingPlugin::Reader::getLineRect, [=](quint64 line, quint64 firstline) {
        PCHECKRETURN(hexfiles[_pcurfile].render->getLineRect(line, firstline),
                     QRect());
      });
  ConnectBaseLamba2(WingPlugin::Reader::headerLineCount, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->headerLineCount(), 0);
  });
  ConnectBaseLamba2(WingPlugin::Reader::borderSize, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->borderSize(), 0);
  });
  ConnectBaseLamba2(WingPlugin::Reader::copy, [=](bool hex) {
    PCHECK(hexfiles[_pcurfile].doc->copy(hex), );
  });
  ConnectBaseLamba2(WingPlugin::Reader::read, [=](qint64 offset, int len) {
    PCHECKRETURN(hexfiles[_pcurfile].doc->read(offset, len), QByteArray());
  });
  ConnectBaseLamba2(
      WingPlugin::Reader::findAllBytes,
      [=](qlonglong begin, qlonglong end, QByteArray b,
          QList<quint64> &results) {
        PCHECK(hexfiles[_pcurfile].doc->findAllBytes(begin, end, b, results), );
      });
  ConnectBaseLamba2(
      WingPlugin::Reader::searchForward, [=](const QByteArray &ba) {
        PCHECKRETURN(hexfiles[_pcurfile].doc->searchForward(ba), qint64(-1));
      });
  ConnectBaseLamba2(
      WingPlugin::Reader::searchBackward, [=](const QByteArray &ba) {
        PCHECKRETURN(hexfiles[_pcurfile].doc->searchBackward(ba), qint64(-1));
      });
  ConnectBaseLamba2(WingPlugin::Reader::getMetaLine, [=](quint64 line) {
    auto ometas = hexfiles[_pcurfile].doc->metadata()->get(line);
    HexLineMetadata metas;
    for (auto item : ometas) {
      metas.push_back(HexMetadataItem(item.line, item.start, item.length,
                                      item.foreground, item.background,
                                      item.comment));
    }
    return metas;
  });
  ConnectBaseLamba2(WingPlugin::Reader::getMetadatas, [=](qint64 offset) {
    auto ometaline = hexfiles[_pcurfile].doc->metadata()->gets(offset);
    QList<HexMetadataAbsoluteItem> metaline;
    for (auto item : ometaline) {
      metaline.push_back(
          HexMetadataAbsoluteItem(item.begin, item.end, item.foreground,
                                  item.background, item.comment));
    }
    return metaline;
  });
  ConnectBaseLamba2(WingPlugin::Reader::lineHasMetadata, [=](quint64 line) {
    return hexfiles[_pcurfile].doc->metadata()->lineHasMetadata(line);
  });
  ConnectBaseLamba2(WingPlugin::Reader::getOpenFiles, [=] {
    QList<QString> files;
    for (auto item : hexfiles) {
      files.push_back(item.filename);
    }
    return files;
  });
  ConnectBase2(WingPlugin::Reader::getSupportedEncodings,
               Utilities::GetEncodings);
  ConnectBaseLamba2(WingPlugin::Reader::currentEncoding,
                    [=] { return hexfiles[_pcurfile].render->encoding(); });
}

void MainWindow::connectControl(IWingPlugin *plugin) {

#define ConnectControl(Signal, Slot) connect(plugin, &Signal, this, &Slot)
#define ConnectControlLamba(Signal, Function) connect(plugin, &Signal, Function)
#define ConnectControl2(Signal, Slot)                                          \
  connect(&plugin->controller, &Signal, this, &Slot)
#define ConnectControlLamba2(Signal, Function)                                 \
  connect(&plugin->controller, &Signal, Function)

  ConnectControlLamba2(WingPlugin::Controller::switchDocument,
                       [=](int index, bool gui) {
                         if (gui) {
                           setFilePage(index);
                           _pcurfile = _currentfile;
                         } else {
                           if (index >= 0 && index < hexfiles.count())
                             _pcurfile = index;
                         }
                       });

  ConnectControlLamba2(WingPlugin::Controller::setLockedFile, [=](bool b) {
    hexfiles[_pcurfile].doc->setLockedFile(b);
  });
  ConnectControlLamba2(WingPlugin::Controller::setKeepSize, [=](bool b) {
    hexfiles[_pcurfile].doc->setKeepSize(b);
  });
  ConnectControlLamba2(WingPlugin::Controller::setAsciiVisible, [=](bool b) {
    hexfiles[_pcurfile].render->setAsciiVisible(b);
  });
  ConnectControlLamba2(WingPlugin::Controller::setHeaderVisible, [=](bool b) {
    hexfiles[_pcurfile].render->setHeaderVisible(b);
  });
  ConnectControlLamba2(WingPlugin::Controller::setAddressVisible, [=](bool b) {
    hexfiles[_pcurfile].render->setAddressVisible(b);
  });
  ConnectControlLamba2(
      WingPlugin::Controller::setAddressBase,
      [=](quint64 base) { hexfiles[_pcurfile].doc->setBaseAddress(base); });
  ConnectControlLamba2(
      WingPlugin::Controller::setAreaIndent,
      [=](quint8 value) { hexfiles[_pcurfile].doc->setAreaIndent(value); });
  ConnectControlLamba2(
      WingPlugin::Controller::setHexLineWidth,
      [=](quint8 value) { hexfiles[_pcurfile].doc->setHexLineWidth(value); });
  ConnectControlLamba2(WingPlugin::Controller::undo,
                       [=] { hexfiles[_pcurfile].doc->undo(); });
  ConnectControlLamba2(WingPlugin::Controller::redo,
                       [=] { hexfiles[_pcurfile].doc->redo(); });
  ConnectControlLamba2(WingPlugin::Controller::cut,
                       [=](bool hex) { hexfiles[_pcurfile].doc->cut(hex); });
  ConnectControlLamba2(WingPlugin::Controller::paste,
                       [=](bool hex) { hexfiles[_pcurfile].doc->paste(hex); });

#define ConnectControlLamba3(Signal, Function)                                 \
  connect(&plugin->controller, Signal, Function)

  void (WingPlugin::Controller::*insertchar)(qint64 offset, uchar b) =
      &WingPlugin::Controller::insert;
  void (WingPlugin::Controller::*insertarr)(
      qint64 offset, const QByteArray &data) = &WingPlugin::Controller::insert;

  ConnectControlLamba3(insertchar, [=](qint64 offset, uchar b) {
    hexfiles[_pcurfile].doc->insert(offset, b);
  });
  ConnectControlLamba3(insertarr, [=](qint64 offset, const QByteArray &data) {
    hexfiles[_pcurfile].doc->insert(offset, data);
  });

  void (WingPlugin::Controller::*replacechar)(qint64 offset, uchar b) =
      &WingPlugin::Controller::replace;
  void (WingPlugin::Controller::*replacearr)(
      qint64 offset, const QByteArray &data) = &WingPlugin::Controller::replace;
  ConnectControlLamba3(replacechar, [=](qint64 offset, uchar b) {
    hexfiles[_pcurfile].doc->replace(offset, b);
  });
  ConnectControlLamba3(replacearr, [=](qint64 offset, const QByteArray &data) {
    hexfiles[_pcurfile].doc->replace(offset, data);
  });
  ConnectControlLamba2(WingPlugin::Controller::remove,
                       [=](qint64 offset, int len) {
                         hexfiles[_pcurfile].doc->remove(offset, len);
                       });

  void (WingPlugin::Controller::*moveToHP)(const HexPosition &pos) =
      &WingPlugin::Controller::moveTo;
  void (WingPlugin::Controller::*moveTo)(quint64 line, int column,
                                         int nibbleindex) =
      &WingPlugin::Controller::moveTo;

  void (WingPlugin::Controller::*moveToOff)(qint64 offset);
  ConnectControlLamba3(moveToHP, [=](const HexPosition &pos) {
    QHexPosition p;
    p.line = pos.line;
    p.column = pos.column;
    p.lineWidth = pos.lineWidth;
    p.nibbleindex = pos.nibbleindex;
    hexfiles[_pcurfile].doc->cursor()->moveTo(p);
  });
  ConnectControlLamba3(moveTo, [=](quint64 line, int column, int nibbleindex) {
    hexfiles[_pcurfile].doc->cursor()->moveTo(line, column, nibbleindex);
  });
  ConnectControlLamba3(moveToOff, [=](qint64 offset) {
    hexfiles[_pcurfile].doc->cursor()->moveTo(offset);
  });

  void (WingPlugin::Controller::*selectHP)(const HexPosition &pos) =
      &WingPlugin::Controller::select;
  void (WingPlugin::Controller::*select)(quint64 line, int column,
                                         int nibbleindex) =
      &WingPlugin::Controller::select;

  void (WingPlugin::Controller::*selectL)(int length);
  ConnectControlLamba3(selectHP, [=](const HexPosition &pos) {
    QHexPosition p;
    p.line = pos.line;
    p.column = pos.column;
    p.lineWidth = pos.lineWidth;
    p.nibbleindex = pos.nibbleindex;
    hexfiles[_pcurfile].doc->cursor()->select(p);
  });
  ConnectControlLamba3(select, [=](quint64 line, int column, int nibbleindex) {
    hexfiles[_pcurfile].doc->cursor()->select(line, column, nibbleindex);
  });
  ConnectControlLamba3(selectL, [=](int length) {
    hexfiles[_pcurfile].doc->cursor()->select(length);
  });

  ConnectControlLamba2(
      WingPlugin::Controller::selectOffset, [=](qint64 offset, int length) {
        hexfiles[_pcurfile].doc->cursor()->selectOffset(offset, length);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::setInsertionMode, [=](bool isinsert) {
        hexfiles[_pcurfile].doc->cursor()->setInsertionMode(
            isinsert ? QHexCursor::InsertMode : QHexCursor::OverwriteMode);
      });
  ConnectControlLamba2(WingPlugin::Controller::setLineWidth, [=](quint8 width) {
    hexfiles[_pcurfile].doc->cursor()->setLineWidth(width);
  });

  void (WingPlugin::Controller::*metadata)(
      qint64 begin, qint64 end, const QColor &fgcolor, const QColor &bgcolor,
      const QString &comment) = &WingPlugin::Controller::metadata;

  void (WingPlugin::Controller::*metadataL)(
      quint64 line, int start, int length, const QColor &fgcolor,
      const QColor &bgcolor, const QString &comment) =
      &WingPlugin::Controller::metadata;

  ConnectControlLamba3(metadata,
                       [=](qint64 begin, qint64 end, const QColor &fgcolor,
                           const QColor &bgcolor, const QString &comment) {
                         hexfiles[_pcurfile].doc->metadata()->metadata(
                             begin, end, fgcolor, bgcolor, comment);
                       });
  ConnectControlLamba3(
      metadataL, [=](quint64 line, int start, int length, const QColor &fgcolor,
                     const QColor &bgcolor, const QString &comment) {
        hexfiles[_pcurfile].doc->metadata()->metadata(
            line, start, length, fgcolor, bgcolor, comment);
      });

  ConnectControlLamba2(
      WingPlugin::Controller::removeMetadata, [=](qint64 offset) {
        hexfiles[_pcurfile].doc->metadata()->removeMetadata(offset);
      });

  void (WingPlugin::Controller::*clear)() = &WingPlugin::Controller::clearMeta;
  ConnectControlLamba3(clear,
                       [=]() { hexfiles[_pcurfile].doc->metadata()->clear(); });

  ConnectControlLamba2(WingPlugin::Controller::color,
                       [=](quint64 line, int start, int length,
                           const QColor &fgcolor, const QColor &bgcolor) {
                         hexfiles[_pcurfile].doc->metadata()->color(
                             line, start, length, fgcolor, bgcolor);
                       });

  ConnectControlLamba2(
      WingPlugin::Controller::comment,
      [=](quint64 line, int start, int length, const QString &comment) {
        hexfiles[_pcurfile].doc->metadata()->comment(line, start, length,
                                                     comment);
      });

  ConnectControlLamba2(
      WingPlugin::Controller::foreground,
      [=](quint64 line, int start, int length, const QColor &fgcolor) {
        hexfiles[_pcurfile].doc->metadata()->foreground(line, start, length,
                                                        fgcolor);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::background,
      [=](quint64 line, int start, int length, const QColor &bgcolor) {
        hexfiles[_pcurfile].doc->metadata()->background(line, start, length,
                                                        bgcolor);
      });

  ConnectControlLamba2(WingPlugin::Controller::setCurrentEncoding,
                       [=](QString encoding) {
                         hexfiles[_pcurfile].render->setEncoding(encoding);
                       });

  ConnectControlLamba2(WingPlugin::Controller::openWorkSpace,
                       [=](QString filename, bool readonly) {
                         openWorkSpace(filename, readonly);
                       });
  ConnectControl2(WingPlugin::Controller::newFile, MainWindow::newFile);
  ConnectControlLamba2(
      WingPlugin::Controller::openFile,
      [=](QString filename, bool readonly) { openFile(filename, readonly); });
  ConnectControl2(WingPlugin::Controller::openDriver, MainWindow::openDriver);
  ConnectControl2(WingPlugin::Controller::closeFile, MainWindow::closeFile);
  ConnectControl2(WingPlugin::Controller::saveFile, MainWindow::save);
  ConnectControl2(WingPlugin::Controller::exportFile, MainWindow::exportFile);
  ConnectControl2(WingPlugin::Controller::exportFileGUI,
                  MainWindow::on_exportfile);
  ConnectControl2(WingPlugin::Controller::saveasFile, MainWindow::saveAs);
  ConnectControl2(WingPlugin::Controller::saveasFileGUI, MainWindow::on_saveas);
  ConnectControl2(WingPlugin::Controller::closeCurrentFile,
                  MainWindow::closeCurrentFile);
  ConnectControl2(WingPlugin::Controller::saveCurrentFile,
                  MainWindow::saveCurrent);
  ConnectControl2(WingPlugin::Controller::openFileGUI, MainWindow::on_openfile);
  ConnectControl2(WingPlugin::Controller::openDriverGUI,
                  MainWindow::on_opendriver);
  ConnectControl2(WingPlugin::Controller::gotoGUI, MainWindow::on_gotoline);
  ConnectControl2(WingPlugin::Controller::findGUI, MainWindow::on_findfile);
  ConnectControl2(WingPlugin::Controller::fillGUI, MainWindow::on_fill);
  ConnectControl2(WingPlugin::Controller::fillzeroGUI, MainWindow::on_fillzero);
  ConnectControl2(WingPlugin::Controller::fillnopGUI, MainWindow::on_fillnop);
}

bool MainWindow::requestControl(IWingPlugin *plugin) {
  return plgsys->requestControl(plugin);
}
bool MainWindow::requestRelease(IWingPlugin *plugin) {
  return plgsys->requestRelease(plugin);
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
    _pcurfile = -1;
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
    enableDirverLimit(d.isdriver);
    tabs->setCurrentIndex(index);
  }

  // check the plugin file index validation
  if (_pcurfile >= hexfiles.count()) {
    _pcurfile = -1;
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
  hexeditor->renderer()->setEncoding(_encoding);
  hf.render = hexeditor->renderer();
  hf.vBarValue = -1;
  hf.filename = ":" + title;
  hf.workspace = "";
  hf.isdriver = false;
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

ErrFile MainWindow::openFile(QString filename, bool readonly, int *openedindex,
                             QString workspace, bool *oldworkspace) {
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

    int i = 0;
    for (auto item : hexfiles) {
      if (item.filename == filename) {
        if (oldworkspace) {
          *oldworkspace = item.workspace.length() > 0;
        }
        if (openedindex) {
          *openedindex = i;
        }
        if (_enableplugin) {
          params[0].setValue(HookIndex::OpenFileEnd);
          params << ErrFile::AlreadyOpened;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::AlreadyOpened;
      }
      i++;
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
    hf.filename = filename;
    hf.workspace = workspace;
    hf.vBarValue = -1;
    p->isWorkspace = workspace.length() > 0;
    hexeditor->setLockedFile(readonly);
    hexeditor->setDocument(p);
    hexeditor->setKeepSize(true);
    hf.isdriver = false;
    hexeditor->renderer()->setEncoding(_encoding);
    hf.render = hexeditor->renderer();

    hexfiles.push_back(hf);

    QIcon qicon;

    if (p->isWorkspace) {
      qicon = ICONRES("pro");
    } else {
      QMimeDatabase db;
      auto t = db.mimeTypeForFile(filename);
      auto ico = t.iconName();
      qicon = QIcon::fromTheme(ico, QIcon(ico));
    }

    tabs->addTab(qicon, info.fileName());
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
    QFileInfo info(driver);

    if (info.exists()) {
      if (!info.permission(QFile::ReadUser)) {
        if (_enableplugin) {
          params[0].setValue(HookIndex::OpenFileEnd);
          params << ErrFile::Permission;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::Permission;
      }

      if (!info.permission(QFile::WriteUser)) {
        if (_enableplugin) {
          params[0].setValue(HookIndex::OpenFileEnd);
          params << ErrFile::Permission;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::Permission;
      }

      for (auto item : hexfiles) {
        if (item.filename == driver) {
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
      auto *p = QHexDocument::fromLargeFile(driver, false, this);
      if (p == nullptr) {
        if (_enableplugin) {
          params[0].setValue(HookIndex::OpenFileEnd);
          params << ErrFile::Error;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::Error;
      }

      hf.doc = p;
      hexeditor->setDocument(p);
      hexeditor->setKeepSize(true);
      hf.isdriver = true;
      hf.render = hexeditor->renderer();
      hf.vBarValue = -1;
      hf.filename = driver;
      hexfiles.push_back(hf);

      QMimeDatabase db;
      auto t = db.mimeTypeForFile(driver);
      auto ico = t.iconName();
      tabs->addTab(QIcon::fromTheme(ico, QIcon(ico)), info.fileName());
      auto index = hexfiles.count() - 1;
      tabs->setCurrentIndex(index);
      tabs->setTabToolTip(index, driver);
      setEditModeEnabled(true);

      if (_enableplugin) {
        params[0].setValue(HookIndex::OpenFileEnd);
        params << ErrFile::Success;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }

      hexeditor->setLockedFile(true);
      setEditModeEnabled(true, true);
      if (_enableplugin) {
        params[0].setValue(HookIndex::OpenDriverEnd);
        params << ErrFile::Success;
        plgsys->raiseDispatch(HookIndex::OpenDriverEnd, params);
      }
      return ErrFile::Success;
    }

    if (_enableplugin) {
      params[0].setValue(HookIndex::OpenFileEnd);
      params << ErrFile::NotExist;
      plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
    }
    return ErrFile::NotExist;
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

bool MainWindow::isSavedFile(int index) {
  if (index < 0 || index >= hexfiles.count())
    return false;
  auto p = hexfiles.at(index);
  return p.doc->isDocSaved();
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
      if (!isSavedFile(index)) {
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
  auto filename =
      QFileDialog::getOpenFileName(this, tr("ChooseFile"), lastusedpath);
  if (!filename.isEmpty()) {
    lastusedpath = QFileInfo(filename).absoluteDir().absolutePath();
    int index;
    auto res = openFile(filename, false, &index);
    if (res == ErrFile::NotExist) {
      QMessageBox::critical(this, tr("Error"), tr("FileNotExist"));
      return;
    }
    if (res == ErrFile::AlreadyOpened) {
      setFilePage(index);
      return;
    }
    if (res == ErrFile::Permission &&
        openFile(filename, true) == ErrFile::Permission) {
      QMessageBox::critical(this, tr("Error"), tr("FilePermission"));
      return;
    }
    recentmanager->addRecentFile(filename);
  }
}

void MainWindow::on_tabCloseRequested(int index) {
  auto res = closeFile(index);
  if (res != ErrFile::Success) {
    auto f = hexfiles.at(index).filename;

    auto r = QMessageBox::question(this, tr("Close"),
                                   tr("ConfirmSave") + "\n" + f.remove(':'));
    if (r == QMessageBox::Yes) {
      closeFile(index, true);
    }
  }
}

void MainWindow::on_tabAddRequested() { newFile(); }

void MainWindow::on_undofile() {
  CheckEnabled;
  hexeditor->document()->undo();
}

void MainWindow::on_redofile() {
  CheckEnabled;
  hexeditor->document()->redo();
}

void MainWindow::on_cutfile() {
  CheckEnabled;
  if (hexeditor->document()->Cut()) {
    DMessageManager::instance()->sendMessage(this, ICONRES("cut"),
                                             tr("CutToClipBoard"));
  } else {
    DMessageManager::instance()->sendMessage(this, ICONRES("cut"),
                                             tr("UnCutToClipBoard"));
  }
}

void MainWindow::on_cuthex() {
  CheckEnabled;
  if (hexeditor->document()->Cut(true)) {
    DMessageManager::instance()->sendMessage(this, ICONRES("cut"),
                                             tr("CutToClipBoard"));
  } else {
    DMessageManager::instance()->sendMessage(this, ICONRES("cut"),
                                             tr("UnCutToClipBoard"));
  }
}

void MainWindow::on_copyfile() {
  CheckEnabled;
  hexeditor->document()->copy();
  DMessageManager::instance()->sendMessage(this, ICONRES("copy"),
                                           tr("CopyToClipBoard"));
}

void MainWindow::on_copyhex() {
  CheckEnabled;
  hexeditor->document()->copy(true);
  DMessageManager::instance()->sendMessage(this, ICONRES("copyhex"),
                                           tr("CopyToClipBoard"));
}

void MainWindow::on_pastefile() {
  CheckEnabled;
  hexeditor->document()->Paste();
}

void MainWindow::on_pastehex() {
  CheckEnabled;
  hexeditor->document()->Paste(true);
}

void MainWindow::on_opendriver() {
  DriverSelectorDialog ds;
  if (ds.exec()) {
    if (openDriver(ds.GetResult().device()) != ErrFile::Success)
      DMessageManager::instance()->sendMessage(this, ICONRES("opendriver"),
                                               tr("DriverOpenErrorTip"));
  }
}

void MainWindow::on_exportfile() {
  CheckEnabled;
  auto filename =
      QFileDialog::getSaveFileName(this, tr("ChooseExportFile"), lastusedpath);
  if (filename.isEmpty())
    return;
  lastusedpath = QFileInfo(filename).absoluteDir().absolutePath();
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
                                     tr("ConfirmSave") + "\n" + f.remove(':'));
      if (r == QMessageBox::Yes) {
        closeFile(0, true);
        tabs->removeTab(0);
      } else {
        on_save();
        event->ignore();
        return;
      }
    }
  }
  m_settings->saveWindowState(this);
  m_settings->saveFileDialogCurrent(lastusedpath);
  event->accept();
}

void MainWindow::on_save() {
  CheckEnabled;
  auto res = saveCurrent();
  if (res == ErrFile::IsNewFile)
    on_saveas();
  else if (res == ErrFile::Success) {
    DMessageManager::instance()->sendMessage(this, ICONRES("save"),
                                             tr("SaveSuccessfully"));
  } else if (res == ErrFile::WorkSpaceUnSaved) {
    DMessageManager::instance()->sendMessage(this, ICONRES("save"),
                                             tr("SaveWSError"));
  } else {
    DMessageManager::instance()->sendMessage(this, ICONRES("save"),
                                             tr("SaveUnSuccessfully"));
  }
}

void MainWindow::on_delete() {
  CheckEnabled;
  hexeditor->document()->RemoveSelection();
}

void MainWindow::on_saveas() {
  CheckEnabled;
  auto filename =
      QFileDialog::getSaveFileName(this, tr("ChooseSaveFile"), lastusedpath);
  if (filename.isEmpty())
    return;
  lastusedpath = QFileInfo(filename).absoluteDir().absolutePath();
  auto res = saveAs(filename, _currentfile);
  if (res == ErrFile::Success) {
    DMessageManager::instance()->sendMessage(this, ICONRES("saveas"),
                                             tr("SaveSuccessfully"));
  } else if (res == ErrFile::WorkSpaceUnSaved) {
    DMessageManager::instance()->sendMessage(this, ICONRES("saveas"),
                                             tr("SaveWSError"));
  } else {
    DMessageManager::instance()->sendMessage(this, ICONRES("saveas"),
                                             tr("SaveUnSuccessfully"));
  }
}

void MainWindow::on_findfile() {
  CheckEnabled;
  FindDialog *fd = new FindDialog(hexeditor->selectlength() > 1, this);
  if (fd->exec()) {
    auto m = this;
    auto th = QThread::create([=] {
      if (mutex.tryLock(3000)) {
        SearchDirection sd;
        auto res = fd->getResult(sd);
        auto d = hexeditor->document();
        QList<quint64> results;
        if (d == nullptr)
          return;
        qint64 begin, end;
        switch (sd) {
        case SearchDirection::Foreword: {
          begin = 0;
          end = qlonglong(hexeditor->currentOffset());
        } break;
        case SearchDirection::Backword: {
          begin = qlonglong(hexeditor->currentOffset());
          end = -1;
        } break;
        case SearchDirection::Selection: {
          auto cur = hexeditor->document()->cursor();
          begin = qlonglong(cur->selectionStart().offset());
          end = qlonglong(cur->selectionEnd().offset());
        } break;
        default: {
          begin = -1;
          end = -1;
        } break;
        }
        d->findAllBytes(begin, end, res, results, _findmax);
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

        if (len == _findmax) {
          findres = 1;
        } else {
          findres = -1;
        }
      } else {
        findres = 0;
      }
    });
    connect(th, &QThread::finished, this, [=] {
      if (findres > 0) {
        DMessageManager::instance()->sendMessage(m, ICONRES("find"),
                                                 tr("TooMuchFindResult"));
      } else if (findres < 0) {
        DMessageManager::instance()->sendMessage(m, ICONRES("find"),
                                                 tr("FindFininish"));
      } else {
        DMessageManager::instance()->sendMessage(m, ICONRES("find"),
                                                 tr("FindFininishError"));
      }
      delete fd;
      delete th;
      mutex.unlock();
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
    numsitem[NumTableIndex::Uint64].setText(
        QString("0x%1").arg(QString::number(s, 16)));
    auto s1 = qint64(n);
    numsitem[NumTableIndex::Int64].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Uint64].setText("-");
    numsitem[NumTableIndex::Int64].setText("-");
  }

  if (len > int(sizeof(quint32))) {
    auto s = ulong(n);
    numsitem[NumTableIndex::Uint32].setText(
        QString("0x%1").arg(QString::number(s, 16)));
    auto s1 = long(n);
    numsitem[NumTableIndex::Int32].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Uint32].setText("-");
    numsitem[NumTableIndex::Int32].setText("-");
  }

  if (len > int(sizeof(ushort))) {
    auto s = ushort(n);
    numsitem[NumTableIndex::Ushort].setText(
        QString("0x%1").arg(QString::number(s, 16)));
    auto s1 = short(n);
    numsitem[NumTableIndex::Short].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Ushort].setText("-");
    numsitem[NumTableIndex::Short].setText("-");
  }
  if (len > int(sizeof(uchar))) {
    auto s1 = tmp.at(0);
    auto s = uchar(s1);
    numsitem[NumTableIndex::Byte].setText(
        QString("0x%1").arg(QString::number(s, 16)));
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

void MainWindow::on_savesel() {
  CheckEnabled;
  auto filename =
      QFileDialog::getSaveFileName(this, tr("ChooseSaveFile"), lastusedpath);
  if (filename.isEmpty())
    return;
  lastusedpath = QFileInfo(filename).absoluteDir().absolutePath();
  QFile qfile(filename);
  if (qfile.open(QFile::WriteOnly)) {
    auto buffer = hexeditor->document()->selectedBytes();
    qfile.write(buffer);
    qfile.close();
  } else {
    DMessageManager::instance()->sendMessage(this, ICONRES("savesel"),
                                             tr("SaveSelError"));
  }
}

void MainWindow::on_bookmarkChanged() {
  auto doc = hexeditor->document();
  QList<BookMarkStruct> bookmaps;
  bookmarks->clear();
  doc->getBookMarks(bookmaps);
  for (auto item : bookmaps) {
    QListWidgetItem *litem = new QListWidgetItem;
    litem->setIcon(ICONRES("bookmark"));
    litem->setText(item.comment);
    litem->setToolTip(QString(tr("Addr : 0x%1")).arg(item.pos, 0, 16));
    bookmarks->addItem(litem);
  }
}

void MainWindow::on_documentSwitched() {
  iReadWrite->setPixmap(hexeditor->isReadOnly() ? infoReadonly : infoWriteable);
  if (hexfiles.count()) {
    iw->setPixmap(hexeditor->document()->isWorkspace ? infow : infouw);
  } else {
    iw->setPixmap(infouw);
  }
}

ErrFile MainWindow::save(int index) {
  if (index >= 0 && index < hexfiles.count()) {
    auto f = hexfiles.at(index);
    if (f.isdriver)
      return ErrFile::IsDirver;
    if (f.filename.at(0) == ':')
      return ErrFile::IsNewFile;
    QFile file(f.filename);
    file.open(QFile::WriteOnly);
    if (f.doc->saveTo(&file, true)) {
      file.close();
      if (f.doc->metadata()->hasMetadata()) {
        auto w = f.workspace;
        if (QFile::exists(w)) {
          auto b = WorkSpaceManager::saveWorkSpace(
              w, f.filename, hexeditor->document()->getAllBookMarks(),
              hexeditor->document()->metadata()->getallMetas());
          if (!b)
            return ErrFile::WorkSpaceUnSaved;
          f.doc->isWorkspace = true;
          iw->setPixmap(infow);
          tabs->setTabIcon(index, ICONRES("pro"));
          f.doc->setDocSaved();
        } else {
          auto b = WorkSpaceManager::saveWorkSpace(
              f.filename + PROEXT, f.filename,
              hexeditor->document()->getAllBookMarks(),
              hexeditor->document()->metadata()->getallMetas());
          if (!b)
            return ErrFile::WorkSpaceUnSaved;
          hexfiles[index].workspace = f.filename + PROEXT;
          f.doc->isWorkspace = true;
          iw->setPixmap(infow);
          tabs->setTabIcon(index, ICONRES("pro"));
          f.doc->setDocSaved();
        }
      }
      return ErrFile::Success;
    }
    return ErrFile::Permission;
  }
  return ErrFile::Error;
}

ErrFile MainWindow::exportFile(QString filename, int index) {
  if (index >= 0 && index < hexfiles.count()) {
    auto f = hexfiles.at(index);
    if (f.isdriver)
      return ErrFile::IsDirver;
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

ErrFile MainWindow::saveAs(QString filename, int index) {
  if (index >= 0 && index < hexfiles.count()) {
    auto f = hexfiles.at(index);
    if (f.isdriver)
      return ErrFile::IsDirver;
    QFile file(filename);
    file.open(QFile::WriteOnly);
    if (f.doc->saveTo(&file, true)) {
      hexfiles[index].filename = filename;
      file.close();
      if (f.doc->metadata()->hasMetadata()) {
        auto b = WorkSpaceManager::saveWorkSpace(
            filename + PROEXT, filename,
            hexeditor->document()->getAllBookMarks(),
            hexeditor->document()->metadata()->getallMetas());
        if (!b)
          return ErrFile::WorkSpaceUnSaved;
        hexfiles[index].workspace = filename + PROEXT;
        f.doc->isWorkspace = true;
        iw->setPixmap(infow);
        tabs->setTabIcon(index, ICONRES("pro"));
        f.doc->setDocSaved();
      }
      return ErrFile::Success;
    }
    file.close();
    return ErrFile::Permission;
  }
  return ErrFile::Error;
}

ErrFile MainWindow::saveCurrent() { return save(_currentfile); }

void MainWindow::on_metadataedit() {
  CheckEnabled;
  if (hexeditor->documentBytes() > 0) {
    MetaDialog m;
    auto cur = hexeditor->document()->cursor();
    if (cur->selectionLength() > 0) {
      auto mc =
          hexeditor->document()->metadata()->gets(cur->position().offset());

      if (mc.length() > 0) {
        auto meta = mc.last();
        auto begin = meta.begin;
        auto end = meta.end;
        m.setForeGroundColor(meta.foreground);
        m.setBackGroundColor(meta.background);
        m.setComment(meta.comment);
        if (m.exec()) {
          auto mi = hexeditor->document()->metadata();
          QHexMetadataAbsoluteItem o;
          o.begin = begin;
          o.end = end;
          o.foreground = m.foreGroundColor();
          o.background = m.backGroundColor();
          o.comment = m.comment();
          mi->ModifyMetadata(meta, o);
        }
      } else {
        DMessageManager::instance()->sendMessage(this, ICONRES("metadata"),
                                                 tr("NoMetaData"));
      }
    } else {
      DMessageManager::instance()->sendMessage(this, ICONRES("metadata"),
                                               tr("NoSelection"));
    }
  }
}

void MainWindow::on_metadata() {
  CheckEnabled;
  if (hexeditor->documentBytes() > 0) {
    MetaDialog m;
    auto cur = hexeditor->document()->cursor();

    if (cur->selectionLength() > 0) {
      auto begin = qint64(cur->selectionStart().offset());
      auto end = qint64(cur->selectionEnd().offset()) + 1;
      if (m.exec()) {
        hexeditor->document()->metadata()->Metadata(
            begin, end, m.foreGroundColor(), m.backGroundColor(), m.comment());
      }

    } else {
      DMessageManager::instance()->sendMessage(this, ICONRES("metadata"),
                                               tr("NoSelection"));
    }
  }
}

void MainWindow::on_metadatadel() {
  CheckEnabled;
  auto doc = hexeditor->document();
  auto meta = doc->metadata();
  auto pos = doc->cursor()->position().offset();
  meta->RemoveMetadata(pos);
}

void MainWindow::on_metadatacls() {
  CheckEnabled;
  hexeditor->document()->metadata()->Clear();
}

void MainWindow::on_setting_plugin() {
  if (!_enableplugin)
    return;
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
    bool ok;
    auto comment =
        DInputDialog::getText(this, tr("BookMark"), tr("InputComment"),
                              QLineEdit::Normal, b.comment, &ok);
    if (ok) {
      doc->ModBookMark(b.pos, comment);
    }
  } else {
    bool ok;
    auto comment =
        DInputDialog ::getText(this, tr("BookMark"), tr("InputComment"),
                               QLineEdit::Normal, QString(), &ok);
    if (ok) {
      auto pos = qint64(hexeditor->currentOffset());
      doc->AddBookMark(pos, comment);
    }
  }
}

void MainWindow::on_bookmarkdel() {
  CheckEnabled;
  auto doc = hexeditor->document();
  int index = -1;
  if (doc->existBookMark(index)) {
    doc->RemoveBookMark(index);
  }
}

void MainWindow::on_bookmarkcls() {
  CheckEnabled;
  hexeditor->document()->ClearBookMark();
}

void MainWindow::on_encoding() {
  CheckEnabled;
  EncodingDialog d;
  if (d.exec()) {
    auto res = d.getResult();
    hexeditor->renderer()->setEncoding(res);
  }
}

void MainWindow::setEditModeEnabled(bool b, bool isdriver) {
  for (auto item : toolbartools.values()) {
    item->setEnabled(b);
  }
  hexeditorMenu->setEnabled(b);
  for (auto item : toolmenutools.values()) {
    item->setEnabled(b);
  }
  for (auto item : toolbtnstools.values()) {
    item->setEnabled(b);
  }

  if (b) {
    enableDirverLimit(isdriver);
    auto dm = hexeditor->document()->isWorkspace;
    iw->setPixmap(dm ? infow : infouw);
  } else {
    iw->setPixmap(infouw);
  }
  status->setEnabled(b);

  auto doc = hexeditor->document();
  doc->canRedoChanged(doc->canRedo());
  doc->canUndoChanged(doc->canUndo());

  if (!b) {
    iSaved->setPixmap(infoSaveg);
    iReadWrite->setPixmap(inforwg);
    iLocked->setIcon(infoLockg);
    iw->setPixmap(infowg);
    iOver->setIcon(infoOverg);
    lblloc->setText("(0,0)");
    lblsellen->setText("0 - 0x0");
    for (auto i = 0; i < NumTableIndexCount; i++) {
      numsitem[i].setText("-");
    }
    bookmarks->clear();
  }
}

void MainWindow::enableDirverLimit(bool b) {
  auto e = !b;
  toolbartools[ToolBoxIndex::SaveAs]->setEnabled(e);
  toolbartools[ToolBoxIndex::Export]->setEnabled(e);
  toolmenutools[ToolBoxIndex::SaveAs]->setEnabled(e);
  toolmenutools[ToolBoxIndex::Export]->setEnabled(e);
}

void MainWindow::on_restoreLayout() { m_settings->loadWindowState(this, true); }

void MainWindow::on_fill() {
  CheckEnabled;
  bool b;
  auto in = DInputDialog::getText(this, tr("Fill"), tr("PleaseInputFill"),
                                  QLineEdit::Normal, QString(), &b);
  if (b) {
    auto ch = char(in.toULongLong(&b, 0));
    if (b) {
      auto doc = hexeditor->document();
      if (doc->isEmpty() || hexeditor->selectlength() == 0)
        return;
      auto pos = doc->cursor()->selectionStart().offset();
      doc->Replace(pos, QByteArray(int(hexeditor->selectlength()), char(ch)));
    } else {
      DMessageManager::instance()->sendMessage(this, ICONRES("fill"),
                                               tr("FillInputError"));
    }
  }
}

void MainWindow::on_fillnop() {
  CheckEnabled;
  auto doc = hexeditor->document();
  if (doc->isEmpty() || hexeditor->selectlength() == 0)
    return;
  auto pos = doc->cursor()->selectionStart().offset();
  doc->Replace(pos, QByteArray(int(hexeditor->selectlength()), char(0x90)));
}

void MainWindow::on_fillzero() {
  CheckEnabled;
  auto doc = hexeditor->document();
  if (doc->isEmpty() || hexeditor->selectlength() == 0)
    return;
  auto pos = doc->cursor()->selectionStart().offset();
  doc->Replace(pos, QByteArray(int(hexeditor->selectlength()), char(0)));
}

void MainWindow::on_loadplg() {
  if (!_enableplugin)
    return;
  auto filename = QFileDialog::getOpenFileName(
      this, tr("ChoosePlugin"), lastusedpath, tr("PluginFile (*.wingplg)"));
  if (!filename.isEmpty()) {
    auto info = QFileInfo(filename);
    lastusedpath = info.absoluteDir().absolutePath();
    plgsys->loadPlugin(info);
  }
}

void MainWindow::on_clearfindresult() {
  delete[] findresitem;
  findresult->setRowCount(0);
  findresitem = nullptr;
}

void MainWindow::on_exportfindresult() {
  auto c = findresult->rowCount();
  if (c == 0) {
    DMessageManager::instance()->sendMessage(this, ICONRES("export"),
                                             tr("EmptyFindResult"));
    return;
  }
  auto filename =
      QFileDialog::getSaveFileName(this, tr("ChooseSaveFile"), lastusedpath);
  if (filename.isEmpty())
    return;
  lastusedpath = QFileInfo(filename).absoluteDir().absolutePath();
  QFile f(filename);
  if (f.open(QFile::WriteOnly)) {
    QJsonArray arr;
    for (int i = 0; i < c; i++) {
      QJsonObject jobj;
      jobj.insert(QLatin1String("file"), findresitem[i][0].text());
      jobj.insert(QLatin1String("offset"), findresitem[i][1].text());
      jobj.insert(QLatin1String("value"), findresitem[i][2].text());
      arr.append(jobj);
    }
    QJsonDocument doc(arr);
    if (f.write(doc.toJson(QJsonDocument::JsonFormat::Indented)) >= 0) {
      f.close();
      DMessageManager::instance()->sendMessage(this, ICONRES("export"),
                                               tr("SaveFindResult"));
    }
  } else {
    DMessageManager::instance()->sendMessage(this, ICONRES("export"),
                                             tr("SaveFindResultError"));
  }
}

void MainWindow::on_wiki() {
  QDesktopServices::openUrl(QUrl("https://code.gitlink.org.cn/wingsummer/"
                                 "WingHexExplorer/wiki/%E7%AE%80%E4%BB%8B"));
}

void MainWindow::on_sponsor() {
  SponsorDialog d;
  d.exec();
}

void MainWindow::on_about() {
  AboutSoftwareDialog d;
  d.exec();
}

ErrFile MainWindow::openWorkSpace(QString filename, bool readonly,
                                  int *openedindex) {
  QString file;
  QList<BookMarkStruct> bookmarks;
  QList<QHexMetadataAbsoluteItem> metas;
  auto res = ErrFile::Error;
  if (WorkSpaceManager::loadWorkSpace(filename, file, bookmarks, metas)) {
    bool b;
    int index;
    res = openFile(file, readonly, &index, filename, &b);
    if (res == ErrFile::AlreadyOpened) {
      if (openedindex)
        *openedindex = index;
      return res;
    } else {
      if (res != ErrFile::Success)
        return res;
    }

    auto doc = hexeditor->document();
    doc->applyBookMarks(bookmarks);
    on_documentSwitched();
    doc->metadata()->applyMetas(metas);
  }
  return res;
}

void MainWindow::on_openworkspace() {
  auto filename = QFileDialog::getOpenFileName(
      this, tr("ChooseFile"), lastusedpath, tr("ProjectFile (*.wingpro)"));
  if (filename.isEmpty())
    return;
  lastusedpath = QFileInfo(filename).absoluteDir().absolutePath();
  int index;
  auto res = openWorkSpace(filename, false, &index);
  if (res == ErrFile::AlreadyOpened) {
    if (hexfiles[index].workspace.length() == 0)
      DMessageManager::instance()->sendMessage(this, ICONRES("workspace"),
                                               tr("WSOpenedUnSuccessfully"));
  } else if (res != ErrFile::Success) {
    DMessageManager::instance()->sendMessage(this, ICONRES("workspace"),
                                             tr("WorkSpaceOpenUnSuccessfully"));
  }
  recentmanager->addRecentFile(filename);
}
