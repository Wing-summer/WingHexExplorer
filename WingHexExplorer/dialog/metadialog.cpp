#include "metadialog.h"
#include "utilities.h"
#include <DDialogButtonBox>
#include <DMessageManager>
#include <QShortcut>

MetaDialog::MetaDialog(DMainWindow *parent)
    : DDialog(parent), _foreground(Qt::transparent),
      _background(Qt::transparent) {
  setWindowTitle(tr("Metadata"));
  cforeground = new DCheckBox(this);
  cforeground->setText(tr("foreground"));
  addContent(cforeground);
  addSpacing(2);

  iforeground = new DPushButton(this);
  iforeground->setText(tr("foreground"));
  iforeground->setEnabled(false);
  addContent(iforeground);

  addSpacing(2);

  cbackground = new DCheckBox(this);
  cbackground->setText(tr("background"));
  addContent(cbackground);

  addSpacing(2);

  ibackground = new DPushButton(this);
  ibackground->setText(tr("background"));
  ibackground->setEnabled(false);
  addContent(ibackground);

  addSpacing(2);

  ccomment = new DCheckBox(this);
  ccomment->setText(tr("comment"));
  addContent(ccomment);

  addSpacing(2);

  m_comment = new DLineEdit(this);
  addContent(m_comment);
  m_comment->setEnabled(false);

  addSpacing(5);

  auto dbbox = new DDialogButtonBox(
      DDialogButtonBox::Ok | DDialogButtonBox::Cancel, this);
  addContent(dbbox);
  addSpacing(2);

  connect(cforeground, &DCheckBox::clicked, iforeground,
          &DPushButton::setEnabled);
  connect(cbackground, &DCheckBox::clicked, ibackground,
          &DPushButton::setEnabled);
  connect(ccomment, &DCheckBox::clicked, m_comment, &DPushButton::setEnabled);
  connect(iforeground, &DPushButton::clicked, this, [=] {
    QColorDialog cd;
    if (cd.exec()) {
      QPalette pe;
      pe.setColor(QPalette::ButtonText, cd.currentColor());
      iforeground->setPalette(pe);
      _foreground = cd.currentColor();
    }
  });
  connect(ibackground, &DPushButton::clicked, this, [=] {
    QColorDialog cd;
    if (cd.exec()) {
      QPalette pe;
      pe.setColor(QPalette::ButtonText, cd.currentColor());
      ibackground->setPalette(pe);
      _background = cd.currentColor();
    }
  });
  connect(dbbox, &DDialogButtonBox::accepted, this, &MetaDialog::on_accept);
  connect(dbbox, &DDialogButtonBox::rejected, this, &MetaDialog::on_reject);
  auto key = QKeySequence(Qt::Key_Return);
  auto s = new QShortcut(key, this);
  connect(s, &QShortcut::activated, this, &MetaDialog::on_accept);
}

void MetaDialog::on_accept() {
  if ((cforeground->isChecked() &&
       (_foreground.isValid() || _foreground.rgba() == 0)) ||
      (cbackground->isChecked() &&
       (_background.isValid() || _background.rgba() == 0)) ||
      (ccomment->isChecked() && m_comment->text().trimmed().length() == 0)) {
    DMessageManager::instance()->sendMessage(this, ICONRES("metadata"),
                                             tr("NoChoose"));
    return;
  }

  _comment = ccomment->text();
  done(1);
}

void MetaDialog::on_reject() { done(0); }

QString MetaDialog::comment() {
  if (ccomment->isChecked())
    return m_comment->text();
  else
    return "";
}

QColor MetaDialog::foreGroundColor() {
  if (cforeground->isChecked())
    return _foreground;
  else
    return QColor::fromRgba(qRgba(0, 0, 0, 0));
}

QColor MetaDialog::backGroundColor() {
  if (cbackground->isChecked())
    return _background;
  else
    return QColor::fromRgba(qRgba(0, 0, 0, 0));
}

void MetaDialog::setComment(QString comment) {
  if (comment.length() > 0) {
    ccomment->setChecked(true);
    emit ccomment->clicked(true);
    _comment = comment;
    m_comment->setText(comment);
  }
}

void MetaDialog::setBackGroundColor(QColor color) {
  if (color.rgba()) {
    cbackground->setChecked(true);
    emit cbackground->clicked(true);
    _background = color;
    QPalette pe;
    pe.setColor(QPalette::ButtonText, color);
    ibackground->setPalette(pe);
  }
}

void MetaDialog::setForeGroundColor(QColor color) {
  if (color.rgba()) {
    cforeground->setChecked(true);
    emit cforeground->clicked(true);
    _foreground = color;
    QPalette pe;
    pe.setColor(QPalette::ButtonText, color);
    iforeground->setPalette(pe);
  }
}

void MetaDialog::closeEvent(QCloseEvent *event) {
  Q_UNUSED(event);
  done(0);
}
