#include "finddialog.h"
#include <DDialogButtonBox>

FindDialog::FindDialog(DMainWindow *parent) : DDialog(parent) {
  this->setFixedSize(500, 600);
  this->setWindowTitle(tr("find"));

  m_string = new DRadioButton(this);
  m_string->setText(tr("findstring"));
  addContent(m_string);
  addSpacing(3);

  m_lineeditor = new DLineEdit(this);
  connect(m_string, &DRadioButton::toggled, m_lineeditor,
          &DLineEdit::setEnabled);
  addContent(m_lineeditor);
  addSpacing(3);

  m_hex = new DRadioButton(this);
  m_hex->setText(tr("findhex"));
  m_hex->setEnabled(true);
  addContent(m_hex);
  addSpacing(3);

  m_hexeditor = new QHexView(this);
  m_hexeditor->setAsciiVisible(false);
  m_hexeditor->setAddressVisible(false);
  m_hexeditor->setEnabled(false);
  connect(m_hex, &DRadioButton::toggled, m_hexeditor, &QHexView::setEnabled);
  addContent(m_hexeditor);
  addSpacing(10);

  m_string->setChecked(true);

  auto dbbox = new DDialogButtonBox(
      DDialogButtonBox::Ok | DDialogButtonBox::Cancel, this);
  connect(dbbox, &DDialogButtonBox::accepted, this, &FindDialog::on_accept);
  connect(dbbox, &DDialogButtonBox::rejected, this, &FindDialog::on_reject);
  addContent(dbbox);
}

QByteArray FindDialog::getResult() { return _findarr; }

void FindDialog::on_accept() {
  if (m_string->isChecked()) {
    _findarr = m_lineeditor->text().toUtf8();
  } else {
    _findarr =
        m_hexeditor->document()->read(0, int(m_hexeditor->documentBytes()));
  }
  done(1);
}

void FindDialog::on_reject() {
  _findarr.clear();
  done(0);
}
