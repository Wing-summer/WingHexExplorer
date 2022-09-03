#include "openregiondialog.h"
#include "utilities.h"
#include <DDialogButtonBox>
#include <DLabel>
#include <DMessageManager>
#include <QShortcut>

OpenRegionDialog::OpenRegionDialog(DMainWindow *parent) : DDialog(parent) {
  this->setWindowTitle(tr("OpenRegion"));
  addContent(new DLabel(tr("ChooseFile"), this));
  addSpacing(5);
  filepath = new DFileChooserEdit(this);
  filepath->initDialog();
  addContent(filepath);
  addSpacing(10);

  addContent(new DLabel(tr("Start"), this));
  addSpacing(5);
  sbStart = new DSpinBox(this);
  sbStart->setRange(0, INT_MAX);
  sbStart->setPrefix("0x");
  sbStart->setDisplayIntegerBase(16);
  addContent(sbStart);
  addSpacing(10);

  addContent(new DLabel(tr("Len"), this));
  addSpacing(5);
  sbLength = new DSpinBox(this);
  sbLength->setRange(1, INT_MAX);
  addContent(sbLength);

  addSpacing(20);
  auto dbbox = new DDialogButtonBox(
      DDialogButtonBox::Ok | DDialogButtonBox::Cancel, this);
  connect(dbbox, &DDialogButtonBox::accepted, this,
          &OpenRegionDialog::on_accept);
  connect(dbbox, &DDialogButtonBox::rejected, this,
          &OpenRegionDialog::on_reject);
  auto key = QKeySequence(Qt::Key_Return);
  auto s = new QShortcut(key, this);
  connect(s, &QShortcut::activated, this, &OpenRegionDialog::on_accept);
  addContent(dbbox);
}

RegionFileResult OpenRegionDialog::getResult() { return res; }

void OpenRegionDialog::on_accept() {
  auto file = filepath->lineEdit()->text();
  if (file.length()) {
    if (!QFile::exists(file)) {
      DMessageManager::instance()->sendMessage(this, ICONRES("open"),
                                               tr("FileNotExist"));
    }
  } else {
    DMessageManager::instance()->sendMessage(this, ICONRES("open"),
                                             tr("NoFileSelected"));
    return;
  }
  res.filename = file;
  res.start = sbStart->value();
  res.length = sbLength->value();
  done(1);
}

void OpenRegionDialog::on_reject() { done(0); }

void OpenRegionDialog::closeEvent(QCloseEvent *event) {
  Q_UNUSED(event);
  done(0);
}
