#ifndef SETTING_H
#define SETTING_H

#include <DApplication>
#include <DDialog>
#include <DKeySequenceEdit>
#include <DMainWindow>
#include <DSettings>
#include <DSettingsDialog>
#include <QKeyEvent>
#include <QObject>
#include <QWidget>
#include <QtCore>
#include <qsettingbackend.h>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE
DTK_USE_NAMESPACE

class Settings : public QObject {
  Q_OBJECT

public:
  explicit Settings(QWidget *parent = nullptr);
  ~Settings();

  void dtkThemeWorkaround(QWidget *parent, const QString &theme);
  static QPair<QWidget *, QWidget *> createFontComBoBoxHandle(QObject *obj);
  static Settings *instance();

  void setSettingDialog(DSettingsDialog *settingsDialog);
  void applySetting();
  void saveWindowState(DMainWindow *wnd, bool isorign = false);
  void loadWindowState(DMainWindow *wnd, bool isorign = false);
  QString loadFileDialogCurrent();
  void saveFileDialogCurrent(QString path);
  QStringList loadRecent();
  void saveRecent(QStringList recent);

  int m_iDefaultFontSize = 12;
  int m_iMaxFontSize = 50;
  int m_iMinFontSize = 8;

  DSettings *settings;

signals:
  void sigAdjustFont(QString name);
  void sigAdjustInfoFontSize(int fontSize);
  void sigAdjustEditorFontSize(int fontSize);
  void sigShowAddressNumber(bool enable);
  void sigShowColNumber(bool enable);
  void sigShowEncodingText(bool enable);
  void sigChangedEncoding(QString encoding);
  void sigChangeWindowSize(QString mode);
  void sigChangeWindowState(QString state);
  void sigChangePluginEnabled(bool b);
  void sigChangeRootPluginEnabled(bool b);
  void sigAdjustFindMaxCount(int count);

private:
  DDialog *createDialog(const QString &title, const QString &content,
                        const bool &bIsConflicts);

private:
  Dtk::Core::QSettingBackend *m_backend;
  DSettingsDialog *m_pSettingsDialog;
  static Settings *s_pSetting;
  DDialog *m_pDialog;
};

#endif // SETTING_H
