#ifndef BOOKMARKADDCOMMAND_H
#define BOOKMARKADDCOMMAND_H

#include "bookmarkcommand.h"

class BookMarkAddCommand : public BookMarkCommand {
public:
  BookMarkAddCommand(QHexDocument *doc, qint64 pos, QString comment,
                     QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;
};

#endif // BOOKMARKADDCOMMAND_H
