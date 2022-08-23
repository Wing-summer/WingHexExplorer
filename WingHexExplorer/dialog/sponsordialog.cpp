#include "sponsordialog.h"
#include <DLabel>
#include <QPixmap>

SponsorDialog::SponsorDialog(DMainWindow *parent, QString message, QPixmap img)
    : DDialog(parent) {
  setWindowTitle(tr("Sponsor"));

  addSpacing(5);
  addContent(
      new DLabel(message.isEmpty() ? tr("ThanksForSponsor") : message, this),
      Qt::AlignHCenter);
  addSpacing(5);

  QPixmap sponsor;

  if (img.isNull()) {
    sponsor.load(":/resources/sponsor.png");
  } else {
    sponsor.swap(img);
  }

  auto l = new DLabel(this);
  l->setPixmap(sponsor);
  l->setScaledContents(true);
  addContent(l);
}
