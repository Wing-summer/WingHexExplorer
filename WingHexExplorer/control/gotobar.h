#ifndef GOTOBAR_H
#define GOTOBAR_H

#include <DFloatingWidget>
#include <DIconButton>
#include <DLineEdit>
#include <DRadioButton>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QPushButton>

DWIDGET_USE_NAMESPACE

enum SEEKPOS { Invaild, Start, End, Relative };

class GotoBar : public DFloatingWidget {
  Q_OBJECT
public:
  GotoBar(QWidget *parent = nullptr);

public slots:
  void focus();
  bool isFocus();

  void activeInput(int oldrow, int oldcolumn, quint64 oldoffset,
                   quint64 maxfilebytes, int maxfilelines);

  void handleLineChanged();

  void jumpCancel();
  void jumpConfirm();

signals:
  void jumpToLine(qlonglong pos, bool isline);
  void pressEsc();

private:
  DIconButton *m_close;
  DIconButton *m_goto;
  DLineEdit *m_editLine;
  QHBoxLayout *m_layout;
  DRadioButton *m_line;
  DRadioButton *m_offset;
  QLabel *m_label;

  quint64 m_oldFileOffsetBeforeJump;
  quint64 m_maxFileBytes;
  int m_maxFilelines;
  int m_rowBeforeJump;
  int m_columnBeforeJump;

private:
  qint64 Convert2Pos(QString value, SEEKPOS &ps, bool isline);
};

#endif // GOTOBAR_H
