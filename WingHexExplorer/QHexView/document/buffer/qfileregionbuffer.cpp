#include "qfileregionbuffer.h"
#include <QFile>
#include <QFileInfo>
#include <unistd.h>

QFileRegionBuffer::QFileRegionBuffer(QObject *parent) : QHexBuffer(parent) {}

qint64 QFileRegionBuffer::length() const { return buffer.length(); }

void QFileRegionBuffer::insert(qint64 offset, const QByteArray &data) {
  buffer.insert(int(offset), data);
  return;
}

void QFileRegionBuffer::remove(qint64 offset, int length) {
  buffer.remove(int(offset), int(length));
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

#include <QDebug>

void QFileRegionBuffer::write(QIODevice *iodevice) {
  auto file = qobject_cast<QFile *>(iodevice);
  lseek(file->handle(), offset, SEEK_SET);
  file->write(buffer);
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
