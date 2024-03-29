#include "chunks.h"
#include <limits.h>

#define BUFFER_SIZE 0x10000
#define CHUNK_SIZE 0x1000
#define READ_CHUNK_MASK Q_INT64_C(0xfffffffffffff000)

/*this file is modified by wingsummer in order to fit the QHexView*/

// ***************************************** Constructors and file settings

Chunks::Chunks(QObject *parent) : QObject(parent) {}

Chunks::Chunks(QIODevice *ioDevice, QObject *parent) : QObject(parent) {
  setIODevice(ioDevice);
  if (ioDevice)
    ioDevice->setParent(this);
}

Chunks::~Chunks() {}

bool Chunks::setIODevice(QIODevice *ioDevice) {
  _ioDevice = ioDevice;
  if (_ioDevice &&
      (_ioDevice->isOpen() ||
       _ioDevice->open(QIODevice::ReadOnly))) // Try to open IODevice
  {
    _size = _ioDevice->size();
    _ioDevice->close();
  } else // Fallback is an empty buffer
  {
    QBuffer *buf = new QBuffer(this);
    _ioDevice = buf;
    _size = 0;
  }
  _chunks.clear();
  _pos = 0;
  return true;
}

// ***************************************** Getting data out of Chunks

QByteArray Chunks::data(qint64 pos, qint64 maxSize) {
  qint64 ioDelta = 0;
  int chunkIdx = 0;

  Chunk chunk;
  QByteArray buffer;

  // Do some checks and some arrangements

  if (pos >= _size)
    return buffer;

  if (maxSize < 0)
    maxSize = _size;
  else if ((pos + maxSize) > _size)
    maxSize = _size - pos;

  _ioDevice->open(QIODevice::ReadOnly);

  while (maxSize > 0) {
    chunk.absPos = LLONG_MAX;
    bool chunksLoopOngoing = true;
    while ((chunkIdx < _chunks.count()) && chunksLoopOngoing) {
      // In this section, we track changes before our required data and
      // we take the editdet data, if availible. ioDelta is a difference
      // counter to justify the read pointer to the original data, if
      // data in between was deleted or inserted.

      chunk = _chunks[chunkIdx];
      if (chunk.absPos > pos)
        chunksLoopOngoing = false;
      else {
        chunkIdx += 1;
        qint64 count;
        qint64 chunkOfs = qint64(pos - chunk.absPos);
        if (maxSize > (chunk.data.size() - chunkOfs)) {
          count = qint64(chunk.data.size()) - chunkOfs;
          ioDelta += CHUNK_SIZE - quint64(chunk.data.size());
        } else
          count = maxSize;
        if (count > 0) {
          buffer += chunk.data.mid(int(chunkOfs), int(count));
          maxSize -= count;
          pos += quint64(count);
        }
      }
    }

    if ((maxSize > 0) && (pos < chunk.absPos)) {
      // In this section, we read data from the original source. This only will
      // happen, whe no copied data is available

      qint64 byteCount;
      QByteArray readBuffer;
      if (chunk.absPos - pos > maxSize)
        byteCount = maxSize;
      else
        byteCount = chunk.absPos - pos;

      maxSize -= byteCount;
      _ioDevice->seek(pos + ioDelta);
      readBuffer = _ioDevice->read(byteCount);
      buffer += readBuffer;
      pos += quint64(readBuffer.size());
    }
  }
  _ioDevice->close();
  return buffer;
}

bool Chunks::write(QIODevice *iODevice, qint64 pos, qint64 count) {
  if (count == -1)
    count = qint64(_size);

  // fix the bug
  bool ok = (iODevice->isOpen() && iODevice->isWritable()) ||
            iODevice->open(QIODevice::WriteOnly);
  if (ok) {
    for (qint64 idx = pos; idx < count; idx += BUFFER_SIZE) {
      QByteArray ba = data(idx, BUFFER_SIZE);
      iODevice->write(ba);
    }
    iODevice->close();
  }
  return ok;
}

// ***************************************** Search API

qint64 Chunks::indexOf(const QByteArray &ba, qint64 from) {
  qint64 result = -1;
  QByteArray buffer;

  for (qint64 pos = from; (pos < _size) && (result < 0); pos += BUFFER_SIZE) {
    buffer = data(pos, BUFFER_SIZE + ba.size() - 1);
    int findPos = buffer.indexOf(ba);
    if (findPos >= 0)
      result = pos + findPos;
  }
  return result;
}

