#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include "./QHexView/qhexview.h"
#include <DDialog>
#include <DLineEdit>
#include <DMainWindow>
#include <DRadioButton>
#include <QObject>

DWIDGET_USE_NAMESPACE

class FindDialog : public DDialog {
  Q_OBJECT
public:
  FindDialog(DMainWindow *parent = nullptr);
  QByteArray getResult();

private:
  void on_accept();
  void on_reject();

private:
  QHexView *m_hexeditor;
  DLineEdit *m_lineeditor;
  DRadioButton *m_string;
  DRadioButton *m_hex;

  QByteArray _findarr;
};

#endif // FINDDIALOG_H
