#include "driverselectordialog.h"
#include "utilities.h"
#include <DDialogButtonBox>
#include <DLabel>
#include <QShortcut>

DriverSelectorDialog::DriverSelectorDialog(DMainWindow *parent)
    : DDialog(parent) {
  setWindowTitle(tr("OpenDriver"));
  drivers = new DListWidget(this);
  drivers->setSortingEnabled(false);
  QStorageInfo si;
  auto ico = ICONRES("opendriver");
  auto infos = si.mountedVolumes();
  addContent(new DLabel("PleaseChooseDriver", this));
  addSpacing(5);
  for (auto item : infos) {
    if (item.device()[0] == '/') {
      drivers->addItem(new QListWidgetItem(ico, item.device()));
      m_infos.push_back(item);
    }
  }
  addContent(drivers);
  addSpacing(5);
  infob = new DTextBrowser(this);
  addContent(infob);
  addSpacing(5);
  addContent(new DLabel(tr("DriverTips"), this));
  addSpacing(5);
  auto dbbox =
      new DDialogButtonBox(DDialogButtonBox::Ok | DDialogButtonBox::Cancel);

  connect(dbbox, &DDialogButtonBox::accepted, this,
          &DriverSelectorDialog::on_accepted);
  connect(dbbox, &DDialogButtonBox::rejected, this,
          &DriverSelectorDialog::on_rejected);
  addContent(dbbox);
  auto key = QKeySequence(Qt::Key_Return);
  auto s = new QShortcut(key, this);
  connect(s, &QShortcut::activated, this, &DriverSelectorDialog::on_accepted);
  connect(drivers, &QListWidget::itemSelectionChanged, this,
          &DriverSelectorDialog::on_list_selectionChanged);
}

void DriverSelectorDialog::on_list_selectionChanged() {
  infob->clear();
#define Info(mem, info) infob->append(mem + " : " + info)
  auto item = m_infos.at(drivers->currentRow());
  Info(tr("device"), item.device());
  Info(tr("displayName"), item.displayName());
  Info(tr("fileSystemType"), item.fileSystemType());
  Info(tr("name"), item.name());

  if (item.isReady()) {
    Info(tr("isReady"), "True");
    Info(tr("bytesAvailable"),
         Utilities::ProcessBytesCount(item.bytesAvailable()));
    Info(tr("bytesTotal"), Utilities::ProcessBytesCount(item.bytesTotal()));
  } else {
    Info(tr("isReady"), "False");
  }
}

void DriverSelectorDialog::on_accepted() {
  m_si = m_infos.at(drivers->currentRow());
  done(1);
}

void DriverSelectorDialog::on_rejected() { done(0); }

QStorageInfo DriverSelectorDialog::GetResult() { return m_si; }
