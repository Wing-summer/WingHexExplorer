#ifndef QFILEREGIONBUFFER_H
#define QFILEREGIONBUFFER_H

#include "qhexbuffer.h"

class QFileRegionBuffer : public QHexBuffer {
  Q_OBJECT
public:
  explicit QFileRegionBuffer(QObject *parent = nullptr);

public:
  qint64 length() const override;
  void insert(qint64 offset, const QByteArray &data) override;
  void remove(qint64 offset, int length) override;
  QByteArray read(qint64 offset, int length) override;
  bool read(QIODevice *iodevice) override;
  void write(QIODevice *iodevice) override;

  qint64 indexOf(const QByteArray &ba, qint64 from) override;
  qint64 lastIndexOf(const QByteArray &ba, qint64 from) override;

  void setReadOffset(qint64 offset);
  qint64 readOffset();

  qint64 readMaxBytes();
  void setReadMaxBytes(qint64 maxlength);

private:
  qint64 offset = 0;
  qint64 maxlength = 1024;
  QByteArray buffer;
};

#endif // QFILEREGIONBUFFER_H
