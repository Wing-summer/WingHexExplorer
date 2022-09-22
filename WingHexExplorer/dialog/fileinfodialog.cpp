#include "fileinfodialog.h"
#include "utilities.h"
#include <DLabel>
#include <DTextBrowser>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>

FileInfoDialog::FileInfoDialog(QString filename, DMainWindow *parent)
    : DDialog(parent) {
  setWindowTitle(tr("FileInfo"));

  QFileInfo finfo(filename);

  QMimeDatabase db;
  auto t = db.mimeTypeForFile(filename);
  auto ico = t.iconName();
  auto icon = QIcon::fromTheme(ico, QIcon(ico));

  auto l = new DLabel(this);
  l->setFixedSize(100, 100);
  l->setScaledContents(true);
  l->setPixmap(icon.pixmap(icon.availableSizes().last()));
  addContent(l, Qt::AlignHCenter);
  addSpacing(10);
  auto b = new DTextBrowser(this);

  static const QString dfmt("yyyy/MM/dd hh:mm:ss ddd");

  if (filename[0] != '/') {
    b->append(tr("FileNew"));
  } else {
    b->append(tr("FileName:") + finfo.fileName());
    b->append(tr("FilePath:") + finfo.filePath());
    b->append(tr("FileSize:") + Utilities::processBytesCount(finfo.size()));
    b->append(tr("Mime:") + t.name());
    b->append(tr("Md5:") + Utilities::getMd5(filename).toHex());
    b->append(tr("FileBirthTime:") +
              finfo.fileTime(QFile::FileTime::FileBirthTime).toString(dfmt));
    b->append(tr("FileAccessTime:") +
              finfo.fileTime(QFile::FileTime::FileAccessTime).toString(dfmt));
    b->append(
        tr("FileModificationTime:") +
        finfo.fileTime(QFile::FileTime::FileModificationTime).toString(dfmt));
    b->append(tr("LastRead:") + finfo.lastRead().toString(dfmt));
    b->append(tr("LastMod:") + finfo.lastModified().toString(dfmt));
  }

  b->setFixedSize(400, 300);
  addContent(b);
}
