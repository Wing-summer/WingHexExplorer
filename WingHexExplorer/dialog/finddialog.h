#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include "./QHexView/qhexview.h"
#include <DComboBox>
#include <DDialog>
#include <DLineEdit>
#include <DMainWindow>
#include <DRadioButton>
#include <QObject>

DWIDGET_USE_NAMESPACE

enum class SearchDirection { None, Foreword, Backword, Selection };

class FindDialog : public DDialog {
  Q_OBJECT
public:
  FindDialog(bool sel = true, DMainWindow *parent = nullptr);
  QByteArray getResult(SearchDirection &dir);

private:
  void on_accept();
  void on_reject();

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  QHexView *m_hexeditor;
  DLineEdit *m_lineeditor;
  DRadioButton *m_string;
  DRadioButton *m_hex;
  DComboBox *m_encodings;
  QByteArray _findarr;

  SearchDirection _dir = SearchDirection::None;
};

#endif // FINDDIALOG_H
