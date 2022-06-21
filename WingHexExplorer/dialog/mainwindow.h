#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "QHexView/document/qhexdocument.h"
#include "QHexView/qhexview.h"
#include "class/logger.h"
#include "class/recentfilemanager.h"
#include "class/workspacemanager.h"
#include "control/gotobar.h"
#include "plugin/pluginsystem.h"
#include "settings.h"
#include "utilities.h"
#include <DApplicationHelper>
#include <DDockWidget>
#include <DIconButton>
#include <DLabel>
#include <DLineEdit>
#include <DMainWindow>
#include <DMenu>
#include <DMenuBar>
#include <DStatusBar>
#include <DTabBar>
#include <DTableWidget>
#include <DToolBar>
#include <DToolButton>
#include <QFile>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QPoint>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow {
  Q_OBJECT

  enum NumTableIndex {
    Byte,
    Char,
    Ushort,
    Short,
    Uint32,
    Int32,
    Uint64,
    Int64,
    NumTableIndexCount
  };

  enum class ToolBoxIndex {
    New,
    OpenFile,
    OpenDriver,
    Save,
    SaveAs,
    SaveSel,
    Export,
    OpenWorkSpace,
    SaveWorkSpace,
    SaveAsWorkSpace,
    Undo,
    Redo,
    Cut,
    CutHex,
    Copy,
    CopyHex,
    Paste,
    PasteHex,
    Del,
    Find,
    Goto,
    Fill,
    FillNop,
    FillZero,
    Meta,
    MetaEdit,
    DelMeta,
    ClsMeta,
    BookMark,
    DelBookMark,
    ClsBookMark,
    Encoding
  };

public:
  MainWindow(DMainWindow *parent = nullptr);
  ~MainWindow() override;

