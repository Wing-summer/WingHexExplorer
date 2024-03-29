#include "mainwindow.h"
#include "QHexView/document/buffer/qmemorybuffer.h"
#include "QHexView/document/qhexcursor.h"
#include "QHexView/document/qhexmetadata.h"
#include "aboutsoftwaredialog.h"
#include "class/appmanager.h"
#include "class/recentfilemanager.h"
#include "dialog/openregiondialog.h"
#include "driverselectordialog.h"
#include "encodingdialog.h"
#include "fileinfodialog.h"
#include "finddialog.h"
#include "metadialog.h"
#include "plugin/iwingplugin.h"
#include "pluginwindow.h"
#include "settings.h"
#include "sponsordialog.h"
#include <DAnchors>
#include <DMenuBar>
#include <DMessageManager>
#include <DSettingsDialog>
#include <DSettingsWidgetFactory>
#include <DTitlebar>
#include <DWidgetUtil>
#include <QCheckBox>
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QHeaderView>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QScrollBar>
#include <QShortcut>
#include <QStorageInfo>
#include <QThread>

#ifndef WITHOUTLICENSEINFO
#include "mlicense/lincensedialog.h"
#endif

#define FILEMAXBUFFER 0x6400000 // 100MB

#define CheckEnabled                                                           \
  if (hexfiles.count() == 0)                                                   \
    return;

#define SaveWorkSpaceInitInfo                                                  \
  WorkSpaceInfo infos;                                                         \
  infos.base = doc->baseAddress();                                             \
  infos.locked = doc->isLocked();                                              \
  infos.keepsize = doc->isKeepSize();                                          \
  infos.showstr = render->stringVisible();                                     \
  infos.showaddr = render->addressVisible();                                   \
  infos.showheader = render->headerVisible();                                  \
  infos.encoding = render->encoding();                                         \
  infos.showmetabg = doc->metabgVisible();                                     \
  infos.showmetafg = doc->metafgVisible();                                     \
  infos.showmetacomment = doc->metaCommentVisible();

MainWindow::MainWindow(DMainWindow *parent) {
  Q_UNUSED(parent)

  // init mainwindow
  setMinimumSize(QSize(1200, 800));
  setAcceptDrops(true);

  auto _title = titlebar();
  auto picon = Utilities::isRoot() ? ICONRES("iconroot") : ICONRES("icon");
  setWindowIcon(picon);
  _title->setIcon(picon);
  _title->setTitle("WingHexExplorer");

  tabs = new DTabBar(this);
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
  connect(tabs, &DTabBar::tabBarDoubleClicked, this,
          &MainWindow::on_tabBarDoubleClicked);
  connect(tabs, &DTabBar::tabMoved, this, &MainWindow::on_tabMoved);

  iconmetah = ICONRES("metadatah");
  iconmetas = ICONRES("metadata");

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

  auto keynewb = QKeySequence(Qt::KeyboardModifier::ControlModifier |
                              Qt::KeyboardModifier::ShiftModifier | Qt::Key_N);
  auto keyopenr = QKeySequence(Qt::KeyboardModifier::ControlModifier |
                               Qt::KeyboardModifier::ShiftModifier | Qt::Key_O);

  AddToolSubMenuShortcutAction("newb", tr("NewBigFile"),
                               MainWindow::on_newbigfile, keynewb);
  AddToolSubMenuShortcutAction("open", tr("OpenF"), MainWindow::on_openfile,
                               QKeySequence::Open);
  AddToolSubMenuShortcutAction("openr", tr("OpenFR"), MainWindow::on_openregion,
                               keyopenr);

  auto keysaveas =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_S);
  auto keygoto =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_G);
  auto keyGeneral =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_G);
  auto keyplugin =
      QKeySequence(Qt::KeyboardModifier::ControlModifier |
                   Qt::KeyboardModifier::ShiftModifier | Qt::Key_P);
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
  auto keymetafg =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_1);
  auto keymetabg =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_2);
  auto keymetacom =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_3);
  auto keymetashow =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_Plus);
  auto keymetahide =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_Minus);
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
  auto keyfinfo =
      QKeySequence(Qt::KeyboardModifier::ControlModifier | Qt::Key_I);
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
  AddToolSubMenuAction("opendriver", tr("OpenD"), MainWindow::on_opendriver);
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
  AddToolSubMenuAction("reload", tr("Reload"), MainWindow::on_reload);
  AddMenuDB(ToolBoxIndex::Reload);
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
  AddToolSubMenuShortcutAction("info", tr("FileInfo"), MainWindow::on_fileInfo,
                               keyfinfo);
  AddMenuDB(ToolBoxIndex::FileInfo);
  menu->addMenu(tm);

  tm = new DMenu(this);
  tm->setTitle(tr("Mark"));
  tm->setIcon(ICONRES("mark"));
  AddToolSubMenuShortcutAction("bookmark", tr("BookMark"),
                               MainWindow::on_bookmark, keybookmark);
  AddMenuDB(ToolBoxIndex::BookMark);
  AddToolSubMenuShortcutAction("bookmarkdel", tr("DeleteBookMark"),
                               MainWindow::on_bookmarkdel, keybookmarkdel);
  AddMenuDB(ToolBoxIndex::DelBookMark);
  AddToolSubMenuShortcutAction("bookmarkcls", tr("ClearBookMark"),
                               MainWindow::on_bookmarkcls, keybookmarkcls);
  AddMenuDB(ToolBoxIndex::ClsBookMark);
  tm->addSeparator();
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

#define AddCheckMenu(Title, ShorCut, Slot, StatusSlot)                         \
  a = new QAction(Title, tm);                                                  \
  a->setShortcut(ShorCut);                                                     \
  a->setShortcutVisibleInContextMenu(true);                                    \
  a->setCheckable(true);                                                       \
  connect(a, &QAction::triggered, this, &Slot);                                \
  connect(hexeditor, &StatusSlot, a, &QAction::setChecked);                    \
  tm->addAction(a);

  AddCheckMenu(tr("ShowMetafg"), keymetafg, MainWindow::on_metadatafg,
               QHexView::metafgVisibleChanged);
  AddMenuDB(ToolBoxIndex::Metafg);
  AddCheckMenu(tr("ShowMetabg"), keymetabg, MainWindow::on_metadatabg,
               QHexView::metabgVisibleChanged);
  AddMenuDB(ToolBoxIndex::Metabg);
  AddCheckMenu(tr("ShowMetaComment"), keymetacom,
               MainWindow::on_metadatacomment,
               QHexView::metaCommentVisibleChanged);
  AddMenuDB(ToolBoxIndex::MetaComment);
  tm->addSeparator();
  AddToolSubMenuShortcutAction("metashow", tr("MetaShowAll"),
                               MainWindow::on_metashowall, keymetashow);
  AddMenuDB(ToolBoxIndex::MetaShow);
  AddToolSubMenuShortcutAction("metahide", tr("MetaHideAll"),
                               MainWindow::on_metahideall, keymetahide);
  AddMenuDB(ToolBoxIndex::MetaHide);
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
  tm->addSeparator();
  AddToolSubMenuAction("log", tr("ExportLog"), MainWindow::on_exportlog);
  AddToolSubMenuAction("clearhis", tr("ClearLog"), MainWindow::on_clslog);
  tm->addSeparator();
  AddToolSubMenuAction("fullscreen", tr("Fullscreen"),
                       MainWindow::on_fullScreen);
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

#ifndef WITHOUTLICENSEINFO
  AddToolSubMenuAction("license", tr("License"), MainWindow::on_license);
#endif

  AddToolSubMenuAction("wiki", tr("Wiki"), MainWindow::on_wiki);
  a = new QAction(ICONRES("qt"), tr("AboutQT"), tm);
  connect(a, &QAction::triggered, this, [=] { QMessageBox::aboutQt(this); });
  tm->addAction(a);
  menu->addMenu(tm);

  _title->setMenu(menu);

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
  auto tva = toolbar->toggleViewAction();
  tva->setVisible(false);

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

  DToolButton *tbtn;
  DMenu *tmenu;

#define AddToolBtnBegin(DIcon)                                                 \
  tbtn = new DToolButton(this);                                                \
  tbtn->setEnabled(false);                                                     \
  tbtn->setIcon(ICONRES(DIcon));                                               \
  tmenu = new DMenu(this);

#define AddToolBtnBegin2(DIcon)                                                \
  tbtn = new DToolButton(this);                                                \
  tbtn->setIcon(ICONRES(DIcon));                                               \
  tmenu = new DMenu(this);

#define AddToolBtnBtn(Icon, Title, Slot)                                       \
  a = new QAction(ICONRES(Icon), Title, this);                                 \
  connect(a, &QAction::triggered, this, &Slot);                                \
  tmenu->addAction(a);

#define AddToolBtnEnd2()                                                       \
  tbtn->setMenu(tmenu);                                                        \
  tbtn->setPopupMode(DToolButton::ToolButtonPopupMode::InstantPopup);          \
  toolbar->addWidget(tbtn);

#define AddToolBtnEnd(Index)                                                   \
  tbtn->setMenu(tmenu);                                                        \
  tbtn->setPopupMode(DToolButton::ToolButtonPopupMode::InstantPopup);          \
  toolbar->addWidget(tbtn);                                                    \
  toolbtnstools.insert(Index, tbtn);

  AddToolBtnBegin2("new") {
    AddToolBtnBtn("new", tr("New"), MainWindow::on_newfile);
    AddToolBtnBtn("newb", tr("NewBigFile"), MainWindow::on_newbigfile);
  }
  AddToolBtnEnd2();

  AddToolBtnBegin2("open") {
    AddToolBtnBtn("open", tr("OpenF"), MainWindow::on_openfile);
    AddToolBtnBtn("openr", tr("OpenFR"), MainWindow::on_openregion);
  }
  AddToolBtnEnd2();

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
  AddToolBarTool("reload", MainWindow::on_reload, tr("Reload"));
  AddToolsDB(ToolBoxIndex::Reload);
  toolbar->addSeparator();
  AddToolBarTool("undo", MainWindow::on_undofile, tr("Undo"));
  AddToolsDB(ToolBoxIndex::Undo);
  AddToolBarTool("redo", MainWindow::on_redofile, tr("Redo"));
  AddToolsDB(ToolBoxIndex::Redo);

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

#define AddToolCheckBtn(Title, Slot, StatusSlot)                               \
  a = new QAction(Title, this);                                                \
  a->setCheckable(true);                                                       \
  a->setChecked(true);                                                         \
  connect(a, &QAction::triggered, this, &Slot);                                \
  connect(hexeditor, &StatusSlot, a, &QAction::setChecked);                    \
  tmenu->addAction(a);

  AddToolBtnBegin("metadata") {
    AddToolBtnBtn("metadata", tr("MetaData"), MainWindow::on_metadata);
    AddToolBtnBtn("metadataedit", tr("MetaDataEdit"),
                  MainWindow::on_metadataedit);
    AddToolBtnBtn("metadatadel", tr("DeleteMetaData"),
                  MainWindow::on_metadatadel);
    AddToolBtnBtn("metadatacls", tr("ClearMetaData"),
                  MainWindow::on_metadatacls);
    tmenu->addSeparator();

    AddToolCheckBtn(tr("ShowMetafg"), MainWindow::on_metadatafg,
                    QHexView::metafgVisibleChanged);

    AddToolCheckBtn(tr("ShowMetabg"), MainWindow::on_metadatabg,
                    QHexView::metabgVisibleChanged);
    AddToolCheckBtn(tr("ShowMetaComment"), MainWindow::on_metadatacomment,
                    QHexView::metaCommentVisibleChanged);

    tmenu->addSeparator();

    AddToolBtnBtn("metashow", tr("MetaShowAll"), MainWindow::on_metashowall);
    AddToolBtnBtn("metahide", tr("MetaHideAll"), MainWindow::on_metahideall);
  }
  AddToolBtnEnd(ToolBoxIndex::Meta);

  connect(hexeditor, &QHexView::metaStatusChanged, this,
          &MainWindow::on_metastatusChanged);

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

#ifndef WITHOUTLICENSEINFO
  AddToolBarTool("license", MainWindow::on_license, tr("License"));
