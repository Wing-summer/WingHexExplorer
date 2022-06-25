#ifndef HEXCOMMAND_H
#define HEXCOMMAND_H

#include "../../buffer/qhexbuffer.h"
#include <QUndoCommand>

class HexCommand : public QUndoCommand {
public:
  HexCommand(QHexBuffer *buffer, QUndoCommand *parent = nullptr);

protected:
  QHexBuffer *m_buffer;
  qint64 m_offset;
  int m_length;
  QByteArray m_data;
};

#endif // HEXCOMMAND_H
