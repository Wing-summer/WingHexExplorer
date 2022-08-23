#include "aboutsoftwaredialog.h"
#include <DLabel>
#include <DTextBrowser>
#include <QPixmap>

AboutSoftwareDialog::AboutSoftwareDialog(DMainWindow *parent, QPixmap img,
                                         QStringList searchPaths,
                                         QString source)
    : DDialog(parent) {
  setWindowTitle(tr("About"));

  QPixmap pic;
  if (img.isNull()) {
    pic.load(":/images/author.jpg");
  } else {
    pic.swap(img);
  }

  auto l = new DLabel(this);
  l->setFixedSize(100, 100);
  l->setScaledContents(true);
  l->setPixmap(pic);
  addContent(l, Qt::AlignHCenter);
  addSpacing(10);
  auto b = new DTextBrowser(this);

  if (searchPaths.length()) {
    b->setSearchPaths(searchPaths);
  } else {
    b->setSearchPaths(QStringList({":/resources", ":/images"}));
  }

  if (source.isEmpty()) {
    b->setSource(QUrl("README.md"), QTextDocument::MarkdownResource);
  } else {
    b->setSource(QUrl(source), QTextDocument::MarkdownResource);
  }

  b->setFixedSize(800, 500);
  b->setOpenExternalLinks(true);
  addContent(b);
}