#endif

  AddToolBarTool("soft", MainWindow::on_about, tr("About"));
  toolbar->addSeparator();
  this->addToolBar(toolbar);

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
  connect(iSetBaseAddr, &DIconButton::clicked, this, [=] {
    QInputDialog d;
    bool b;
    auto num = d.getText(this, tr("addressBase"), tr("inputAddressBase"),
                         QLineEdit::Normal, QString(), &b);
    if (b) {
      qulonglong qnum = num.toULongLong(&b, 0);
      if (b) {
        auto r = qnum + hexeditor->documentBytes();
        if (qnum > r || hexeditor->documentBytes() > r) {
          DMessageManager::instance()->sendMessage(this, ICONRES("mAddr"),
                                                   tr("WarnBigBaseAddress"));
        }
        hexeditor->setAddressBase(qnum);
      } else {
        DMessageManager::instance()->sendMessage(this, ICONRES("mAddr"),
                                                 tr("ErrBaseAddress"));
      }
    }
  });

  AddFunctionIconButton(iColInfo, "mColInfo");
  iColInfo->setToolTip(tr("SetColInfo"));
  connect(iColInfo, &DIconButton::clicked, this,
          [=] { hexeditor->setAddressVisible(!hexeditor->addressVisible()); });
  AddFunctionIconButton(iHeaderInfo, "mLineInfo");
  iHeaderInfo->setToolTip(tr("SetHeaderInfo"));
  connect(iHeaderInfo, &DIconButton::clicked, this,
          [=] { hexeditor->setHeaderVisible(!hexeditor->headerVisible()); });
  AddFunctionIconButton(iAsciiString, "mStr");
  iAsciiString->setToolTip(tr("SetAsciiString"));
  connect(iAsciiString, &DIconButton::clicked, this,
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

  connect(iLocked, &DIconButton::clicked, this, [=]() {
    CheckEnabled;
    if (!hexeditor->setLockedFile(!hexeditor->isLocked())) {
      auto d = DMessageManager::instance();
      d->sendMessage(this, infoLock, tr("ErrUnLock"));
    }
  });

  connect(iOver, &DIconButton::clicked, this, [=]() {
    CheckEnabled;
    if (!hexeditor->setKeepSize(!hexeditor->isKeepSize())) {
      DMessageManager::instance()->sendMessage(this, infoCannotOver,
                                               tr("ErrUnOver"));
    } else {
      if (hexeditor->document()->metadata()->hasMetadata()) {
        DMessageManager::instance()->sendMessage(this, infoCanOver,
                                                 tr("InfoCanOverLimit"));
      }
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
  connect(findresult, &QTableWidget::customContextMenuRequested, this,
          [=]() { findresultMenu->popup(cursor().pos()); });
  connect(findresult, &QTableWidget::itemDoubleClicked, this, [=] {
    auto item = findresult->item(findresult->currentRow(), 0);
    auto filename = hexfiles.at(_currentfile).filename;
    if (filename != item->text()) {
      int i = 0;
      for (auto &item : hexfiles) {
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
  dw->setMinimumSize(450, 300);
  pluginInfo->setFocusPolicy(Qt::StrongFocus);
  pluginInfo->setOpenExternalLinks(true);
  pluginInfo->setUndoRedoEnabled(false);
  dw->setWidget(pluginInfo);
  this->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, dw);

  connect(Logger::getInstance(), &Logger::log, this, [=](QString msg) {
    QMutexLocker locker(&logmutex);
    Q_ASSERT(pluginInfo);
    auto cur = pluginInfo->textCursor();
    cur.movePosition(QTextCursor::End);
    pluginInfo->setTextCursor(cur);
    pluginInfo->insertHtml(msg);
    pluginInfo->append("");
  });

  Logger::info(tr("LoggerInitFinish"));

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

  numtableMenu = new DMenu(numshowtable);
  a = new QAction(numtableMenu);
  a->setText(tr("Copy"));
  connect(a, &QAction::triggered, this, [=] {
    auto r = numshowtable->currentRow();
    qApp->clipboard()->setText(numsitem[r].text());
    DMessageManager::instance()->sendMessage(this, ICONRES("copy"),
                                             tr("CopyToClipBoard"));
  });
  numtableMenu->addAction(a);
  numtableMenu->addSeparator();

  auto le = Utilities::checkIsLittleEndian();
  a = new QAction(numtableMenu);
  a->setText(tr("LittleEndian"));
  a->setCheckable(true);
  a->setChecked(le);
  connect(a, &QAction::triggered, this, [=] {
    islittle = true;
    this->on_locChanged();
    littleEndian->setChecked(true);
    bigEndian->setChecked(false);
  });
  littleEndian = a;
  numtableMenu->addAction(a);

  a = new QAction(numtableMenu);
  a->setText(tr("BigEndian"));
  a->setCheckable(true);
  a->setChecked(!le);
  connect(a, &QAction::triggered, this, [=] {
    islittle = false;
    this->on_locChanged();
    littleEndian->setChecked(false);
    bigEndian->setChecked(true);
  });
  bigEndian = a;
  numtableMenu->addAction(a);

  numshowtable->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(numshowtable, &DTableWidget::customContextMenuRequested, this,
          [=] { numtableMenu->popup(cursor().pos()); });

  numsitem = new QTableWidgetItem[NumTableIndexCount];
  for (int i = 0; i < NumTableIndexCount; i++) {
    auto item = numsitem + i;
    item->setText("-");
    item->setTextAlignment(Qt::AlignCenter);
    numshowtable->setItem(i, 0, item);
  }
  dw2->setObjectName("Number");
  dw2->setWindowTitle(tr("Number"));
  dw2->setMinimumSize(450, 300);
  dw2->setWidget(numshowtable);
  this->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, dw2);
  this->tabifyDockWidget(dw, dw2);

  dw = new DDockWidget(this);
  dw->setMinimumSize(450, 300);
  AddDockWin(tr("BookMark"));
  bookmarks = new DListWidget(this);

  menu = new DMenu(bookmarks);
  a = new QAction(ICONRES("bookmarkdel"), tr("DeleteBookMark"), menu);
  connect(a, &QAction::triggered, this, [=] {
    auto s = bookmarks->selectedItems();
    QList<qint64> pos;
    for (auto &item : s) {
      pos.push_back(item->data(Qt::UserRole).toLongLong());
    }
    hexeditor->document()->RemoveBookMarks(pos);
  });
  menu->addAction(a);
  a = new QAction(ICONRES("bookmarkcls"), tr("ClearBookMark"), menu);
  connect(a, &QAction::triggered, this, &MainWindow::on_bookmarkcls);
  menu->addAction(a);

  bookmarks->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  bookmarks->setSelectionMode(DListWidget::SelectionMode::ExtendedSelection);
  bookmarks->setFocusPolicy(Qt::StrongFocus);
  connect(bookmarks, &DListWidget::itemDoubleClicked, this, [=]() {
    hexeditor->renderer()->enableCursor(true);
    hexeditor->document()->gotoBookMark(bookmarks->currentRow());
  });
  connect(bookmarks, &DListWidget::customContextMenuRequested, this,
          [=] { menu->popup(cursor().pos()); });
  dw->setWidget(bookmarks);
  dw->setObjectName("BookMark");
  dw->setWindowTitle(tr("BookMark"));
  this->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, dw);
  this->tabifyDockWidget(dw2, dw);

  dw = new DDockWidget(this);
  AddDockWin(tr("DecodeText"));
  txtDecode = new QTextBrowser(this);
  txtDecode->setUndoRedoEnabled(false);
  dw->setWindowTitle(tr("DecodeText"));
  dw->setObjectName("DecodeText");
  dw->setMinimumSize(450, 300);
  dw->setWidget(txtDecode);
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

#define EnsureSetEnabled(w, b)                                                 \
  if (w)                                                                       \
    w->setEnabled(b);

  // connect hexeditor status
  connect(hexeditor, &QHexView::canUndoChanged, this, [=](bool b) {
    toolbartools[ToolBoxIndex::Undo]->setEnabled(b);
    toolmenutools[ToolBoxIndex::Undo]->setEnabled(b);
  });
  connect(hexeditor, &QHexView::canRedoChanged, this, [=](bool b) {
    toolbartools[ToolBoxIndex::Redo]->setEnabled(b);
    toolmenutools[ToolBoxIndex::Redo]->setEnabled(b);
  });
  connect(hexeditor, &QHexView::documentSaved, this, [=](bool b) {
    CheckEnabled;
    iSaved->setPixmap(b ? infoSaved : infoUnsaved);
  });
  connect(hexeditor, &QHexView::documentKeepSize, this, [=](bool b) {
    CheckEnabled;
    iOver->setIcon(b ? infoCannotOver : infoCanOver);
  });
  connect(hexeditor, &QHexView::documentLockedFile, this, [=](bool b) {
    CheckEnabled;
    iLocked->setIcon(b ? infoLock : infoUnLock);
  });
  connect(hexeditor, &QHexView::copyLimitRaised, this, [=] {
    CheckEnabled;
    DMessageManager::instance()->sendMessage(this, ICONRES("copy"),
                                             tr("CopyLimit"));
  });

#define ConnectShortCut(ShortCut, Slot)                                        \
  s = new QShortcut(ShortCut, this);                                           \
  connect(s, &QShortcut::activated, this, &Slot);

#define ConnectShortCut2(ShortCut, Slot)                                       \
  s = new QShortcut(ShortCut, this);                                           \
  connect(s, &QShortcut::activated, this, Slot);

  QShortcut *s;
  ConnectShortCut(QKeySequence::New, MainWindow::on_newfile);
  ConnectShortCut(keynewb, MainWindow::on_newbigfile);
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
  ConnectShortCut(keyopenr, MainWindow::on_openregion);
  ConnectShortCut(keymetadata, MainWindow::on_metadata);
  ConnectShortCut(keyfill, MainWindow::on_fill);
  ConnectShortCut(keyfillnop, MainWindow::on_fillnop);
  ConnectShortCut(keyfillzero, MainWindow::on_fillzero);
  ConnectShortCut(keyexport, MainWindow::on_exportfile);
  ConnectShortCut(keyloadplg, MainWindow::on_loadplg);
  ConnectShortCut(keyencoding, MainWindow::on_encoding);
  ConnectShortCut(keyfinfo, MainWindow::on_fileInfo);
  ConnectShortCut(keyopenws, MainWindow::on_openworkspace);
  ConnectShortCut(keycuthex, MainWindow::on_cuthex);
  ConnectShortCut(keycopyhex, MainWindow::on_copyhex);
  ConnectShortCut(keypastehex, MainWindow::on_pastehex);
  ConnectShortCut(keybookmark, MainWindow::on_bookmark);
  ConnectShortCut(keybookmarkcls, MainWindow::on_bookmarkcls);
  ConnectShortCut(keybookmarkdel, MainWindow::on_bookmarkdel);
  ConnectShortCut2(keymetabg, [=] {
    this->on_metadatabg(!hexeditor->document()->metabgVisible());
  });
  ConnectShortCut2(keymetafg, [=] {
    this->on_metadatafg(!hexeditor->document()->metafgVisible());
  });
  ConnectShortCut2(keymetacom, [=] {
    this->on_metadatacomment(!hexeditor->document()->metaCommentVisible());
  });
  ConnectShortCut(keymetashow, MainWindow::on_metashowall);
  ConnectShortCut(keymetahide, MainWindow::on_metahideall);

  Logger::info(tr("SettingLoading"));

  // setting
  _font = this->font();
  _hexeditorfont = QHexView::getHexeditorFont();
  m_settings = new Settings(this);
  connect(m_settings, &Settings::sigAdjustFont, this, [=](QString name) {
    _font.setFamily(name);
    numshowtable->setFont(_font);
    findresult->setFont(_font);
    pluginInfo->setFont(_font);
    txtDecode->setFont(_font);
  });
  connect(m_settings, &Settings::sigShowColNumber, this,
          [=](bool b) { _showheader = b; });
  connect(m_settings, &Settings::sigAdjustEditorFontSize, this,
          [=](int fontsize) {
            _hexeditorfont.setPointSize(fontsize);
            hexeditor->setFont(_hexeditorfont);
          });
  connect(m_settings, &Settings::sigAdjustInfoFontSize, this,
          [=](int fontsize) {
            _font.setPointSize(fontsize);
            numshowtable->setFont(_font);
            findresult->setFont(_font);
          });
  connect(m_settings, &Settings::sigShowEncodingText, this,
          [=](bool b) { _showascii = b; });
  connect(m_settings, &Settings::sigShowAddressNumber, this,
          [=](bool b) { _showaddr = b; });
  connect(m_settings, &Settings::sigChangeWindowSize, this,
          [=](QString mode) { _windowmode = mode; });
  connect(m_settings, &Settings::sigChangePluginEnabled, this,
          [=](bool b) { _enableplugin = b; });
  connect(m_settings, &Settings::sigChangeRootPluginEnabled, this,
          [=](bool b) { _rootenableplugin = b; });
  connect(m_settings, &Settings::sigChangedEncoding, this,
          [=](QString encoding) { _encoding = encoding; });
  connect(m_settings, &Settings::sigAdjustFindMaxCount, this,
          [=](int count) { _findmax = count; });
  connect(m_settings, &Settings::sigAdjustCopyLimit, this, [=](int count) {
    _cplim = count;
    for (auto &item : hexfiles) {
      item.doc->setCopyLimit(count);
    }
  });
  connect(m_settings, &Settings::sigAdjustDecodeStringLimit, this,
          [=](int count) { _decstrlim = count; });

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
    if (!_rootenableplugin && Utilities::isRoot())
      _enableplugin = false;
  }

  if (_enableplugin) {
    addToolBarBreak();
    Logger::info(tr("PluginLoading"));
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
    connect(plgsys, &PluginSystem::PluginToolButtonAdd, this,
            &MainWindow::PluginToolButtonAdd);
    connect(plgsys, &PluginSystem::PluginToolBarAdd, this,
            &MainWindow::PluginToolBarAdd);
    plgsys->LoadPlugin();
  } else {
    plgmenu->setEnabled(false);
    Logger::critical(tr("UnLoadPluginSetting"));
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
  for (auto &item : hexfiles) {
    item.doc->deleteLater();
    item.render->deleteLater();
  }
  hexeditor->disconnect();
}

void MainWindow::PluginMenuNeedAdd(QMenu *menu) {
  if (menu) {
    Logger::warning(tr("MenuName :") + menu->title());
    plgmenu->addMenu(menu);
  }
}

void MainWindow::PluginDockWidgetAdd(
    const QString &pluginname,
    const QHash<QDockWidget *, Qt::DockWidgetArea> &rdw) {
  if (rdw.isEmpty()) {
    return;
  }
  if (rdw.count() == 1) {
    auto dockw = rdw.begin().key();
    auto align = rdw.begin().value();
    if (align == Qt::DockWidgetArea::NoDockWidgetArea ||
        align == Qt::DockWidgetArea::AllDockWidgetAreas ||
        align == Qt::DockWidgetArea::DockWidgetArea_Mask) {
      align = Qt::DockWidgetArea::BottomDockWidgetArea;
    }
    auto t = dockw->windowTitle();
    if (!t.trimmed().length()) {
      Logger::critical(tr("ErrDockWidgetAddNoName"));
      return;
    }
    Logger::warning(tr("DockWidgetName :") + t);
    dockw->setParent(this);
    addDockWidget(align, dockw);
    dockw->close();
    auto a = new QAction(t, winmenu);
    connect(a, &QAction::triggered, this, [dockw] {
      dockw->show();
      dockw->raise();
    });
    winmenu->addAction(a);
  } else {
    DMenu *d = new DMenu(pluginname, winmenu);
    QDockWidget *dw = nullptr;
    for (auto p = rdw.constBegin(); p != rdw.constEnd(); p++) {
      auto dockw = p.key();
      auto align = p.value();
      if (align == Qt::DockWidgetArea::NoDockWidgetArea ||
          align == Qt::DockWidgetArea::AllDockWidgetAreas ||
          align == Qt::DockWidgetArea::DockWidgetArea_Mask) {
        align = Qt::DockWidgetArea::BottomDockWidgetArea;
      }
      auto t = dockw->windowTitle();
      if (!t.trimmed().length()) {
        Logger::critical(tr("ErrDockWidgetAddNoName"));
        d->deleteLater();
        return;
      }
      Logger::warning(tr("DockWidgetName :") + t);
      dockw->setParent(this);
      addDockWidget(align, dockw);
      dockw->close();
      if (dw)
        tabifyDockWidget(dw, dockw);
      dw = dockw;
      auto a = new QAction(t, winmenu);
      connect(a, &QAction::triggered, this, [dockw] {
        dockw->show();
        dockw->raise();
      });
      d->addAction(a);
    }
    winmenu->addMenu(d);
  }
}

void MainWindow::PluginToolButtonAdd(QToolButton *btn) {
  if (btn) {
    btn->setParent(toolbar);
    toolbar->addWidget(btn);
  }
}

void MainWindow::PluginToolBarAdd(QToolBar *tb, Qt::ToolBarArea align) {
  if (tb) {
    if (align == Qt::ToolBarArea::NoToolBarArea ||
        align == Qt::ToolBarArea::AllToolBarAreas ||
        align == Qt::ToolBarArea::ToolBarArea_Mask) {
      align = Qt::ToolBarArea::TopToolBarArea;
    }
    addToolBar(align, tb);
  }
}

void MainWindow::connectBase(const IWingPlugin *plugin) {
  if (plugin == nullptr)
    return;

#define ConnectBase(Signal, Slot) connect(plugin, &Signal, this, &Slot)
#define ConnectBaseLamba(Signal, Function)                                     \
  connect(plugin, &Signal, this, Function)
#define ConnectBase2(Signal, Slot)                                             \
  connect(&plugin->reader, &Signal, this, &Slot)
#define ConnectBaseLamba2(Signal, Function)                                    \
  connect(&plugin->reader, &Signal, this, Function)

  // connect neccessary signal-slot
  ConnectBase(IWingPlugin::requestControl, MainWindow::requestControl);
  ConnectBase(IWingPlugin::requestRelease, MainWindow::requestRelease);
  ConnectBase(IWingPlugin::hasControl, MainWindow::hasControl);
  ConnectBaseLamba(IWingPlugin::getParentWindow, [=] { return this; });
  ConnectBaseLamba(
      IWingPlugin::toast, [=](const QIcon &icon, const QString &message) {
        DMessageManager::instance()->sendMessage(this, icon, message);
      });
  ConnectBaseLamba(IWingPlugin::debug,
                   [=](const QString &message) { Logger::debug(message); });
  ConnectBaseLamba(IWingPlugin::info,
                   [=](const QString &message) { Logger::info(message); });
  ConnectBaseLamba(IWingPlugin::warn,
                   [=](const QString &message) { Logger::warning(message); });
  ConnectBaseLamba(IWingPlugin::error,
                   [=](const QString &message) { Logger::critical(message); });
  ConnectBaseLamba(IWingPlugin::newDDialog, [=] { return new DDialog(this); });
  ConnectBaseLamba(IWingPlugin::newAboutDialog,
                   [=](const QPixmap &img, const QStringList &searchPaths,
                       const QString &source) {
                     return new AboutSoftwareDialog(this, img, searchPaths,
                                                    source);
                   });
  ConnectBaseLamba(IWingPlugin::newSponsorDialog,
                   [=](const QPixmap &qrcode, const QString &message) {
                     return new SponsorDialog(this, message, qrcode);
                   });
  ConnectBaseLamba(IWingPlugin::addContent,
                   [=](QDialog *ddialog, QWidget *widget, Qt::Alignment align) {
                     auto d = qobject_cast<DDialog *>(ddialog);
                     if (!d)
                       return false;
                     d->addContent(widget, align);
                     return true;
                   });
  ConnectBaseLamba(IWingPlugin::addSpace, [=](QDialog *ddialog, int space) {
    auto d = qobject_cast<DDialog *>(ddialog);
    if (!d)
      return false;
    d->addSpacing(space);
    return true;
  });
  ConnectBaseLamba(IWingPlugin::moveToCenter, [=](QDialog *ddialog) {
    Dtk::Widget::moveToCenter(ddialog);
  });

#define PCHECK(T, TF, F)                                                       \
  if (hexfiles.count() > 0) {                                                  \
    if (plgsys->hasControl() && _pcurfile >= 0) {                              \
      T;                                                                       \
    } else {                                                                   \
      TF;                                                                      \
    }                                                                          \
  } else {                                                                     \
    F;                                                                         \
  }

#define PCHECKRETURN(T, TF, F)                                                 \
  if (hexfiles.count() > 0) {                                                  \
    if (plgsys->hasControl() && _pcurfile >= 0)                                \
      return T;                                                                \
    else                                                                       \
      return TF;                                                               \
  }                                                                            \
  return F;

  ConnectBaseLamba2(WingPlugin::Reader::currentDoc,
                    [=] { PCHECKRETURN(_pcurfile, _currentfile, -1) });
  ConnectBaseLamba2(WingPlugin::Reader::currentHostDoc,
                    [=] { return _currentfile; });
  ConnectBaseLamba2(WingPlugin::Reader::currentDocFilename, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].filename, hexfiles[_currentfile].filename,
                 QString());
  });

  ConnectBaseLamba2(WingPlugin::Reader::isLocked, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->isLocked(),
                 hexeditor->document()->isLocked(), true);
  });

  ConnectBaseLamba2(WingPlugin::Reader::isLocked, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->isLocked(),
                 hexeditor->document()->isLocked(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::isEmpty, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->isEmpty(),
                 hexeditor->document()->isEmpty(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::isKeepSize, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->isKeepSize(),
                 hexeditor->document()->isKeepSize(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::isModified, [=] {
    PCHECKRETURN(!hexfiles[_pcurfile].doc->isDocSaved(),
                 hexeditor->document()->isDocSaved(), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::isReadOnly, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->isReadOnly(),
                 hexeditor->document()->isReadOnly(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::documentLines, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->documentLines(),
                 hexeditor->documentLines(), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::documentBytes, [=] {
    PCHECKRETURN(quint64(hexfiles[_pcurfile].doc->length()),
                 quint64(hexeditor->documentBytes()), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::currentPos, [=] {
    HexPosition pos;
    memset(&pos, 0, sizeof(HexPosition));
    QHexPosition qpos;
    PCHECK(qpos = hexfiles[_pcurfile].doc->cursor()->position(),
           qpos = hexeditor->document()->cursor()->position(),
           memset(&qpos, 0, sizeof(HexPosition)));
    pos.line = qpos.line;
    pos.column = qpos.column;
    pos.lineWidth = qpos.lineWidth;
    pos.nibbleindex = qpos.nibbleindex;
    return pos;
  });
  ConnectBaseLamba2(WingPlugin::Reader::selectionPos, [=] {
    HexPosition pos;
    memset(&pos, 0, sizeof(HexPosition));
    QHexCursor *cur;
    PCHECK(cur = hexfiles[_pcurfile].doc->cursor(),
           cur = hexeditor->document()->cursor(), {
             memset(&pos, 0, sizeof(HexPosition));
             return pos;
           });
    pos.line = cur->selectionLine();
    pos.column = cur->selectionColumn();
    pos.nibbleindex = cur->selectionNibble();
    pos.lineWidth = cur->position().lineWidth;
    return pos;
  });
  ConnectBaseLamba2(WingPlugin::Reader::currentRow, [=] {
    PCHECKRETURN(quint64(hexfiles[_pcurfile].doc->cursor()->currentLine()),
                 quint64(hexeditor->currentRow()), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::currentColumn, [=] {
    PCHECKRETURN(quint64(hexfiles[_pcurfile].doc->cursor()->currentColumn()),
                 quint64(hexeditor->currentColumn()), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::currentOffset, [=] {
    PCHECKRETURN(
        quint64(hexfiles[_pcurfile].doc->cursor()->position().offset()),
        quint64(hexeditor->currentOffset()), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::selectLength, [=] {
    PCHECKRETURN(quint64(hexfiles[_pcurfile].doc->cursor()->selectionLength()),
                 quint64(hexeditor->selectlength()), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::selectedBytes, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->selectedBytes(),
                 hexeditor->document()->selectedBytes(), QByteArray());
  });
  ConnectBaseLamba2(WingPlugin::Reader::stringVisible, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->stringVisible(),
                 hexeditor->renderer()->stringVisible(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::headerVisible, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->headerVisible(),
                 hexeditor->renderer()->headerVisible(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::addressVisible, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->addressVisible(),
                 hexeditor->renderer()->addressVisible(), true);
  });
  ConnectBaseLamba2(WingPlugin::Reader::addressBase, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->baseAddress(),
                 hexeditor->document()->baseAddress(), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::atEnd, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->atEnd(),
                 hexeditor->document()->atEnd(), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::canUndo, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->canUndo(),
                 hexeditor->document()->canUndo(), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::canRedo, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].doc->canRedo(),
                 hexeditor->document()->canRedo(), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::documentLastLine, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->documentLastLine(),
                 hexeditor->renderer()->documentLastLine(), quint64(0));
  });
  ConnectBaseLamba2(WingPlugin::Reader::documentLastColumn, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->documentLastColumn(),
                 hexeditor->renderer()->documentLastColumn(), 0);
  });
  ConnectBaseLamba2(WingPlugin::Reader::copy, [=](bool hex) {
    PCHECK(hexfiles[_pcurfile].doc->copy(hex),
           hexeditor->document()->copy(hex), );
  });
  ConnectBaseLamba2(WingPlugin::Reader::read, [=](qint64 offset, int len) {
    PCHECKRETURN(hexfiles[_pcurfile].doc->read(offset, len),
                 hexeditor->document()->read(offset, len), QByteArray());
  });

  ConnectBaseLamba2(WingPlugin::Reader::readInt8, [=](qint64 offset) {
    PCHECK(
        {
          auto buffer = hexfiles[_pcurfile].doc->read(offset, sizeof(qint8));
          auto pb = reinterpret_cast<const qint8 *>(buffer.constData());
          return *pb;
        },
        {
          auto buffer = hexeditor->document()->read(offset, sizeof(qint8));
          auto pb = reinterpret_cast<const qint8 *>(buffer.constData());
          return *pb;
        },
        return qint8(-1););
  });
  ConnectBaseLamba2(WingPlugin::Reader::readInt16, [=](qint64 offset) {
    PCHECK(
        {
          auto buffer = hexfiles[_pcurfile].doc->read(offset, sizeof(qint16));
          auto pb = reinterpret_cast<const qint16 *>(buffer.constData());
          return *pb;
        },
        {
          auto buffer = hexeditor->document()->read(offset, sizeof(qint16));
          auto pb = reinterpret_cast<const qint16 *>(buffer.constData());
          return *pb;
        },
        return qint16(-1););
  });
  ConnectBaseLamba2(WingPlugin::Reader::readInt32, [=](qint64 offset) {
    PCHECK(
        {
          auto buffer = hexfiles[_pcurfile].doc->read(offset, sizeof(qint32));
          auto pb = reinterpret_cast<const qint32 *>(buffer.constData());
          return *pb;
        },
        {
          auto buffer = hexeditor->document()->read(offset, sizeof(qint32));
          auto pb = reinterpret_cast<const qint32 *>(buffer.constData());
          return *pb;
        },
        return qint32(-1););
  });
  ConnectBaseLamba2(WingPlugin::Reader::readInt64, [=](qint64 offset) {
    PCHECK(
        {
          auto buffer = hexfiles[_pcurfile].doc->read(offset, sizeof(qint64));
          auto pb = reinterpret_cast<const qint64 *>(buffer.constData());
          return *pb;
        },
        {
          auto buffer = hexeditor->document()->read(offset, sizeof(qint64));
          auto pb = reinterpret_cast<const qint64 *>(buffer.constData());
          return *pb;
        },
        return qint64(-1););
  });
  ConnectBaseLamba2(WingPlugin::Reader::readString,
                    [=](qint64 offset, const QString &encoding) {
                      PCHECK(
                          {
                            auto doc = hexfiles[_pcurfile].doc;
                            auto render = hexfiles[_pcurfile].render;
                            auto pos =
                                doc->searchForward(offset, QByteArray(1, 0));
                            if (pos < 0)
                              return QString();
                            auto buffer = doc->read(offset, int(pos - offset));
                            QString enco = encoding;
                            if (!enco.length())
                              enco = render->encoding();
                            auto enc = QTextCodec::codecForName(enco.toUtf8());
                            auto d = enc->makeDecoder();
                            auto unicode = d->toUnicode(buffer);
                            return unicode;
                          },
                          {
                            auto doc = hexeditor->document();
                            auto render = hexeditor->renderer();
                            auto pos =
                                doc->searchForward(offset, QByteArray(1, 0));
                            if (pos < 0)
                              return QString();
                            auto buffer = doc->read(offset, int(pos - offset));
                            QString enco = encoding;
                            if (!enco.length())
                              enco = render->encoding();
                            auto enc = QTextCodec::codecForName(enco.toUtf8());
                            auto d = enc->makeDecoder();
                            auto unicode = d->toUnicode(buffer);
                            return unicode;
                          },
                          return QString());
                    });

  ConnectBaseLamba2(WingPlugin::Reader::findAllBytes,
                    [=](qlonglong begin, qlonglong end, QByteArray b,
                        QList<quint64> &results, int maxCount) {
                      PCHECK(hexfiles[_pcurfile].doc->findAllBytes(
                                 begin, end, b, results, maxCount),
                             hexeditor->document()->findAllBytes(
                                 begin, end, b, results, maxCount), );
                    });
  ConnectBaseLamba2(WingPlugin::Reader::searchForward,
                    [=](qint64 begin, const QByteArray &ba) {
                      PCHECKRETURN(
                          hexfiles[_pcurfile].doc->searchForward(begin, ba),
                          hexeditor->document()->searchForward(begin, ba),
                          qint64(-1));
                    });
  ConnectBaseLamba2(WingPlugin::Reader::searchBackward,
                    [=](qint64 begin, const QByteArray &ba) {
                      PCHECKRETURN(
                          hexfiles[_pcurfile].doc->searchBackward(begin, ba),
                          hexeditor->document()->searchBackward(begin, ba),
                          qint64(-1));
                    });
  ConnectBaseLamba2(WingPlugin::Reader::getMetaLine, [=](quint64 line) {
    auto ometas = hexfiles[_pcurfile].doc->metadata()->get(line);
    HexLineMetadata metas;
    for (auto &item : ometas) {
      metas.push_back(HexMetadataItem(item.line, item.start, item.length,
                                      item.foreground, item.background,
                                      item.comment));
    }
    return metas;
  });
  ConnectBaseLamba2(WingPlugin::Reader::getMetadatas, [=](qint64 offset) {
    auto ometaline = hexfiles[_pcurfile].doc->metadata()->gets(offset);
    QList<HexMetadataAbsoluteItem> metaline;
    for (auto &item : ometaline) {
      metaline.push_back(
          HexMetadataAbsoluteItem(item.begin, item.end, item.foreground,
                                  item.background, item.comment));
    }
    return metaline;
  });
  ConnectBaseLamba2(WingPlugin::Reader::lineHasMetadata, [=](quint64 line) {
    return hexfiles[_pcurfile].doc->metadata()->lineHasMetadata(line);
  });

  ConnectBaseLamba2(WingPlugin::Reader::lineHasBookMark, [=](quint64 line) {
    PCHECKRETURN(hexfiles[_pcurfile].doc->lineHasBookMark(line),
                 hexeditor->document()->lineHasBookMark(line), false);
  });
  ConnectBaseLamba2(WingPlugin::Reader::getsBookmarkPos, [=](quint64 line) {
    PCHECKRETURN(hexfiles[_pcurfile].doc->getsBookmarkPos(line),
                 hexeditor->document()->getsBookmarkPos(line), QList<qint64>());
  });
  ConnectBaseLamba2(WingPlugin::Reader::bookMark, [=](qint64 pos) {
    auto b = hexfiles[_pcurfile].doc->bookMark(pos);
    BookMark book;
    book.pos = b.pos;
    book.comment = b.comment;
    return book;
  });
  ConnectBaseLamba2(WingPlugin::Reader::bookMarkComment, [=](qint64 pos) {
    PCHECKRETURN(hexfiles[_pcurfile].doc->bookMarkComment(pos),
                 hexeditor->document()->bookMarkComment(pos), QString());
  });
  ConnectBaseLamba2(
      WingPlugin::Reader::getBookMarks, [=](QList<BookMark> &bookmarks) {
        PCHECK(
            {
              auto bs = hexfiles[_pcurfile].doc->getAllBookMarks();
              for (auto &item : bs) {
                BookMark i;
                i.pos = item.pos;
                i.comment = item.comment;
                bookmarks.push_back(i);
              }
            },
            {
              auto bs = hexeditor->document()->getAllBookMarks();
              for (auto &item : bs) {
                BookMark i;
                i.pos = item.pos;
                i.comment = item.comment;
                bookmarks.push_back(i);
              }
            }, );
      });
  ConnectBaseLamba2(WingPlugin::Reader::existBookMark, [=](qint64 pos) {
    PCHECKRETURN(hexfiles[_pcurfile].doc->existBookMark(pos),
                 hexeditor->document()->existBookMark(pos), false);
  });

  ConnectBaseLamba2(WingPlugin::Reader::getOpenFiles, [=] {
    QList<QString> files;
    for (auto &item : hexfiles) {
      files.push_back(item.filename);
    }
    return files;
  });
  ConnectBase2(WingPlugin::Reader::getSupportedEncodings,
               Utilities::getEncodings);
  ConnectBaseLamba2(WingPlugin::Reader::currentEncoding, [=] {
    PCHECKRETURN(hexfiles[_pcurfile].render->encoding(),
                 hexeditor->renderer()->encoding(), QString());
  });
}

void MainWindow::connectControl(const IWingPlugin *plugin) {

  auto pctl = &plugin->controller;

#define ConnectControlLamba(Signal, Function)                                  \
  connect(plugin, &Signal, this, Function)
#define ConnectControlLamba2(Signal, Function)                                 \
  connect(pctl, &Signal, this, Function)

  ConnectControlLamba2(
      WingPlugin::Controller::switchDocument, [=](int index, bool gui) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        if (gui) {
          return setFilePage(index);
        } else {
          if (index >= 0 && index < hexfiles.count()) {
            _pcurfile = index;
            return true;
          }
        }
        return false;
      });

  ConnectControlLamba2(WingPlugin::Controller::setLockedFile, [=](bool b) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECKRETURN(hexfiles[_pcurfile].doc->setLockedFile(b),
                 hexeditor->setLockedFile(b), false);
  });
  ConnectControlLamba2(WingPlugin::Controller::setKeepSize, [=](bool b) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECKRETURN(hexfiles[_pcurfile].doc->setKeepSize(b),
                 hexeditor->setKeepSize(b), false);
  });
  ConnectControlLamba2(WingPlugin::Controller::setStringVisible, [=](bool b) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(hexfiles[_pcurfile].render->setStringVisible(b),
           hexeditor->renderer()->setStringVisible(b), );
  });
  ConnectControlLamba2(WingPlugin::Controller::setHeaderVisible, [=](bool b) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(hexfiles[_pcurfile].render->setHeaderVisible(b),
           hexfiles[_pcurfile].render->setHeaderVisible(b), );
  });
  ConnectControlLamba2(WingPlugin::Controller::setAddressVisible, [=](bool b) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(hexfiles[_pcurfile].render->setAddressVisible(b),
           hexeditor->setAddressVisible(b), );
  });
  ConnectControlLamba2(
      WingPlugin::Controller::setAddressBase, [=](quint64 base) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(hexfiles[_pcurfile].doc->setBaseAddress(base),
               hexeditor->setAddressBase(base), );
      });
  ConnectControlLamba2(WingPlugin::Controller::undo, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(hexfiles[_pcurfile].doc->undo(), hexeditor->document()->undo(), );
  });
  ConnectControlLamba2(WingPlugin::Controller::redo, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(hexfiles[_pcurfile].doc->redo(), hexeditor->document()->redo(), );
  });
  ConnectControlLamba2(WingPlugin::Controller::cut, [=](bool hex) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECKRETURN(hexfiles[_pcurfile].doc->cut(hex),
                 hexeditor->document()->cut(hex), false);
  });
  ConnectControlLamba2(WingPlugin::Controller::paste, [=](bool hex) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(hexfiles[_pcurfile].doc->paste(hex),
           hexeditor->document()->paste(hex), )
  });

  connect(pctl, QOverload<qint64, uchar>::of(&WingPlugin::Controller::insert),
          this, [=](qint64 offset, uchar b) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECKRETURN(hexfiles[_pcurfile].doc->insert(offset, b),
                         hexeditor->document()->insert(offset, b), false);
          });

  connect(pctl,
          QOverload<qint64, const QByteArray &>::of(
              &WingPlugin::Controller::insert),
          this, [=](qint64 offset, const QByteArray &data) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECKRETURN(hexfiles[_pcurfile].doc->insert(offset, data),
                         hexeditor->document()->insert(offset, data), false);
          });
  connect(pctl, QOverload<qint64, uchar>::of(&WingPlugin::Controller::write),
          this, [=](qint64 offset, uchar b) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECKRETURN(hexfiles[_pcurfile].doc->replace(offset, b),
                         hexeditor->document()->replace(offset, b), false);
          });
  ConnectControlLamba2(
      WingPlugin::Controller::insertInt8, [=](qint64 offset, qint8 value) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              auto buffer = reinterpret_cast<char *>(&value);
              return doc->insert(offset, QByteArray(buffer, sizeof(qint8)));
            },
            {
              auto doc = hexeditor->document();
              auto buffer = reinterpret_cast<char *>(&value);
              return doc->insert(offset, QByteArray(buffer, sizeof(qint8)));
            },
            return false;)
      });
  ConnectControlLamba2(
      WingPlugin::Controller::insertInt16, [=](qint64 offset, qint16 value) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              auto buffer = reinterpret_cast<char *>(&value);
              return doc->insert(offset, QByteArray(buffer, sizeof(qint16)));
            },
            {
              auto doc = hexeditor->document();
              auto buffer = reinterpret_cast<char *>(&value);
              return doc->insert(offset, QByteArray(buffer, sizeof(qint16)));
            },
            return false;)
      });
  ConnectControlLamba2(
      WingPlugin::Controller::insertInt16, [=](qint64 offset, qint32 value) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              auto buffer = reinterpret_cast<char *>(&value);
              return doc->insert(offset, QByteArray(buffer, sizeof(qint32)));
            },
            {
              auto doc = hexeditor->document();
              auto buffer = reinterpret_cast<char *>(&value);
              return doc->insert(offset, QByteArray(buffer, sizeof(qint32)));
            },
            return false;)
      });
  ConnectControlLamba2(
      WingPlugin::Controller::insertInt16, [=](qint64 offset, qint64 value) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              auto buffer = reinterpret_cast<char *>(&value);
              return doc->insert(offset, QByteArray(buffer, sizeof(qint64)));
            },
            {
              auto doc = hexeditor->document();
              auto buffer = reinterpret_cast<char *>(&value);
              return doc->insert(offset, QByteArray(buffer, sizeof(qint64)));
            },
            return false;)
      });
  ConnectControlLamba2(
      WingPlugin::Controller::insertString,
      [=](qint64 offset, QString value, const QString &encoding) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto render = hexfiles[_pcurfile].render;
              QString enco = encoding;
              if (!enco.length())
                enco = render->encoding();
              auto enc = QTextCodec::codecForName(enco.toUtf8());
              auto e = enc->makeEncoder();
              return hexfiles[_pcurfile].doc->insert(offset,
                                                     e->fromUnicode(value));
            },
            {
              auto render = hexeditor->renderer();
              QString enco = encoding;
              if (!enco.length())
                enco = render->encoding();
              auto enc = QTextCodec::codecForName(enco.toUtf8());
              auto e = enc->makeEncoder();
              return hexeditor->document()->insert(offset,
                                                   e->fromUnicode(value));
            },
            return false);
      });
  connect(
      pctl,
      QOverload<qint64, const QByteArray &>::of(&WingPlugin::Controller::write),
      this, [=](qint64 offset, const QByteArray &data) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECKRETURN(hexfiles[_pcurfile].doc->replace(offset, data),
                     hexeditor->document()->replace(offset, data), false);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::writeInt8, [=](qint64 offset, qint8 value) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto buffer = reinterpret_cast<char *>(&value);
              QByteArray data(buffer, sizeof(qint8));
              return hexfiles[_pcurfile].doc->replace(offset, data);
            },
            {
              auto buffer = reinterpret_cast<char *>(&value);
              QByteArray data(buffer, sizeof(qint8));
              return hexeditor->document()->replace(offset, data);
            },
            return false);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::writeInt16, [=](qint64 offset, qint16 value) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto buffer = reinterpret_cast<char *>(&value);
              QByteArray data(buffer, sizeof(qint16));
              return hexfiles[_pcurfile].doc->replace(offset, data);
            },
            {
              auto buffer = reinterpret_cast<char *>(&value);
              QByteArray data(buffer, sizeof(qint16));
              return hexeditor->document()->replace(offset, data);
            },
            return false);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::writeInt32, [=](qint64 offset, qint32 value) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto buffer = reinterpret_cast<char *>(&value);
              QByteArray data(buffer, sizeof(qint32));
              return hexfiles[_pcurfile].doc->replace(offset, data);
            },
            {
              auto buffer = reinterpret_cast<char *>(&value);
              QByteArray data(buffer, sizeof(qint32));
              return hexeditor->document()->replace(offset, data);
            },
            return false);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::writeInt64, [=](qint64 offset, qint64 value) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto buffer = reinterpret_cast<char *>(&value);
              QByteArray data(buffer, sizeof(qint64));
              return hexfiles[_pcurfile].doc->replace(offset, data);
            },
            {
              auto buffer = reinterpret_cast<char *>(&value);
              QByteArray data(buffer, sizeof(qint64));
              return hexeditor->document()->replace(offset, data);
            },
            return false);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::writeString,
      [=](qint64 offset, const QString &value, const QString &encoding) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto render = hexfiles[_pcurfile].render;
              QString enco = encoding;
              if (!enco.length())
                enco = render->encoding();
              auto enc = QTextCodec::codecForName(enco.toUtf8());
              auto e = enc->makeEncoder();
              return hexfiles[_pcurfile].doc->replace(offset,
                                                      e->fromUnicode(value));
            },
            {
              auto render = hexeditor->renderer();
              QString enco = encoding;
              if (!enco.length())
                enco = render->encoding();
              auto enc = QTextCodec::codecForName(enco.toUtf8());
              auto e = enc->makeEncoder();
              return hexeditor->document()->replace(offset,
                                                    e->fromUnicode(value));
            },
            return false);
      });
  connect(pctl, QOverload<uchar>::of(&WingPlugin::Controller::append), this,
          [=](uchar b) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECK(
                {
                  auto doc = hexfiles[_pcurfile].doc;
                  auto offset = doc->length();
                  return doc->insert(offset, b);
                },
                {
                  auto doc = hexeditor->document();
                  auto offset = doc->length();
                  return doc->insert(offset, b);
                },
                return false;);
          });

  connect(pctl,
          QOverload<const QByteArray &>::of(&WingPlugin::Controller::append),
          this, [=](const QByteArray &data) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECK(
                {
                  auto doc = hexfiles[_pcurfile].doc;
                  auto offset = doc->length();
                  return doc->insert(offset, data);
                },
                {
                  auto doc = hexeditor->document();
                  auto offset = doc->length();
                  return doc->insert(offset, data);
                },
                return false;);
          });
  ConnectControlLamba2(WingPlugin::Controller::appendInt8, [=](qint8 value) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(
        {
          auto doc = hexfiles[_pcurfile].doc;
          auto offset = doc->length();
          auto buffer = reinterpret_cast<char *>(&value);
          QByteArray data(buffer, sizeof(qint8));
          return doc->insert(offset, data);
        },
        {
          auto doc = hexeditor->document();
          auto offset = doc->length();
          auto buffer = reinterpret_cast<char *>(&value);
          QByteArray data(buffer, sizeof(qint8));
          return doc->insert(offset, data);
        },
        return false;);
  });
  ConnectControlLamba2(WingPlugin::Controller::appendInt16, [=](qint16 value) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(
        {
          auto doc = hexfiles[_pcurfile].doc;
          auto offset = doc->length();
          auto buffer = reinterpret_cast<char *>(&value);
          QByteArray data(buffer, sizeof(qint16));
          return doc->insert(offset, data);
        },
        {
          auto doc = hexeditor->document();
          auto offset = doc->length();
          auto buffer = reinterpret_cast<char *>(&value);
          QByteArray data(buffer, sizeof(qint16));
          return doc->insert(offset, data);
        },
        return false;);
  });
  ConnectControlLamba2(WingPlugin::Controller::appendInt32, [=](qint32 value) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(
        {
          auto doc = hexfiles[_pcurfile].doc;
          auto offset = doc->length();
          auto buffer = reinterpret_cast<char *>(&value);
          QByteArray data(buffer, sizeof(qint32));
          return doc->insert(offset, data);
        },
        {
          auto doc = hexeditor->document();
          auto offset = doc->length();
          auto buffer = reinterpret_cast<char *>(&value);
          QByteArray data(buffer, sizeof(qint32));
          return doc->insert(offset, data);
        },
        return false;);
  });
  ConnectControlLamba2(WingPlugin::Controller::appendInt64, [=](qint64 value) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(
        {
          auto doc = hexfiles[_pcurfile].doc;
          auto offset = doc->length();
          auto buffer = reinterpret_cast<char *>(&value);
          QByteArray data(buffer, sizeof(qint64));
          return doc->insert(offset, data);
        },
        {
          auto doc = hexeditor->document();
          auto offset = doc->length();
          auto buffer = reinterpret_cast<char *>(&value);
          QByteArray data(buffer, sizeof(qint64));
          return doc->insert(offset, data);
        },
        return false;);
  });
  ConnectControlLamba2(
      WingPlugin::Controller::appendString,
      [=](const QString &value, const QString &encoding) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto render = hexfiles[_pcurfile].render;
              QString enco = encoding;
              if (!enco.length())
                enco = render->encoding();
              auto enc = QTextCodec::codecForName(enco.toUtf8());
              auto offset = hexfiles[_pcurfile].doc->length();
              auto e = enc->makeEncoder();
              return hexfiles[_pcurfile].doc->insert(offset,
                                                     e->fromUnicode(value));
            },
            {
              auto render = hexeditor->renderer();
              QString enco = encoding;
              if (!enco.length())
                enco = render->encoding();
              auto enc = QTextCodec::codecForName(enco.toUtf8());
              auto offset = hexeditor->document()->length();
              auto e = enc->makeEncoder();
              return hexeditor->document()->replace(offset,
                                                    e->fromUnicode(value));
            },
            return false);
      });

  ConnectControlLamba2(
      WingPlugin::Controller::remove, [=](qint64 offset, int len) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECKRETURN(hexfiles[_pcurfile].doc->remove(offset, len),
                     hexeditor->document()->remove(offset, len), false);
      });
  ConnectControlLamba2(WingPlugin::Controller::removeAll, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(
        {
          auto doc = hexfiles[_pcurfile].doc;
          auto len = doc->length();
          return doc->remove(0, int(len));
        },
        {
          auto doc = hexeditor->document();
          auto len = doc->length();
          return doc->remove(0, int(len));
        },
        return false;);
  });
  connect(pctl,
          QOverload<const HexPosition &>::of(&WingPlugin::Controller::moveTo),
          this, [=](const HexPosition &pos) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECK(
                {
                  QHexPosition p;
                  p.line = pos.line;
                  p.column = pos.column;
                  p.lineWidth = pos.lineWidth;
                  p.nibbleindex = pos.nibbleindex;
                  hexfiles[_pcurfile].doc->cursor()->moveTo(p);
                },
                {
                  QHexPosition p;
                  p.line = pos.line;
                  p.column = pos.column;
                  p.lineWidth = pos.lineWidth;
                  p.nibbleindex = pos.nibbleindex;
                  hexeditor->document()->cursor()->moveTo(p);
                }, );
          });
  connect(pctl,
          QOverload<quint64, int, int>::of(&WingPlugin::Controller::moveTo),
          this, [=](quint64 line, int column, int nibbleindex) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECK(hexfiles[_pcurfile].doc->cursor()->moveTo(line, column,
                                                             nibbleindex),
                   hexeditor->document()->cursor()->moveTo(line, column,
                                                           nibbleindex), );
          });
  connect(pctl, QOverload<qint64>::of(&WingPlugin::Controller::moveTo), this,
          [=](qint64 offset) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECK(hexfiles[_pcurfile].doc->cursor()->moveTo(offset),
                   hexeditor->document()->cursor()->moveTo(offset), );
          });
  connect(pctl, QOverload<qint64, int>::of(&WingPlugin::Controller::select),
          this, [=](qint64 offset, int length) {
            PCHECK(
                hexfiles[_pcurfile].doc->cursor()->setSelection(offset, length),
                {
                  hexeditor->document()->cursor()->setSelection(offset, length);
                  hexeditor->viewport()->update();
                }, )
          });
  connect(pctl,
          QOverload<quint64, int, int>::of(&WingPlugin::Controller::select),
          this, [=](quint64 line, int column, int nibbleindex) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECK(hexfiles[_pcurfile].doc->cursor()->select(line, column,
                                                             nibbleindex),
                   hexeditor->document()->cursor()->select(line, column,
                                                           nibbleindex), );
          });
  ConnectControlLamba2(WingPlugin::Controller::enabledCursor, [=](bool b) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(hexfiles[_pcurfile].render->enableCursor(b),
           hexeditor->renderer()->enableCursor(b), );
  });

  ConnectControlLamba2(
      WingPlugin::Controller::selectOffset, [=](qint64 offset, int length) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(hexfiles[_pcurfile].doc->cursor()->selectOffset(offset, length),
               hexeditor->document()->cursor()->selectOffset(offset, length), );
      });
  ConnectControlLamba2(
      WingPlugin::Controller::setInsertionMode, [=](bool isinsert) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            hexfiles[_pcurfile].doc->cursor()->setInsertionMode(
                isinsert ? QHexCursor::InsertMode : QHexCursor::OverwriteMode),
            hexeditor->document()->cursor()->setInsertionMode(
                isinsert ? QHexCursor::InsertMode
                         : QHexCursor::OverwriteMode), );
      });
  connect(
      pctl,
      QOverload<qint64, qint64, const QColor &, const QColor &,
                const QString &>::of(&WingPlugin::Controller::metadata),
      this,
      [=](qint64 begin, qint64 end, const QColor &fgcolor,
          const QColor &bgcolor, const QString &comment) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->metadata(begin, end, fgcolor, bgcolor, comment);
              doc->setDocSaved(false);
              return true;
            },
            {
              auto doc = hexeditor->document();
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->metadata(begin, end, fgcolor, bgcolor, comment);
              doc->setDocSaved(false);
              return true;
            },
            return false);
      });
  connect(pctl,
          QOverload<quint64, int, int, const QColor &, const QColor &,
                    const QString &>::of(&WingPlugin::Controller::metadata),
          this,
          [=](quint64 line, int start, int length, const QColor &fgcolor,
              const QColor &bgcolor, const QString &comment) {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECK(
                {
                  auto doc = hexfiles[_pcurfile].doc;
                  if (!doc->isKeepSize())
                    return false;
                  doc->metadata()->metadata(line, start, length, fgcolor,
                                            bgcolor, comment);

                  return true;
                },
                {
                  auto doc = hexeditor->document();
                  if (!doc->isKeepSize())
                    return false;
                  doc->metadata()->metadata(line, start, length, fgcolor,
                                            bgcolor, comment);

                  return true;
                },
                return false);
          });

  ConnectControlLamba2(
      WingPlugin::Controller::removeMetadata, [=](qint64 offset) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              doc->metadata()->removeMetadata(offset);
              doc->setDocSaved(false);
            },
            {
              auto doc = hexeditor->document();
              doc->metadata()->removeMetadata(offset);
              doc->setDocSaved(false);
            }, );
      });

  connect(pctl, QOverload<>::of(&WingPlugin::Controller::clearMeta), this,
          [=]() {
            plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
            PCHECK(
                {
                  auto doc = hexfiles[_pcurfile].doc;
                  if (!doc->isKeepSize())
                    return false;
                  doc->metadata()->clear();
                  return true;
                },
                {
                  auto doc = hexeditor->document();
                  if (!doc->isKeepSize())
                    return false;
                  doc->metadata()->clear();
                  return true;
                },
                return false);
          });

  ConnectControlLamba2(
      WingPlugin::Controller::color,
      [=](quint64 line, int start, int length, const QColor &fgcolor,
          const QColor &bgcolor) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->color(line, start, length, fgcolor, bgcolor);
              return true;
            },
            {
              auto doc = hexeditor->document();
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->color(line, start, length, fgcolor, bgcolor);
              return true;
            },
            return false);
      });

  ConnectControlLamba2(
      WingPlugin::Controller::comment,
      [=](quint64 line, int start, int length, const QString &comment) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->comment(line, start, length, comment);
              return true;
            },
            {
              auto doc = hexeditor->document();
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->comment(line, start, length, comment);
              return true;
            },
            return false);
      });

  ConnectControlLamba2(
      WingPlugin::Controller::foreground,
      [=](quint64 line, int start, int length, const QColor &fgcolor) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->foreground(line, start, length, fgcolor);
              return true;
            },
            {
              auto doc = hexeditor->document();
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->foreground(line, start, length, fgcolor);
              return true;
            },
            return false);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::background,
      [=](quint64 line, int start, int length, const QColor &bgcolor) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              auto doc = hexfiles[_pcurfile].doc;
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->background(line, start, length, bgcolor);
              return true;
            },
            {
              auto doc = hexeditor->document();
              if (!doc->isKeepSize())
                return false;
              doc->metadata()->background(line, start, length, bgcolor);
              return true;
            },
            return false);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::applyMetas,
      [=](const QList<HexMetadataAbsoluteItem> &metas) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(
            {
              QList<QHexMetadataAbsoluteItem> ms;
              for (auto &item : metas) {
                QHexMetadataAbsoluteItem i;
                i.begin = item.begin;
                i.end = item.end;
                i.comment = item.comment;
                i.background = item.background;
                i.foreground = item.foreground;
                ms.append(i);
              }
              hexfiles[_pcurfile].doc->metadata()->applyMetas(ms);
            },
            {
              QList<QHexMetadataAbsoluteItem> ms;
              for (auto &item : metas) {
                QHexMetadataAbsoluteItem i;
                i.begin = item.begin;
                i.end = item.end;
                i.comment = item.comment;
                i.background = item.background;
                i.foreground = item.foreground;
                ms.append(i);
              }
              hexeditor->document()->metadata()->applyMetas(ms);
            }, );
      });
  ConnectControlLamba2(WingPlugin::Controller::setMetafgVisible, [=](bool b) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(hexfiles[_pcurfile].doc->setMetafgVisible(b),
           hexeditor->document()->SetMetafgVisible(b), );
  });
  ConnectControlLamba2(WingPlugin::Controller::setMetabgVisible, [=](bool b) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECK(hexfiles[_pcurfile].doc->setMetabgVisible(b),
           hexeditor->document()->SetMetabgVisible(b), );
  });
  ConnectControlLamba2(
      WingPlugin::Controller::setMetaCommentVisible, [=](bool b) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECK(hexfiles[_pcurfile].doc->setMetaCommentVisible(b),
               hexeditor->document()->SetMetaCommentVisible(b), );
      });

  ConnectControlLamba2(
      WingPlugin::Controller::setCurrentEncoding, [=](const QString &encoding) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECKRETURN(hexfiles[_pcurfile].render->setEncoding(encoding),
                     hexeditor->renderer()->setEncoding(encoding), false);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::addBookMark,
      [=](qint64 pos, const QString &comment) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECKRETURN(hexfiles[_pcurfile].doc->addBookMark(pos, comment),
                     hexeditor->document()->addBookMark(pos, comment), false);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::modBookMark,
      [=](qint64 pos, const QString &comment) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        PCHECKRETURN(hexfiles[_pcurfile].doc->modBookMark(pos, comment),
                     hexeditor->document()->modBookMark(pos, comment), false);
      });
  ConnectControlLamba2(WingPlugin::Controller::removeBookMark, [=](qint64 pos) {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECKRETURN(hexfiles[_pcurfile].doc->removeBookMark(pos),
                 hexeditor->document()->removeBookMark(pos), false);
  });
  ConnectControlLamba2(WingPlugin::Controller::clearBookMark, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    PCHECKRETURN(hexfiles[_pcurfile].doc->clearBookMark(),
                 hexeditor->document()->clearBookMark(), false);
  });
  ConnectControlLamba2(WingPlugin::Controller::applyBookMarks,
                       [=](const QList<BookMark> &books) {
                         plgsys->resetTimeout(
                             qobject_cast<IWingPlugin *>(sender()));
                         PCHECK(
                             {
                               QList<BookMarkStruct> bs;
                               for (auto &item : books) {
                                 BookMarkStruct b;
                                 b.pos = item.pos;
                                 b.comment = item.comment;
                                 bs.append(b);
                               }
                               hexfiles[_pcurfile].doc->applyBookMarks(bs);
                             },
                             {
                               QList<BookMarkStruct> bs;
                               for (auto &item : books) {
                                 BookMarkStruct b;
                                 b.pos = item.pos;
                                 b.comment = item.comment;
                                 bs.append(b);
                               }
                               hexeditor->document()->applyBookMarks(bs);
                             }, );
                       });

  ConnectControlLamba2(
      WingPlugin::Controller::openWorkSpace, [=](const QString &filename) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        return openWorkSpace(filename);
      });
  ConnectControlLamba2(WingPlugin::Controller::newFile, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    newFile();
  });
  ConnectControlLamba2(WingPlugin::Controller::openFile,
                       [=](const QString &filename, int *openedIndex) {
                         plgsys->resetTimeout(
                             qobject_cast<IWingPlugin *>(sender()));
                         return openFile(filename, openedIndex);
                       });
  ConnectControlLamba2(
      WingPlugin::Controller::openRegionFile,
      [=](const QString &filename, int *openedIndex, qint64 start,
          qint64 length) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        return openRegionFile(filename, openedIndex, start, length);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::openDriver, [=](const QString &driver) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        return openDriver(driver);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::closeFile, [=](int index, bool force) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        return closeFile(index, force);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::saveFile, [=](int index, bool ignoreMd5) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        return save(index, ignoreMd5);
      });
  ConnectControlLamba2(WingPlugin::Controller::exportFile,
                       [=](const QString &filename, int index, bool ignoreMd5) {
                         plgsys->resetTimeout(
                             qobject_cast<IWingPlugin *>(sender()));
                         return exportFile(filename, index, ignoreMd5);
                       });
  ConnectControlLamba2(WingPlugin::Controller::exportFileGUI, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    on_exportfile();
  });
  ConnectControlLamba2(WingPlugin::Controller::saveasFile,
                       [=](const QString &filename, int index, bool ignoreMd5) {
                         plgsys->resetTimeout(
                             qobject_cast<IWingPlugin *>(sender()));
                         return saveAs(filename, index, ignoreMd5);
                       });
  ConnectControlLamba2(WingPlugin::Controller::saveasFileGUI, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    on_saveas();
  });
  ConnectControlLamba2(
      WingPlugin::Controller::closeCurrentFile, [=](bool force) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        return closeFile(_pcurfile, force);
      });
  ConnectControlLamba2(
      WingPlugin::Controller::saveCurrentFile, [=](bool ignoreMd5) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        return save(_pcurfile, ignoreMd5);
      });
  ConnectControlLamba2(WingPlugin::Controller::openFileGUI, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    on_openfile();
  });
  ConnectControlLamba2(
      WingPlugin::Controller::openRegionFileGUI,
      [=](const QString &filename, qint64 start, qint64 length) {
        plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
        OpenRegionDialog d(lastusedpath, filename, int(start), int(length));
        if (d.exec()) {
          auto res = d.getResult();
          int index;
          auto ret =
              openRegionFile(res.filename, &index, res.start, res.length);
          if (ret == ErrFile::NotExist) {
            QMessageBox::critical(this, tr("Error"), tr("FileNotExist"));
            return;
          }
          if (ret == ErrFile::Permission) {
            QMessageBox::critical(this, tr("Error"), tr("FilePermission"));
            return;
          }
          if (ret == ErrFile::AlreadyOpened) {
            setFilePage(index);
            return;
          }
        }
      });
  ConnectControlLamba2(WingPlugin::Controller::openDriverGUI, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    on_opendriver();
  });
  ConnectControlLamba2(WingPlugin::Controller::gotoGUI, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    on_gotoline();
  });
  ConnectControlLamba2(WingPlugin::Controller::findGUI, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    on_findfile();
  });
  ConnectControlLamba2(WingPlugin::Controller::fillGUI, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    on_fill();
  });
  ConnectControlLamba2(WingPlugin::Controller::fillzeroGUI, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    on_fillzero();
  });
  ConnectControlLamba2(WingPlugin::Controller::fillnopGUI, [=] {
    plgsys->resetTimeout(qobject_cast<IWingPlugin *>(sender()));
    on_fillnop();
  });
}