protected:
  void showEvent(QShowEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private slots:
  void on_hexeditor_customContextMenuRequested(const QPoint &pos);
  void on_tabs_currentChanged(int index);

private:
  QWidget *w;
  DTabBar *tabs;
  DToolBar *toolbar;
  QVBoxLayout *vlayout;
  DStatusBar *status;
  DLabel *lblloc;
  DLabel *lblsellen;

  DMenu *hexeditorMenu;
  DMenu *findresultMenu;

  QAction *settingplg;

private:
  QHexView *hexeditor;
  QFile file;
  GotoBar *gotobar;

private:
  void setTheme(DGuiApplicationHelper::ColorType theme);

public:
  ErrFile openFile(QString filename, bool readonly = false,
                   int *openedindex = nullptr, QString workspace = "",
                   bool *oldworkspace = nullptr);
  ErrFile openWorkSpace(QString filename, bool readonly = false,
                        int *openedindex = nullptr);
  void setFilePage(int index);

private:
  void newFile();
  ErrFile openDriver(QString driver);
  ErrFile closeFile(int index, bool force = false);
  ErrFile save(int index);
  ErrFile exportFile(QString filename, int index);
  ErrFile saveAs(QString filename, int index);
  ErrFile closeCurrentFile(bool force = false);
  ErrFile saveCurrent();
  bool isSavedFile(int index);
  void FindFileBytes(int index, QByteArray arr, QList<int> &indices);
  void FindAllBytes(QByteArray arr, QList<FindResult> &res);
  void gotoFileLine(int index, quint64 offset);
  void gotoCurrentLine(quint64 offset);
  void undoCurrent();
  void setEditModeEnabled(bool b, bool isdriver = false);
  void redoCurrent();
  void undoFile(int index);
  void redoFile(int index);
  void copyFileBytes(int index, quint64 pos, qint64 len, QByteArray &arr);
  void copyCurrentBytes(quint64 pos, qint64 len, QByteArray &arr);
  void cutFileBytes(int index, quint64 pos, qint64 len, QByteArray &arr);
  void cutCurrentBytes(quint64 pos, qint64 len, QByteArray &arr);
  void pasteFileBytes(int index, QByteArray arr, qint64 len = -1);
  void pasteCurrentBytes(quint64 pos, QByteArray arr, qint64 len = -1);

private:
  void on_newfile();
  void on_openfile();
  void on_redofile();
  void on_undofile();
  void on_copyfile();
  void on_copyhex();
  void on_pastehex();
  void on_exportfile();
  void on_cutfile();
  void on_cuthex();
  void on_savesel();
  void on_delete();
  void on_pastefile();
  void on_gotoline();
  void on_findfile();
  void on_tabCloseRequested(int index);
  void on_tabAddRequested();
  void on_tabMoved(int from, int to);
  void on_opendriver();
  void on_save();
  void on_saveas();
  void on_exit();
  void on_setting_general();
  void on_setting_plugin();
  void on_gotobar(int pos, bool isline);
  void on_locChanged();
  void on_documentSwitched();
  void on_metadata();
  void on_metadataedit();
  void on_metadatadel();
  void on_metadatacls();
  void on_bookmark();
  void on_bookmarkdel();
  void on_bookmarkcls();
  void on_restoreLayout();
  void on_about();
  void on_sponsor();
  void on_wiki();
  void on_fillnop();
  void on_fillzero();
  void on_fill();
  void on_exportfindresult();
  void on_clearfindresult();
  void on_loadplg();
  void on_encoding();
  void on_openworkspace();
  void on_bookmarkChanged();

private:
  QList<HexFile> hexfiles;
  QMap<ToolBoxIndex, QAction *> toolbartools;
  QMap<ToolBoxIndex, QAction *> toolmenutools;
  QMap<ToolBoxIndex, QAction *> conmenutools;
  QMap<ToolBoxIndex, DToolButton *> toolbtnstools;

  uint defaultindex = 1; //表示新建使用的累计索引
  int _currentfile = -1; //表示正在使用文件的索引，编辑器使用
  int _pcurfile = -1;    //表示正在使用文件的索引，插件使用
  Settings *m_settings;

  PluginSystem *plgsys;

private:
  void PluginMenuNeedAdd(QMenu *menu);
  void PluginDockWidgetAdd(QDockWidget *dockw, Qt::DockWidgetArea align);
  void connectShadow(HexViewShadow *shadow);
  void connectShadowSlot(HexViewShadow *shadow);
  // shadow
  bool shadowIsValid(IWingPlugin *plugin);
  bool shadowControl(IWingPlugin *plugin);
  bool shadowRelease(IWingPlugin *plugin);
  void shadowDestory(IWingPlugin *plugin);

  void enableDirverLimit(bool b);

private:
  DMenu *plgmenu;
  DMenu *toolmenu;
  DMenu *winmenu;
  QMutex mutex;

  DIconButton *iSetBaseAddr;
  DIconButton *iColInfo;
  DIconButton *iHeaderInfo;
  DIconButton *iAsciiString;

  DLabel *iReadWrite;
  DLabel *iSaved;
  DLabel *iw;

  DIconButton *iLocked;
  DIconButton *iOver;

  QPixmap infow;
  QPixmap infouw;
  QPixmap infowg;

  QPixmap infoSaved;
  QPixmap infoUnsaved;
  QPixmap infoSaveg;
  QPixmap infoReadonly;
  QPixmap infoWriteable;
  QPixmap inforwg;
  QIcon infoCanOver;
  QIcon infoCannotOver;
  QIcon infoOverg;
  QIcon infoLock;
  QIcon infoUnLock;
  QIcon infoLockg;

  QTextBrowser *pluginInfo;
  DTableWidget *numshowtable;
  DTableWidget *findresult;
  DListWidget *bookmarks;
  QTableWidgetItem *numsitem = nullptr;
  QTableWidgetItem (*findresitem)[3] = {nullptr};
  Logger *logger;
  RecentFileManager *recentmanager;

private:
  // hexview default setting
  bool _showheader = true;
  bool _showaddr = true;
  bool _showascii = true;
  QFont _font;
  QFont _hexeditorfont;
  QString _windowmode;
  QString _encoding;

  bool _enableplugin = true;
  bool _rootenableplugin = false;

  int _findmax = 100;
  int findres = 0;

  QString lastusedpath;
};

#endif // MAINWINDOW_H
