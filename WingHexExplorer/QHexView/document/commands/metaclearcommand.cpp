#include "metaclearcommand.h"

MetaClearCommand::MetaClearCommand(QHexMetadata *hexmeta,
                                   QList<QHexMetadataAbsoluteItem> metas,
                                   QUndoCommand *parent)
    : QUndoCommand(parent), m_hexmeta(hexmeta), m_metas(metas) {}

void MetaClearCommand::redo() {
  for (auto item : m_metas)
    m_hexmeta->removeMetadata(item, true);
}

void MetaClearCommand::undo() {
  for (auto item : m_metas)
    m_hexmeta->metadata(item.begin, item.end, item.foreground, item.background,
                        item.comment, false);
}