bool MainWindow::requestControl(int timeout) {
  auto s = qobject_cast<IWingPlugin *>(sender());
  auto res = plgsys->requestControl(s, timeout);
  _pcurfile = _currentfile;
  return res;
}

bool MainWindow::hasControl() {
  auto s = qobject_cast<IWingPlugin *>(sender());
  return plgsys->currentControlPlugin() == s;
}

bool MainWindow::requestRelease() {
  auto s = qobject_cast<IWingPlugin *>(sender());
  return plgsys->requestRelease(s);
}

void MainWindow::setTheme(DGuiApplicationHelper::ColorType theme) {
  Q_UNUSED(theme);
}

void MainWindow::on_hexeditor_customContextMenuRequested(const QPoint &pos) {
  Q_UNUSED(pos)
  hexeditorMenu->popup(QCursor::pos());
}

void MainWindow::on_tabs_currentChanged(int index) { setFilePage(index); }

void MainWindow::on_tabMoved(int from, int to) { hexfiles.move(from, to); }

bool MainWindow::setFilePage(int index) {
  if (index < 0 && hexfiles.count() == 0) {
    _currentfile = -1;
    _pcurfile = -1;
    return false;
  }
  if (index >= 0 && index < hexfiles.count()) {
    if (_currentfile >= 0 && _currentfile < hexfiles.count()) {
      auto s = hexeditor->verticalScrollBar()->value();
      hexfiles[_currentfile].vBarValue = s;
    }
    _currentfile = index;
    auto d = hexfiles.at(index);
    if (d.doc == hexeditor->document())
      return true;
    hexeditor->switchDocument(d.doc, d.render, d.vBarValue);
    enableDirverLimit(d.isdriver);
    tabs->setCurrentIndex(index);

    if (_enableplugin) {
      QList<QVariant> param;
      param << d.filename << index;
      plgsys->raiseDispatch(HookIndex::DocumentSwitched, param);
    }

    return true;
  } else {
    return false;
  }
}

