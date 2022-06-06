#include "qmemorybuffer.h"

QMemoryBuffer::QMemoryBuffer(QObject *parent) : QHexBuffer(parent) {}
uchar QMemoryBuffer::at(qint64 idx) { return uchar(m_buffer.at(int(idx))); }
qint64 QMemoryBuffer::length() const { return qint64(m_buffer.length()); }
void QMemoryBuffer::insert(qint64 offset, const QByteArray &data) {
  m_buffer.insert(int(offset), data);
}
void QMemoryBuffer::remove(qint64 offset, int length) {
  m_buffer.remove(int(offset), length);
}
QByteArray QMemoryBuffer::read(qint64 offset, int length) {
  return m_buffer.mid(int(offset), length);
}

bool QMemoryBuffer::read(QIODevice *device) {
  m_buffer = device->readAll();
  return true;
}
void QMemoryBuffer::write(QIODevice *device) { device->write(m_buffer); }

qint64 QMemoryBuffer::indexOf(const QByteArray &ba, qint64 from) {
  return m_buffer.indexOf(ba, int(from));
}
qint64 QMemoryBuffer::lastIndexOf(const QByteArray &ba, qint64 from) {
  return m_buffer.lastIndexOf(ba, int(from));
}
