#include "qfileregionbuffer.h"
#include <QFile>
#include <QFileInfo>
#include <unistd.h>

QFileRegionBuffer::QFileRegionBuffer(QObject *parent) : QHexBuffer(parent) {}

qint64 QFileRegionBuffer::length() const { return buffer.length(); }

void QFileRegionBuffer::insert(qint64 offset, const QByteArray &data) {
  Q_UNUSED(offset);
  Q_UNUSED(data);
  // 不支持插入，仅支持复写
  return;
}

void QFileRegionBuffer::remove(qint64 offset, int length) {
  // 在此模式下，删除视为按 0 填充
  replace(offset, QByteArray(length, 0));
}

QByteArray QFileRegionBuffer::read(qint64 offset, int length) {
  return buffer.mid(int(offset), length);
}

bool QFileRegionBuffer::read(QIODevice *iodevice) {
  auto file = qobject_cast<QFile *>(iodevice);
  if (file && file->open(QFile::ReadOnly)) {
    lseek(file->handle(), offset,
          SEEK_SET); // 有些特殊的文件 Qt 是不支持 seek 的，用原生函数
    buffer = file->read(maxlength);
    file->close();
    return true;
  }
  return false;
}

void QFileRegionBuffer::write(QIODevice *iodevice) {
  QFile file(iodevice);
  if (file.open(QFile::WriteOnly)) {
    lseek(file.handle(), offset, SEEK_SET);
    file.write(buffer);
    file.close();
  }
}

qint64 QFileRegionBuffer::indexOf(const QByteArray &ba, qint64 from) {
  return buffer.indexOf(ba, int(from));
}

qint64 QFileRegionBuffer::lastIndexOf(const QByteArray &ba, qint64 from) {
  return buffer.lastIndexOf(ba, int(from));
}

void QFileRegionBuffer::setReadOffset(qint64 offset) { this->offset = offset; }

qint64 QFileRegionBuffer::readOffset() { return offset; }

qint64 QFileRegionBuffer::readMaxBytes() { return maxlength; }

void QFileRegionBuffer::setReadMaxBytes(qint64 maxlength) {
  if (maxlength)
    this->maxlength = maxlength;
}