QString MainWindow::saveLog() {
  QDir ndir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
  ndir.mkdir("log"); // 确保日志存放目录存在

  QFile lfile(ndir.absolutePath() + "/log/" +
              QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz") +
              ".log");
  if (lfile.open(QFile::WriteOnly)) {
    lfile.write(pluginInfo->toPlainText().toUtf8());
    lfile.close();
    return lfile.fileName();
  }
  return QString();
}

void MainWindow::on_newfile() { newFile(); }
void MainWindow::on_newbigfile() { newFile(true); }

void MainWindow::newFile(bool bigfile) {
  QList<QVariant> params;
  QString title = tr("Untitled") + QString("-%1").arg(defaultindex);
  if (_enableplugin) {
    params << title;
    plgsys->raiseDispatch(HookIndex::NewFileBegin, params);
  }

  hexeditor->setVisible(true);

  auto p = bigfile ? QHexDocument::fromLargeFile(nullptr)
                   : QHexDocument::fromFile<QMemoryBuffer>(nullptr);
  p->setCopyLimit(_cplim);
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
  tabs->addTab(this->style()->standardIcon(QStyle::SP_FileIcon), title);
  defaultindex++;
  auto curindex = hexfiles.count() - 1;
  tabs->setCurrentIndex(curindex);
  tabs->setTabToolTip(curindex, title);
  setEditModeEnabled(true);
  p->setDocSaved();
  hexeditor->getStatus();
  if (_enableplugin) {
    plgsys->raiseDispatch(HookIndex::NewFileEnd, params);
  }
}

