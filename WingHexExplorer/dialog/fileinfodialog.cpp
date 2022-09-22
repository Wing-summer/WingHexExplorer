#include "fileinfodialog.h"
#include "utilities.h"
#include <DLabel>
#include <DTextBrowser>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>

FileInfoDialog::FileInfoDialog(QString filename, DMainWindow *parent)
    : DDialog(parent) {
  static const QString dfmt("yyyy/MM/dd hh:mm:ss ddd");

  setWindowTitle(tr("FileInfo"));
  auto l = new DLabel(this);
  l->setFixedSize(100, 100);
  l->setScaledContents(true);
  QIcon icon;
  auto b = new DTextBrowser(this);

  if (filename[0] != '/') {
    icon = this->style()->standardIcon(QStyle::SP_FileIcon);
    b->append(tr("FileNew"));
  } else {
    QMimeDatabase db;
    auto t = db.mimeTypeForFile(filename);
    auto ico = t.iconName();
    icon = QIcon::fromTheme(ico, QIcon(ico));
    QFileInfo finfo(filename);
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
  l->setPixmap(icon.pixmap(icon.availableSizes().last()));
  addContent(l, Qt::AlignHCenter);
  addSpacing(10);
  b->setFixedSize(400, 300);
  addContent(b);
}
