#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "QHexView/document/qhexdocument.h"
#include "QHexView/qhexview.h"
#include "gotobar.h"
#include "logger.h"
#include "pluginsystem.h"
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
#include <QFile>
#include <QList>
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
    Export,
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Del,
    Find,
    Goto,
    Fill,
    FillNop,
    FillZero,
    Meta,
    DelMeta,
    ClsMeta,
    BookMark,
    DelBookMark,
    ClsBookMark,
    MaxTool
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
  // DDockWidget *dw;
  DMenu *hexeditorMenu;
  DMenu *findresultMenu;

private:
  QHexView *hexeditor;
  QFile file;
  GotoBar *gotobar;

private:
  void setTheme(DGuiApplicationHelper::ColorType theme);

public:
  ErrFile openFile(QString filename, bool readonly = false);

private:
  void newFile();
  ErrFile openDriver(QString driver);
  ErrFile closeFile(int index, bool force = false);
  ErrFile saveFile(int index);
  ErrFile exportFile(QString filename, int index);
  ErrFile saveasFile(QString filename, int index);
  ErrFile closeCurrentFile(bool force = false);
  ErrFile saveCurrentFile();
  bool isModified(int index);
  void FindFileBytes(int index, QByteArray arr, QList<int> &indices);
  void FindAllBytes(QByteArray arr, QList<FindResult> &res);
  void gotoFileLine(int index, quint64 offset);
  void gotoCurrentLine(quint64 offset);
  void undoCurrent();
  void setEditModeEnabled(bool b);
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
  void setFilePage(int index);
  void on_newfile();
  void on_openfile();
  void on_redofile();
  void on_undofile();
  void on_copyfile();
  void on_exportfile();
  void on_cutfile();
  void on_delete();
  void on_pastefile();
  void on_gotoline();
  void on_findfile();
  void on_tabCloseRequested(int index);
  void on_tabAddRequested();
  void on_tabMoved(int from, int to);
  void on_opendriver();
  void on_savefile();
  void on_saveasfile();
  void on_exit();
  void on_setting_general();
  void on_setting_plugin();
  void on_gotobar(int pos, bool isline);
  void on_locChanged();
  void on_documentChanged();
  void on_documentSwitched();
  void on_documentStatusChanged();
  void on_metadata();
  void on_metadatadel();
  void on_metadatacls();
  void on_bookmark();
  void on_bookmarkdel();
  void on_bookmarkcls();
  void on_restoreLayout();
  void on_about();
  void on_sponsor();
  void on_fillnop();
  void on_fillzero();
  void on_fill();
  void on_clearfindresult();

private:
  QList<HexFile> hexfiles;
  QMap<ToolBoxIndex, QAction *> toolbartools;
  QMap<ToolBoxIndex, QAction *> toolmenutools;
  DMenu *editmenu;

  uint defaultindex = 1; //表示新建使用的累计索引
  int _currentfile = -1; //表示正在使用文件的索引，编辑器使用
  int _pcurfile = -1;    //表示正在使用文件的索引，插件使用
  Settings *m_settings;

  PluginSystem *plgsys;

private:
  void PluginMenuNeedAdd(QMenu *menu);
  void PluginDockWidgetAdd(QDockWidget *dockw, Qt::DockWidgetArea align);
  void connectShadow(HexViewShadow *shadow);

  // shadow
  bool shadowIsValid(IWingPlugin *plugin);
  bool shadowControl(IWingPlugin *plugin, HexViewShadow *shadow);
  bool shadowRelease(IWingPlugin *plugin, HexViewShadow *shadow);
  void shadowDestory(IWingPlugin *plugin);

private:
  DMenu *plgmenu;
  DMenu *toolmenu;

  DIconButton *iSetBaseAddr;
  DIconButton *iColInfo;
  DIconButton *iHeaderInfo;
  DIconButton *iAsciiString;

  DLabel *iReadWrite;
  DLabel *iSaved;
  DIconButton *iLocked;
  DIconButton *iOver;

  QPixmap infoSaved;
  QPixmap infoUnsaved;
  QPixmap infoReadonly;
  QPixmap infoWriteable;
  QIcon infoCanOver;
  QIcon infoCannotOver;
  QIcon infoLock;
  QIcon infoUnLock;

  QTextBrowser *pluginInfo;
  DTableWidget *numshowtable;
  DTableWidget *findresult;
  DListWidget *bookmarks;
  QTableWidgetItem *numsitem = nullptr;
  QTableWidgetItem (*findresitem)[3] = {nullptr};
  Logger *logger;

private:
  // hexview default setting
  bool _showheader = true;
  bool _showaddr = true;
  bool _showascii = true;
  QFont _font;
  QFont _hexeditorfont;
  QString _windowmode;

  bool _enableplugin = true;
};

#endif // MAINWINDOW_H
