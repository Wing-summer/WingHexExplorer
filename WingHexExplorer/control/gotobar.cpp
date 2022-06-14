#include "gotobar.h"
#include "utilities.h"
#include <QShortcut>

const int nJumpLineBarWidth = 212;
const int nJumpLineBarHeight = 60;

GotoBar::GotoBar(QWidget *parent) : DFloatingWidget(parent) {

  m_layout = new QHBoxLayout(this);
  m_layout->setContentsMargins(20, 6, 20, 6);

  m_close = new DIconButton(this);
  m_close->setIcon(ICONRES("closefile"));
  m_close->setFixedSize(25, 25);
  m_layout->addWidget(m_close);

  m_label = new QLabel(this);
  m_label->setText(tr("Goto"));
  m_editLine = new DLineEdit(this);

  m_layout->addWidget(m_label);
  m_layout->addWidget(m_editLine);

  m_line = new DRadioButton(this);
  m_line->setText(tr("Line"));
  m_layout->addWidget(m_line);

  m_offset = new DRadioButton(this);
  m_offset->setText(tr("Offset"));
  m_offset->setChecked(true);
  m_layout->addWidget(m_offset);

  m_goto = new DIconButton(this);
  m_goto->setIcon(ICONRES("goto"));
  m_goto->setFixedSize(50, 30);
  m_layout->addWidget(m_goto);

  this->setLayout(m_layout);

  setFixedHeight(nJumpLineBarHeight);

  connect(m_close, &DIconButton::clicked, this, &GotoBar::jumpCancel);
  connect(m_editLine, &DLineEdit::returnPressed, this, &GotoBar::jumpConfirm);
  connect(m_editLine, &DLineEdit::textChanged, this,
          &GotoBar::handleLineChanged);
  connect(m_goto, &DIconButton::clicked, this, &GotoBar::jumpConfirm);

  auto sc = QKeySequence(Qt::Key_Escape);
  m_close->setShortcut(sc);
  QShortcut s(QKeySequence(Qt::Key_Escape), this);
  s.setContext(Qt::ShortcutContext::WidgetShortcut);
}

void GotoBar::focus() { m_editLine->lineEdit()->setFocus(); }

bool GotoBar::isFocus() { return m_editLine->lineEdit()->hasFocus(); }

void GotoBar::activeInput(int oldrow, int oldcolumn, quint64 oldoffset,
                          quint64 maxfilebytes, int maxfilelines) {
  m_rowBeforeJump = oldrow;
  m_columnBeforeJump = oldcolumn;
  m_oldFileOffsetBeforeJump = oldoffset;
  m_maxFileBytes = maxfilebytes;
  m_maxFilelines = maxfilelines;
  auto edit = m_editLine->lineEdit();
  edit->clear();
  edit->setFocus();
  setVisible(true);
}

void GotoBar::handleLineChanged() {
  QString content = m_editLine->lineEdit()->text();
  if (content != "") {
    auto ps = SEEKPOS::Invaild;
    auto isline = m_line->isChecked();
    auto p = Convert2Pos(content, ps, isline);
    if (ps != SEEKPOS::Invaild)
      jumpToLine(p, isline);
    else
      m_editLine->showAlertMessage(tr("InvalidContent"), 1500);
  }
}

void GotoBar::jumpCancel() {
  jumpToLine(qlonglong(m_oldFileOffsetBeforeJump), false);
  setVisible(false);
}

void GotoBar::jumpConfirm() {
  handleLineChanged();
  setVisible(false);
}

qint64 GotoBar::Convert2Pos(QString value, SEEKPOS &ps, bool isline) {
  qint64 res = 0;
  if (value.length() > 0) {
    auto ch = value.at(0);
    if (ch == '+') {
      ps = SEEKPOS::Relative;
      value = value.remove(0, 1);

      bool ok = false;
      res = value.toLongLong(&ok, 0);

      if (!ok) {
        ps = SEEKPOS::Invaild;
      } else {
        if (isline) {
          if (res + m_rowBeforeJump > m_maxFilelines) {
            ps = SEEKPOS::Invaild;
            res = 0;
          } else {
            res += m_rowBeforeJump;
          }
        } else {
          if (qulonglong(res) + m_oldFileOffsetBeforeJump > m_maxFileBytes) {
            ps = SEEKPOS::Invaild;
            res = 0;
          } else {
            res += m_oldFileOffsetBeforeJump;
          }
        }
      }

    } else if (ch == '-') {
      ps = SEEKPOS::Relative;

      value = value.remove(0, 1);

      bool ok = false;
      res = value.toLongLong(&ok, 0);

      if (!ok) {
        ps = SEEKPOS::Invaild;
      } else {
        if (isline) {
          if (res - m_rowBeforeJump < 0) {
            ps = SEEKPOS::Invaild;
            res = 0;
          } else {
            res -= m_rowBeforeJump;
          }
        } else {
          if (qlonglong(m_oldFileOffsetBeforeJump) - res < 0) {
            ps = SEEKPOS::Invaild;
            res = 0;
          } else {
            res = qlonglong(m_oldFileOffsetBeforeJump) - res;
          }
        }
      }

    } else if (ch == '<') {
      ps = SEEKPOS::End;
      value = value.remove(0, 1);

      bool ok = false;
      res = value.toLongLong(&ok, 0);

      if (!ok || res < 0) {
        ps = SEEKPOS::Invaild;
      } else {
        if (isline) {
          if (m_maxFilelines - res < 0) {
            ps = SEEKPOS::Invaild;
            res = 0;
          } else {
            res = m_maxFilelines - res;
          }
        } else {
          if (qlonglong(m_maxFileBytes) - res < 0) {
            ps = SEEKPOS::Invaild;
            res = 0;
          } else {
            res = qlonglong(m_maxFileBytes) - res;
          }
        }
      }
    } else {
      ps = SEEKPOS::Start;

      bool ok = false;
      res = value.toInt(&ok, 0);

      if (!ok) {
        ps = SEEKPOS::Invaild;
      } else {
        if (res < 0 || quint64(res) > (isline ? quint64(m_maxFilelines)
                                              : m_maxFileBytes)) {
          ps = SEEKPOS::Invaild;
          res = 0;
        }
      }
    }
  }
  return res;
}