qint64 Chunks::lastIndexOf(const QByteArray &ba, qint64 from) {
  qint64 result = -1;
  QByteArray buffer;

  for (qint64 pos = from; (pos > 0) && (result < 0); pos -= BUFFER_SIZE) {
    qint64 sPos = pos - BUFFER_SIZE - ba.size() + 1;
    /*if (sPos < 0)
      sPos = 0;*/
    buffer = data(sPos, pos - sPos);
    int findPos = buffer.lastIndexOf(ba);
    if (findPos >= 0)
      result = sPos + findPos;
  }
  return result;
}

// ***************************************** Char manipulations

bool Chunks::insert(qint64 pos, char b) {
  if (pos > _size)
    return false;
  int chunkIdx;
  if (pos == _size) {
    chunkIdx = getChunkIndex(pos - 1);
  } else
    chunkIdx = getChunkIndex(pos);
  qint64 posInBa = pos - _chunks[chunkIdx].absPos;
  _chunks[chunkIdx].data.insert(int(posInBa), b);
  _chunks[chunkIdx].dataChanged.insert(int(posInBa), char(1));
  for (int idx = chunkIdx + 1; idx < _chunks.size(); idx++)
    _chunks[idx].absPos += 1;
  _size += 1;
  _pos = pos;
  return true;
}

bool Chunks::overwrite(qint64 pos, char b) {
  if (pos >= _size)
    return false;
  int chunkIdx = getChunkIndex(pos);
  qint64 posInBa = pos - _chunks[chunkIdx].absPos;
  _chunks[chunkIdx].data[int(posInBa)] = b;
  _chunks[chunkIdx].dataChanged[int(posInBa)] = char(1);
  _pos = pos;
  return true;
}

bool Chunks::removeAt(qint64 pos) {
  if (pos >= _size)
    return false;
  int chunkIdx = getChunkIndex(pos);
  qint64 posInBa = pos - _chunks[chunkIdx].absPos;
  _chunks[chunkIdx].data.remove(int(posInBa), 1);
  _chunks[chunkIdx].dataChanged.remove(int(posInBa), 1);
  for (int idx = chunkIdx + 1; idx < _chunks.size(); idx++)
    _chunks[idx].absPos -= 1;
  _size -= 1;
  _pos = pos;
  return true;
}

// ***************************************** Utility functions

char Chunks::operator[](qint64 pos) {
  auto d = data(pos, 1);
  if (d.isEmpty())
    return '0';
  return d.at(0);
}

qint64 Chunks::pos() { return _pos; }

qint64 Chunks::size() { return _size; }

int Chunks::getChunkIndex(qint64 absPos) {
  // This routine checks, if there is already a copied chunk available. If so,
  // it returns a reference to it. If there is no copied chunk available,
  // original data will be copied into a new chunk.

  int foundIdx = -1;
  int insertIdx = 0;
  qint64 ioDelta = 0;

  // fix the bug by wingsummer
  if (absPos < 0) {
    Chunk newChunk;
    newChunk.data = QByteArray(CHUNK_SIZE, 0);
    newChunk.absPos = 0;
    newChunk.dataChanged = nullptr;
    _chunks.insert(insertIdx, newChunk);
    return insertIdx;
  }

  for (int idx = 0; idx < _chunks.size(); idx++) {
    Chunk chunk = _chunks[idx];
    if ((absPos >= chunk.absPos) &&
        (absPos < (chunk.absPos + chunk.data.size()))) {
      foundIdx = idx;
      break;
    }
    if (absPos < chunk.absPos) {
      insertIdx = idx;
      break;
    }
    ioDelta += uint(chunk.data.size()) - CHUNK_SIZE;
    insertIdx = idx + 1;
  }

  if (foundIdx == -1) {
    Chunk newChunk;
    qint64 readAbsPos = absPos - ioDelta;
    qint64 readPos = (readAbsPos & READ_CHUNK_MASK);
    _ioDevice->open(QIODevice::ReadOnly);
    _ioDevice->seek(qint64(readPos));
    newChunk.data = _ioDevice->read(CHUNK_SIZE);
    _ioDevice->close();
    newChunk.absPos = absPos - (readAbsPos - readPos);
    newChunk.dataChanged = QByteArray(newChunk.data.size(), char(0));
    _chunks.insert(insertIdx, newChunk);
    foundIdx = insertIdx;
  }
  return foundIdx;
}
