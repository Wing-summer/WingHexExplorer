#include "encodingdialog.h"
#include "utilities.h"
#include <DLabel>
#include <DPushButton>
#include <QAction>
#include <QListWidgetItem>
#include <QShortcut>

EncodingDialog::EncodingDialog(DMainWindow *parent) {
  Q_UNUSED(parent);
  this->setWindowTitle(tr("Encoding"));
  this->setFixedSize(500, 600);
  auto l = new DLabel(tr("ChooseEncoding"), this);
  addContent(l);
  addSpacing(5);
  enclist = new DListWidget(this);
  for (auto item : Utilities::GetEncodings()) {
    enclist->addItem(item);
  }
  addContent(enclist);
  addSpacing(10);
  auto dbbox = new DDialogButtonBox(
      DDialogButtonBox::Ok | DDialogButtonBox::Cancel, this);
  connect(dbbox, &DDialogButtonBox::accepted, this, &EncodingDialog::on_accept);
  connect(dbbox, &DDialogButtonBox::rejected, this, &EncodingDialog::on_reject);
  auto key = QKeySequence(Qt::Key_Return);
  auto s = new QShortcut(key, this);
  connect(s, &QShortcut::activated, this, &EncodingDialog::on_accept);
  addContent(dbbox);
}

QString EncodingDialog::getResult() { return result; }

void EncodingDialog::on_accept() {
  auto s = enclist->selectedItems();
  if (s.count() > 0) {
    result = s.first()->text();
    done(1);
  } else {
    done(0);
  }
}

void EncodingDialog::on_reject() { done(0); }

void EncodingDialog::closeEvent(QCloseEvent *event) {
  Q_UNUSED(event);
  done(0);
}
