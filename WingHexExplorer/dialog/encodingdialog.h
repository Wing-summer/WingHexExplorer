#ifndef ENCODINGDIALOG_H
#define ENCODINGDIALOG_H

#include <DDialog>
#include <DDialogButtonBox>
#include <DListWidget>
#include <DMainWindow>
#include <QTextCodec>

DWIDGET_USE_NAMESPACE

class EncodingDialog : public DDialog {
  Q_OBJECT
public:
  EncodingDialog(DMainWindow *parent = nullptr);
  QString getResult();

private:
  void on_accept();
  void on_reject();

private:
  DListWidget *enclist;
  QString result;
};

#endif // ENCODINGDIALOG_H