ErrFile MainWindow::openRegionFile(QString filename, int *openedindex,
                                   qint64 start, qint64 length) {
  QList<QVariant> params;
  if (_enableplugin) {
    params << filename;
    plgsys->raiseDispatch(HookIndex::OpenFileBegin, params);
  }
  QFileInfo info(filename);
  if (info.exists()) {
    if (!info.permission(QFile::ReadUser)) {
      if (_enableplugin) {
        params << ErrFile::Permission;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      return ErrFile::Permission;
    }

    int i = 0;
    for (auto &item : hexfiles) {
      if (item.filename == filename) {
        if (openedindex) {
          *openedindex = i;
        }
        if (_enableplugin) {
          params << ErrFile::AlreadyOpened;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::AlreadyOpened;
      }
      i++;
    }

    auto readonly = Utilities::fileCanWrite(filename);
    HexFile hf;
    auto *p =
        QHexDocument::fromRegionFile(filename, start, length, readonly, this);

    if (p == nullptr) {
      if (_enableplugin) {
        params << ErrFile::Permission;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      return ErrFile::Permission;
    }

    hexeditor->setVisible(true);
    p->setCopyLimit(_cplim);
    hf.doc = p;
    hf.filename = filename;
    hf.workspace.clear();
    hf.md5 = Utilities::getMd5(filename);
    hf.vBarValue = -1;
    p->setDocumentType(DocumentType::RegionFile);
    hf.isdriver = false;
    hexeditor->setDocument(p);
    hexeditor->setLockedFile(readonly);
    hexeditor->setKeepSize(true);
    hexeditor->renderer()->setEncoding(_encoding);
    hf.render = hexeditor->renderer();
    hexfiles.push_back(hf);
    p->setDocSaved();
    hexeditor->getStatus();

    tabs->addTab(Utilities::getIconFromFile(style(), filename),
                 info.fileName());
    auto index = hexfiles.count() - 1;
    tabs->setCurrentIndex(index);
    tabs->setTabToolTip(index, filename);
    setEditModeEnabled(true);
    _currentfile = index;

    if (_enableplugin) {
      params << ErrFile::Success;
      plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
    }
    return ErrFile::Success;
  }

  if (_enableplugin) {
    params << ErrFile::NotExist;
    plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
  }
  return ErrFile::NotExist;
}

ErrFile MainWindow::openFile(QString filename, int *openedindex,
                             QString workspace, bool *oldworkspace) {
  QList<QVariant> params;
  if (_enableplugin) {
    params << filename;
    plgsys->raiseDispatch(HookIndex::OpenFileBegin, params);
  }
  QFileInfo info(filename);
  if (info.exists()) {

    if (!info.permission(QFile::ReadUser)) {
      if (_enableplugin) {
        params << ErrFile::Permission;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      return ErrFile::Permission;
    }

    int i = 0;
    for (auto &item : hexfiles) {
      if (item.filename == filename) {
        if (oldworkspace) {
          *oldworkspace = item.workspace.length() > 0;
        }
        if (openedindex) {
          *openedindex = i;
        }
        if (_enableplugin) {
          params << ErrFile::AlreadyOpened;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::AlreadyOpened;
      }
      i++;
    }

    HexFile hf;
    auto readonly = !Utilities::fileCanWrite(filename);
    auto *p =
        info.size() > FILEMAXBUFFER
            ? QHexDocument::fromLargeFile(filename, readonly, this)
            : QHexDocument::fromFile<QMemoryBuffer>(filename, readonly, this);

    if (p == nullptr) {
      if (_enableplugin) {
        params << ErrFile::Permission;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      return ErrFile::Permission;
    }

    hexeditor->setVisible(true);
    p->setCopyLimit(_cplim);
    hf.doc = p;
    hf.filename = filename;
    hf.workspace = workspace;
    hf.vBarValue = -1;
    p->setDocumentType(workspace.length() > 0 ? DocumentType::WorkSpace
                                              : DocumentType::File);
    hf.isdriver = false;
    hexeditor->setDocument(p);
    hexeditor->setLockedFile(readonly);
    hexeditor->setKeepSize(true);
    hexeditor->renderer()->setEncoding(_encoding);
    hf.render = hexeditor->renderer();
    hexfiles.push_back(hf);
    p->setDocSaved();
    hexeditor->getStatus();

    QIcon qicon;

    if (p->documentType() == DocumentType::WorkSpace) {
      qicon = ICONRES("pro");
    } else {
      qicon = Utilities::getIconFromFile(style(), filename);
    }

    tabs->addTab(qicon, info.fileName());
    auto index = hexfiles.count() - 1;
    tabs->setCurrentIndex(index);
    tabs->setTabToolTip(index, filename);
    setEditModeEnabled(true);
    _currentfile = index;

    recentmanager->addRecentFile(filename);

    if (_enableplugin) {
      params << ErrFile::Success;
      plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
    }

    return ErrFile::Success;
  }

  if (_enableplugin) {
    params << ErrFile::NotExist;
    plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
  }
  return ErrFile::NotExist;
}

ErrFile MainWindow::openDriver(QString driver) {
  QList<QVariant> params;
  if (_enableplugin) {
    params << driver;
    plgsys->raiseDispatch(HookIndex::OpenDriverBegin, params);
  }

  if (Utilities::isRoot()) {
    QFileInfo info(driver);

    if (info.exists()) {
      if (!info.permission(QFile::ReadUser)) {
        if (_enableplugin) {
          params << ErrFile::Permission;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::Permission;
      }

      if (!info.permission(QFile::WriteUser)) {
        if (_enableplugin) {
          params << ErrFile::Permission;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::Permission;
      }

      for (auto &item : hexfiles) {
        if (item.filename == driver) {
          if (_enableplugin) {
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
          params << ErrFile::Error;
          plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
        }
        return ErrFile::Error;
      }

      p->setCopyLimit(_cplim);
      hf.doc = p;
      hexeditor->setDocument(p);
      hexeditor->setKeepSize(true);
      hf.isdriver = true;
      hf.render = hexeditor->renderer();
      hf.vBarValue = -1;
      hf.filename = driver;
      hexfiles.push_back(hf);
      p->setDocSaved();
      hexeditor->getStatus();
      tabs->addTab(Utilities::getIconFromFile(style(), driver),
                   info.fileName());
      auto index = hexfiles.count() - 1;
      tabs->setCurrentIndex(index);
      tabs->setTabToolTip(index, driver);
      setEditModeEnabled(true);

      if (_enableplugin) {
        params << ErrFile::Success;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }

      hexeditor->setLockedFile(true);
      setEditModeEnabled(true, true);
      if (_enableplugin) {
        params << ErrFile::Success;
        plgsys->raiseDispatch(HookIndex::OpenDriverEnd, params);
      }
      return ErrFile::Success;
    }

    if (_enableplugin) {
      params << ErrFile::NotExist;
      plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
    }
    return ErrFile::NotExist;
  } else {
    QMessageBox::critical(this, tr("Error"), tr("NoRoot"));
    if (_enableplugin) {
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
    params << index << force;
    plgsys->raiseDispatch(HookIndex::CloseFileBegin, params);
  }

  if (index >= 0 && index < hexfiles.count()) {
    auto p = hexfiles.at(index);
    if (!force) {
      if (!isSavedFile(index)) {
        if (_enableplugin) {
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
  if (hexfiles.count() == 0) {
    setEditModeEnabled(false);
    txtDecode->clear();
    gotobar->setVisible(false);
  }
  if (_enableplugin) {
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
    auto res = openFile(filename, &index);
    if (res == ErrFile::NotExist) {
      QMessageBox::critical(this, tr("Error"), tr("FileNotExist"));
      return;
    }
    if (res == ErrFile::Permission) {
      QMessageBox::critical(this, tr("Error"), tr("FilePermission"));
      return;
    }
    if (res == ErrFile::AlreadyOpened) {
      setFilePage(index);
      return;
    }
  }
}

void MainWindow::on_openregion() {
  OpenRegionDialog d(lastusedpath);
  if (d.exec()) {
    auto res = d.getResult();
    int index;
    auto ret = openRegionFile(res.filename, &index, res.start, res.length);
    if (ret == ErrFile::NotExist) {
      QMessageBox::critical(this, tr("Error"), tr("FileNotExist"));
      return;
    }
    if (ret == ErrFile::Permission) {
      QMessageBox::critical(this, tr("Error"), tr("FilePermission"));
      return;
    }
    if (ret == ErrFile::AlreadyOpened) {
      setFilePage(index);
      return;
    }
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

void MainWindow::on_tabBarDoubleClicked(int index) {
  auto &h = hexfiles[index];
  FileInfoDialog d(h.filename, Utilities::isRegionFile(h.doc));
  d.exec();
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
  if (hexeditor->document()->copy())
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
  auto res = exportFile(filename, _currentfile);

restart:
  switch (res) {
  case ErrFile::Success: {
    DMessageManager::instance()->sendMessage(this, ICONRES("export"),
                                             tr("ExportSuccessfully"));
    break;
  }
  case ErrFile::SourceFileChanged: {
    if (QMessageBox::warning(this, tr("Warn"), tr("SourceChanged"),
                             QMessageBox::Yes | QMessageBox::No) ==
        QMessageBox::Yes) {
      res = exportFile(filename, _currentfile, true);
      goto restart;
    } else {
      DMessageManager::instance()->sendMessage(this, ICONRES("export"),
                                               tr("ExportSourceFileError"));
    }
    break;
  }
  default: {
    DMessageManager::instance()->sendMessage(this, ICONRES("export"),
                                             tr("ExportUnSuccessfully"));
    break;
  }
  }
}

void MainWindow::on_exit() { close(); }

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
  else
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event) {
  const QMimeData *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    QStringList files;
    for (auto &item : mimeData->urls()) {
      if (!item.isEmpty())
        files << item.toLocalFile();
    }
    AppManager::openFiles(files);
  }
}

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
      } else {
        event->ignore();
        return;
      }
    }
  }
  m_settings->saveWindowState(this);
  m_settings->saveFileDialogCurrent(lastusedpath);
  event->accept();
  QApplication::exit();
}

void MainWindow::on_save() {
  CheckEnabled;
  auto res = saveCurrent();

restart:

  switch (res) {
  case ErrFile::IsNewFile: {
    on_saveas();
    break;
  }
  case ErrFile::Success: {
    DMessageManager::instance()->sendMessage(this, ICONRES("save"),
                                             tr("SaveSuccessfully"));
    break;
  }
  case ErrFile::WorkSpaceUnSaved: {
    DMessageManager::instance()->sendMessage(this, ICONRES("save"),
                                             tr("SaveWSError"));
    break;
  }
  case ErrFile::SourceFileChanged: {
    if (QMessageBox::warning(this, tr("Warn"), tr("SourceChanged"),
                             QMessageBox::Yes | QMessageBox::No) ==
        QMessageBox::Yes) {
      res = save(_currentfile, true);
      goto restart;
    } else {
      DMessageManager::instance()->sendMessage(this, ICONRES("save"),
                                               tr("SaveSourceFileError"));
    }
    break;
  }
  default: {
    DMessageManager::instance()->sendMessage(this, ICONRES("save"),
                                             tr("SaveUnSuccessfully"));
    break;
  }
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

restart:
  switch (res) {
  case ErrFile::Success: {
    DMessageManager::instance()->sendMessage(this, ICONRES("saveas"),
                                             tr("SaveSuccessfully"));
    break;
  }
  case ErrFile::WorkSpaceUnSaved: {
    DMessageManager::instance()->sendMessage(this, ICONRES("saveas"),
                                             tr("SaveWSError"));
    break;
  }
  case ErrFile::SourceFileChanged: {
    if (QMessageBox::warning(this, tr("Warn"), tr("SourceChanged"),
                             QMessageBox::Yes | QMessageBox::No) ==
        QMessageBox::Yes) {
      res = saveAs(filename, _currentfile, true);
      goto restart;
    } else {
      DMessageManager::instance()->sendMessage(this, ICONRES("save"),
                                               tr("SaveSourceFileError"));
    }
    break;
  }
  default: {
    DMessageManager::instance()->sendMessage(this, ICONRES("saveas"),
                                             tr("SaveUnSuccessfully"));
    break;
  }
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
  hexeditor->renderer()->enableCursor();
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
  lblsellen->setText(QString("%1 - 0x%2")
                         .arg(sellen)
                         .arg(QString::number(sellen, 16).toUpper()));

  // number analyse
  auto off = qint64(hexeditor->currentOffset());
  auto d = hexeditor->document();

  auto tmp = d->read(off, sizeof(quint64));
  quint64 n = *reinterpret_cast<const quint64 *>(tmp.constData());

  auto len = tmp.length();

  if (len == sizeof(quint64)) {
    auto s = processEndian(n);
    numsitem[NumTableIndex::Uint64].setText(
        QString("0x%1").arg(QString::number(s, 16).toUpper()));
    auto s1 = processEndian(qint64(n));
    numsitem[NumTableIndex::Int64].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Uint64].setText("-");
    numsitem[NumTableIndex::Int64].setText("-");
  }

  if (len > int(sizeof(quint32))) {
    auto s = processEndian(quint32(n));
    numsitem[NumTableIndex::Uint32].setText(
        QString("0x%1").arg(QString::number(s, 16).toUpper()));
    auto s1 = processEndian(qint32(n));
    numsitem[NumTableIndex::Int32].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Uint32].setText("-");
    numsitem[NumTableIndex::Int32].setText("-");
  }

  if (len > int(sizeof(quint16))) {
    auto s = processEndian(quint16(n));
    numsitem[NumTableIndex::Ushort].setText(
        QString("0x%1").arg(QString::number(s, 16).toUpper()));
    auto s1 = processEndian(qint16(n));
    numsitem[NumTableIndex::Short].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Ushort].setText("-");
    numsitem[NumTableIndex::Short].setText("-");
  }
  if (len > int(sizeof(uchar))) {
    auto s1 = tmp.at(0);
    auto s = uchar(s1);
    numsitem[NumTableIndex::Byte].setText(
        QString("0x%1").arg(QString::number(s, 16).toUpper()));
    numsitem[NumTableIndex::Char].setText(QString::number(s1));
  } else {
    numsitem[NumTableIndex::Byte].setText("-");
    numsitem[NumTableIndex::Char].setText("-");
  }

  //解码字符串
  if (sellen > 1) {
    // 如果不超过 10KB 那么解码，防止太多卡死
    if (sellen <= ulong(1024 * _decstrlim)) {
      auto enc =
          QTextCodec::codecForName(hexeditor->renderer()->encoding().toUtf8());
      auto dec = enc->makeDecoder();
      txtDecode->setText(dec->toUnicode(d->selectedBytes()));
    } else {
      txtDecode->setHtml(QString("<font color=\"red\">%1</font>")
                             .arg(tr("TooManyBytesDecode")));
    }
  } else {
    txtDecode->clear();
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

void MainWindow::on_reload() {
  CheckEnabled;
  auto doc = hexeditor->document();
  QList<QVariant> params;
  auto &hf = hexfiles[_currentfile];
  auto filename = hf.filename;
  auto readonly = doc->isReadOnly();
  if (_enableplugin) {
    params << filename << readonly;
    plgsys->raiseDispatch(HookIndex::OpenFileBegin, params);
  }

  QFileInfo info(filename);
  if (info.exists()) {
    if (!info.permission(QFile::ReadUser)) {
      if (_enableplugin) {
        params << ErrFile::Permission;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      return;
    }

    if (!readonly && !info.permission(QFile::WriteUser)) {
      if (_enableplugin) {
        params << ErrFile::Permission;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      return;
    }

    auto *p =
        info.size() > FILEMAXBUFFER
            ? QHexDocument::fromLargeFile(filename, readonly, this)
            : QHexDocument::fromFile<QMemoryBuffer>(filename, readonly, this);

    if (p == nullptr) {
      if (_enableplugin) {
        params << ErrFile::Error;
        plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
      }
      DMessageManager::instance()->sendMessage(this, ICONRES("reload"),
                                               tr("ReloadFileDocError"));
      return;
    }

    hf.doc->deleteLater();
    p->setCopyLimit(_cplim);
    hf.doc = p;
    hf.render->switchDoc(p);
    hexeditor->switchDocument(p, hf.render, hf.vBarValue);

    p->setDocSaved();
    hexeditor->getStatus();

    if (_enableplugin) {
      params << ErrFile::Success;
      plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
    }
    DMessageManager::instance()->sendMessage(this, ICONRES("reload"),
                                             tr("ReloadSuccess"));
    return;
  } else {
    DMessageManager::instance()->sendMessage(this, ICONRES("reload"),
                                             tr("ReloadFileNotExist"));
  }

  if (_enableplugin) {
    params << ErrFile::NotExist;
    plgsys->raiseDispatch(HookIndex::OpenFileEnd, params);
  }
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

void MainWindow::on_bookmarkChanged(BookMarkModEnum flag, int index, qint64 pos,
                                    QString comment) {
  auto doc = hexeditor->document();
  switch (flag) {
  case BookMarkModEnum::Insert: {
    QListWidgetItem *litem = new QListWidgetItem;
    litem->setIcon(ICONRES("bookmark"));
    litem->setData(Qt::UserRole, pos);
    litem->setText(comment);
    litem->setToolTip(QString(tr("Addr : 0x%1")).arg(pos, 0, 16));
    bookmarks->addItem(litem);
  } break;
  case BookMarkModEnum::Modify: {
    bookmarks->item(index)->setText(comment);
  } break;
  case BookMarkModEnum::Remove: {
    auto item = bookmarks->item(index);
    bookmarks->removeItemWidget(item);
    delete item; // let item disappear
  } break;
  case BookMarkModEnum::Apply: {
    QList<BookMarkStruct> bookmaps;
    bookmarks->clear();
    doc->getBookMarks(bookmaps);
    for (auto &item : bookmaps) {
      QListWidgetItem *litem = new QListWidgetItem;
      litem->setIcon(ICONRES("bookmark"));
      litem->setData(Qt::UserRole, item.pos);
      litem->setText(item.comment);
      litem->setToolTip(QString(tr("Addr : 0x%1")).arg(item.pos, 0, 16));
      bookmarks->addItem(litem);
    }
  } break;
  case BookMarkModEnum::Clear: {
    bookmarks->clear();
  } break;
  }
}

void MainWindow::on_documentSwitched() {
  iReadWrite->setPixmap(hexeditor->isReadOnly() ? infoReadonly : infoWriteable);
  if (hexfiles.count()) {
    iw->setPixmap(hexeditor->document()->documentType() ==
                          DocumentType::WorkSpace
                      ? infow
                      : infouw);
  } else {
    iw->setPixmap(infouw);
  }
  if (gotobar->isVisible()) {
    gotobar->clearInput();
  }
}

ErrFile MainWindow::save(int index, bool ignoreMd5) {
  if (index >= 0 && index < hexfiles.count()) {
    auto f = hexfiles.at(index);
    if (f.isdriver)
      return ErrFile::IsDirver;
    if (f.filename.at(0) == ':')
      return ErrFile::IsNewFile;

    QFile file(f.filename);

    if (f.doc->documentType() == DocumentType::RegionFile) {
      if (!ignoreMd5 && Utilities::getMd5(f.filename) != f.md5) {
        return ErrFile::SourceFileChanged;
      }
      if (!file.open(QFile::ReadWrite)) {
        return ErrFile::Permission;
      }
    } else {
      if (!file.open(QFile::WriteOnly)) {
        return ErrFile::Permission;
      }
    }

    if (f.doc->saveTo(&file, true)) {
      file.close();
      if (f.doc->metadata()->hasMetadata()) {
        auto w = f.workspace;
        if (QFile::exists(w)) {
          auto doc = hexeditor->document();
          auto render = hexeditor->renderer();

          SaveWorkSpaceInitInfo;
          auto b = WorkSpaceManager::saveWorkSpace(
              w, f.filename, doc->getAllBookMarks(),
              doc->metadata()->getallMetas(), infos);
          if (!b)
            return ErrFile::WorkSpaceUnSaved;
          f.doc->setDocumentType(DocumentType::WorkSpace);
          iw->setPixmap(infow);
          tabs->setTabIcon(index, ICONRES("pro"));
          f.doc->setDocSaved();
        } else {
          auto doc = hexeditor->document();
          auto render = hexeditor->renderer();

          SaveWorkSpaceInitInfo;
          auto b = WorkSpaceManager::saveWorkSpace(
              f.filename + PROEXT, f.filename, doc->getAllBookMarks(),
              doc->metadata()->getallMetas(), infos);
          if (!b)
            return ErrFile::WorkSpaceUnSaved;
          hexfiles[index].workspace = f.filename + PROEXT;
          f.doc->setDocumentType(DocumentType::WorkSpace);
          iw->setPixmap(infow);
          tabs->setTabIcon(index, ICONRES("pro"));
          f.doc->setDocSaved();
        }
      } else {
        // 如果不是工作区，更新文件关联的图标
        tabs->setTabIcon(index,
                         Utilities::getIconFromFile(style(), f.filename));
      }
      return ErrFile::Success;
    }
    return ErrFile::Permission;
  }
  return ErrFile::Error;
}

ErrFile MainWindow::exportFile(QString filename, int index, bool ignoreMd5) {
  if (index >= 0 && index < hexfiles.count()) {
    auto f = hexfiles.at(index);
    if (f.isdriver)
      return ErrFile::IsDirver;
    QFile file(filename);

    // 如果是局部文件就拷贝一份
    if (f.doc->documentType() == DocumentType::RegionFile) {
      if (!ignoreMd5 && Utilities::getMd5(f.filename) != f.md5) {
        return ErrFile::SourceFileChanged;
      }
      if (!QFile::copy(f.filename, filename)) {
        return ErrFile::Error;
      }
      if (!file.open(QFile::ReadWrite)) {
        return ErrFile::Permission;
      }
    } else {
      if (!file.open(QFile::WriteOnly)) {
        return ErrFile::Permission;
      }
    }

    if (f.doc->saveTo(&file, false)) {
      file.close();
      if (f.doc->metadata()->hasMetadata()) {
        auto doc = hexeditor->document();
        auto render = hexeditor->renderer();
        SaveWorkSpaceInitInfo;
        auto b = WorkSpaceManager::saveWorkSpace(
            filename + PROEXT, filename, doc->getAllBookMarks(),
            doc->metadata()->getallMetas(), infos);
        if (!b)
          return ErrFile::WorkSpaceUnSaved;
      }
      return ErrFile::Success;
    }
    file.close();
    return ErrFile::Permission;
  }
  return ErrFile::Error;
}

ErrFile MainWindow::saveAs(QString filename, int index, bool ignoreMd5) {
  if (index >= 0 && index < hexfiles.count()) {
    auto f = hexfiles.at(index);
    if (f.isdriver)
      return ErrFile::IsDirver;

    QFile file(filename);

    // 如果是局部文件就拷贝一份
    if (f.doc->documentType() == DocumentType::RegionFile) {
      if (!ignoreMd5 && Utilities::getMd5(f.filename) != f.md5) {
        return ErrFile::SourceFileChanged;
      }
      if (!QFile::copy(f.filename, filename)) {
        return ErrFile::Error;
      }
      if (!file.open(QFile::ReadWrite)) {
        return ErrFile::Permission;
      }
    } else {
      if (!file.open(QFile::WriteOnly)) {
        return ErrFile::Permission;
      }
    }

    if (f.doc->saveTo(&file, true)) {
      hexfiles[index].filename = filename;
      tabs->setTabText(index, QFileInfo(file).fileName());
      tabs->setTabToolTip(index, filename);
      file.close();
      if (f.doc->metadata()->hasMetadata()) {
        auto doc = hexeditor->document();
        auto render = hexeditor->renderer();

        SaveWorkSpaceInitInfo;
        auto b = WorkSpaceManager::saveWorkSpace(
            filename + PROEXT, filename, doc->getAllBookMarks(),
            doc->metadata()->getallMetas(), infos);
        if (!b)
          return ErrFile::WorkSpaceUnSaved;
        hexfiles[index].workspace = filename + PROEXT;
        f.doc->setDocumentType(DocumentType::WorkSpace);
        iw->setPixmap(infow);
        tabs->setTabIcon(index, ICONRES("pro"));
        f.doc->setDocSaved();
      } else {
        // 如果不是工作区，更新文件关联的图标
        tabs->setTabIcon(index, Utilities::getIconFromFile(style(), filename));
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
  auto doc = hexeditor->document();
  if (!doc->isKeepSize()) {
    DMessageManager::instance()->sendMessage(this, ICONRES("metadataedit"),
                                             tr("CheckKeepSize"));
    return;
  }
  if (hexeditor->documentBytes() > 0) {
    MetaDialog m;
    auto cur = doc->cursor();
    if (cur->selectionLength() > 0) {
      auto mc = doc->metadata()->gets(cur->position().offset());

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
  auto doc = hexeditor->document();
  if (!doc->isKeepSize()) {
    DMessageManager::instance()->sendMessage(this, ICONRES("metadata"),
                                             tr("CheckKeepSize"));
    return;
  }
  if (hexeditor->documentBytes() > 0) {
    MetaDialog m;
    auto cur = doc->cursor();

    if (cur->selectionLength() > 0) {
      auto begin = qint64(cur->selectionStart().offset());
      auto end = qint64(cur->selectionEnd().offset()) + 1;
      if (m.exec()) {
        doc->metadata()->Metadata(begin, end, m.foreGroundColor(),
                                  m.backGroundColor(), m.comment());
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
  if (!doc->isKeepSize()) {
    DMessageManager::instance()->sendMessage(this, ICONRES("metadatadel"),
                                             tr("CheckKeepSize"));
    return;
  }
  auto meta = doc->metadata();
  auto pos = doc->cursor()->position().offset();
  meta->RemoveMetadata(pos);
}

void MainWindow::on_metadatacls() {
  CheckEnabled;
  auto doc = hexeditor->document();
  if (!doc->isKeepSize()) {
    DMessageManager::instance()->sendMessage(this, ICONRES("metadatacls"),
                                             tr("CheckKeepSize"));
    return;
  }
  doc->metadata()->Clear();
}

void MainWindow::on_metashowall() {
  CheckEnabled;
  hexeditor->document()->SetMetaVisible(true);
}

void MainWindow::on_metastatusChanged() {
  auto doc = hexeditor->document();
  if (hexfiles.count() && (!doc->metabgVisible() || !doc->metafgVisible() ||
                           !doc->metaCommentVisible())) {
    toolbtnstools[ToolBoxIndex::Meta]->setIcon(iconmetah);
  } else {
    toolbtnstools[ToolBoxIndex::Meta]->setIcon(iconmetas);
  }
}

void MainWindow::on_metahideall() {
  CheckEnabled;
  hexeditor->document()->SetMetaVisible(false);
}

void MainWindow::on_setting_plugin() {
  if (!_enableplugin)
    return;
  PluginWindow pw(this);
  pw.setPluginSystem(plgsys);
  pw.exec();
}

void MainWindow::on_fullScreen() { showFullScreen(); }

void MainWindow::on_bookmark() {
  CheckEnabled;
  auto doc = hexeditor->document();
  if (!doc->isKeepSize()) {
    DMessageManager::instance()->sendMessage(this, ICONRES("bookmark"),
                                             tr("CheckKeepSize"));
    return;
  }
  int index = -1;
  if (doc->existBookMark(index)) {
    auto b = doc->bookMark(index);
    bool ok;
    hexeditor->renderer()->enableCursor();
    auto comment =
        QInputDialog::getText(this, tr("BookMark"), tr("InputComment"),
                              QLineEdit::Normal, b.comment, &ok);
    if (ok) {
      doc->ModBookMark(b.pos, comment);
    }
  } else {
    bool ok;
    auto comment =
        QInputDialog ::getText(this, tr("BookMark"), tr("InputComment"),
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
  if (!doc->isKeepSize()) {
    DMessageManager::instance()->sendMessage(this, ICONRES("bookmarkdel"),
                                             tr("CheckKeepSize"));
    return;
  }
  int index = -1;
  if (doc->existBookMark(index)) {
    doc->RemoveBookMark(index);
  }
}

void MainWindow::on_bookmarkcls() {
  CheckEnabled;
  auto doc = hexeditor->document();
  if (!doc->isKeepSize()) {
    DMessageManager::instance()->sendMessage(this, ICONRES("bookmarkcls"),
                                             tr("CheckKeepSize"));
    return;
  }
  doc->ClearBookMark();
}

void MainWindow::on_encoding() {
  CheckEnabled;
  EncodingDialog d;
  if (d.exec()) {
    auto res = d.getResult();
    hexeditor->renderer()->SetEncoding(res);
  }
}

void MainWindow::on_fileInfo() {
  CheckEnabled;
  auto &h = hexfiles[_currentfile];
  FileInfoDialog d(h.filename, Utilities::isRegionFile(h.doc));
  d.exec();
}

void MainWindow::setEditModeEnabled(bool b, bool isdriver) {
  for (auto &item : toolbartools) {
    item->setEnabled(b);
  }
  hexeditorMenu->setEnabled(b);
  for (auto &item : toolmenutools) {
    item->setEnabled(b);
  }
  for (auto &item : toolbtnstools) {
    item->setEnabled(b);
  }

  if (b) {
    enableDirverLimit(isdriver);
    auto dm = hexeditor->document()->documentType() == DocumentType::WorkSpace;
    iw->setPixmap(dm ? infow : infouw);
    auto doc = hexeditor->document();
    emit doc->canRedoChanged(doc->canRedo());
    emit doc->canUndoChanged(doc->canUndo());
  } else {
    iw->setPixmap(infouw);
  }
  status->setEnabled(b);

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

void MainWindow::on_metadatabg(bool b) {
  CheckEnabled;
  hexeditor->document()->SetMetabgVisible(b);
}

void MainWindow::on_metadatafg(bool b) {
  CheckEnabled;
  hexeditor->document()->SetMetafgVisible(b);
}

void MainWindow::on_metadatacomment(bool b) {
  CheckEnabled;
  hexeditor->document()->SetMetaCommentVisible(b);
}

void MainWindow::on_exportlog() {
  auto nfile = saveLog();
  if (nfile.isEmpty()) {
    DMessageManager::instance()->sendMessage(this, ICONRES("log"),
                                             tr("ExportLogError"));
  } else {
    DMessageManager::instance()->sendMessage(this, ICONRES("log"),
                                             tr("ExportLogSuccess") + nfile);
  }
}

void MainWindow::on_clslog() {
  pluginInfo->clear();
  DMessageManager::instance()->sendMessage(this, ICONRES("clearhis"),
                                           tr("ClearLogSuccess"));
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
  auto in = QInputDialog::getText(this, tr("Fill"), tr("PleaseInputFill"),
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
#ifdef QT_DEBUG
  auto filename = QFileDialog::getOpenFileName(
      this, tr("ChoosePlugin"), lastusedpath, tr("PluginFile (*.so)"));
#else
  auto filename = QFileDialog::getOpenFileName(
      this, tr("ChoosePlugin"), lastusedpath, tr("PluginFile (*.wingplg)"));
#endif

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

#ifndef WITHOUTLICENSEINFO
void MainWindow::on_license() {
  LincenseDialog d;
  d.exec();
}
#endif

void MainWindow::on_about() {
  AboutSoftwareDialog d;
  d.exec();
}

ErrFile MainWindow::openWorkSpace(QString filename, int *openedindex) {
  QString file;
  QList<BookMarkStruct> bookmarks;
  QList<QHexMetadataAbsoluteItem> metas;
  auto res = ErrFile::Error;
  WorkSpaceInfo infos;

  if (WorkSpaceManager::loadWorkSpace(filename, file, bookmarks, metas,
                                      infos)) {
    bool b;
    int index;
    res = openFile(file, &index, filename, &b);
    if (res == ErrFile::AlreadyOpened) {
      if (openedindex)
        *openedindex = index;
      return res;
    } else {
      if (res != ErrFile::Success)
        return res;
    }

    auto doc = hexeditor->document();
    auto render = hexeditor->renderer();
    doc->applyBookMarks(bookmarks);
    doc->setBaseAddress(infos.base);
    hexeditor->setLockedFile(infos.locked);
    hexeditor->setKeepSize(infos.keepsize);
    doc->setMetabgVisible(infos.showmetabg);
    doc->setMetafgVisible(infos.showmetafg);
    doc->setMetaCommentVisible(infos.showmetacomment);
    render->setStringVisible(infos.showstr);
    render->setHeaderVisible(infos.showheader);
    render->setAddressVisible(infos.showaddr);
    render->setEncoding(infos.encoding);
    doc->metadata()->applyMetas(metas);
    doc->setDocSaved();
    on_documentSwitched();
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
  auto res = openWorkSpace(filename, &index);
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
