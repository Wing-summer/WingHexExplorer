#ifndef BOOKMARKREPLACECOMMAND_H
#define BOOKMARKREPLACECOMMAND_H

#include "bookmarkcommand.h"

class BookMarkReplaceCommand : public BookMarkCommand {
public:
  BookMarkReplaceCommand(QHexDocument *doc, qint64 pos, QString comment,
                         QString oldcomment, QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

protected:
  QString m_oldcomment;
};

#endif // BOOKMARKREPLACECOMMAND_H
