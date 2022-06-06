#ifndef METADIALOG_H
#define METADIALOG_H

#include <DCheckBox>
#include <DColorDialog>
#include <DDialog>
#include <DLabel>
#include <DLineEdit>
#include <DMainWindow>
#include <DPushButton>
#include <QColor>
#include <QObject>
#include <QPalette>
DWIDGET_USE_NAMESPACE

class MetaDialog : public DDialog {
  Q_OBJECT
public:
  explicit MetaDialog(DMainWindow *parent = nullptr);
  QColor foreGroundColor();
  QColor backGroundColor();
  QString comment();

  void setForeGroundColor(QColor color);
  void setBackGroundColor(QColor color);
  void setComment(QString comment);

signals:

private:
  void on_accept();
  void on_reject();

private:
  DCheckBox *cforeground, *cbackground, *ccomment;
  DLineEdit *m_comment;
  DPushButton *iforeground, *ibackground;

  QColor _foreground;
  QColor _background;
  QString _comment;
};

#endif // METADIALOG_H
