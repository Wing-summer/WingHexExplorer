#include "sponsordialog.h"
#include <DLabel>
#include <QPixmap>

SponsorDialog::SponsorDialog(DMainWindow *parent) : DDialog(parent) {
  setWindowTitle(tr("Sponsor"));

  addSpacing(5);
  addContent(new DLabel(tr("ThanksForSponsor"), this), Qt::AlignHCenter);
  addSpacing(5);
  QPixmap sponsor(":/resources/sponsor.png");

  auto l = new DLabel(this);
  l->setPixmap(sponsor);
  l->setScaledContents(true);
  addContent(l);
}
