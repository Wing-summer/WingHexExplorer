#include "qhexview.h"
#include "document/buffer/qmemorybuffer.h"
#include <QApplication>
#include <QFontDatabase>
#include <QHelpEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTextCursor>
#include <QToolTip>
#include <cmath>

#define CURSOR_BLINK_INTERVAL 500 // ms
#define DOCUMENT_WHEEL_LINES 3

/*======================*/
// added by wingsummer

QHexRenderer *QHexView::renderer() { return m_renderer; }

int QHexView::getWorkSpaceState(QHexDocument *doc, bool b) {
  if (doc->isWorkspace) {
    if (b)
      return 1;
    else
      return 0;
  } else {
    return -1;
  }
}

void QHexView::switchDocument(QHexDocument *document, QHexRenderer *renderer,
                              int vBarValue) {
  if (document && renderer) {

    if (m_document) {
      m_document->disconnect();
      m_document->cursor()->disconnect();
    }

    m_document = document;
    m_renderer = renderer;

    establishSignal(document);

    this->adjustScrollBars();
    this->viewport()->update();
    auto vbar = this->verticalScrollBar();
    if (vBarValue >= 0 && vBarValue <= vbar->maximum())
      vbar->setValue(vBarValue);

    m_blinktimer->start(); // let mouse blink
  }
}

bool QHexView::isReadOnly() { return m_document->isReadOnly(); }
bool QHexView::isKeepSize() { return m_document->isKeepSize(); }
bool QHexView::isLocked() { return m_document->isLocked(); }

bool QHexView::setLockedFile(bool b) {
  bool res = m_document->setLockedFile(b);
  emit documentLockedFile(b);
  return res;
}

bool QHexView::setKeepSize(bool b) {
  bool res = m_document->setKeepSize(b);
  emit documentKeepSize(b);
  return res;
}

quint64 QHexView::documentLines() { return m_renderer->documentLines(); }
quint64 QHexView::documentBytes() { return quint64(m_document->length()); }
quint64 QHexView::currentRow() {
  return quint64(m_document->cursor()->currentLine());
}
quint64 QHexView::currentColumn() {
  return quint64(m_document->cursor()->currentColumn());
}
quint64 QHexView::currentOffset() {
  return quint64(m_document->cursor()->position().offset());
}

quint64 QHexView::selectlength() {
  return quint64(m_document->cursor()->selectionLength());
}

bool QHexView::asciiVisible() { return m_renderer->asciiVisible(); }

bool QHexView::headerVisible() { return m_renderer->headerVisible(); }

bool QHexView::addressVisible() { return m_renderer->addressVisible(); }

void QHexView::setAsciiVisible(bool b) {
  m_renderer->setAsciiVisible(b);
  this->adjustScrollBars();
  this->viewport()->update();
}

void QHexView::setAddressVisible(bool b) {
  m_renderer->setAddressVisible(b);
  this->adjustScrollBars();
  this->viewport()->update();
}

void QHexView::setHeaderVisible(bool b) {
  m_renderer->setHeaderVisible(b);
  this->adjustScrollBars();
  this->viewport()->update();
}

quint64 QHexView::addressBase() { return m_document->baseAddress(); }

void QHexView::setAddressBase(quint64 base) {
  m_document->setBaseAddress(base);
}

bool QHexView::isSaved() { return m_document->isDocSaved(); }

QFont QHexView::getHexeditorFont() {
  QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  if (f.styleHint() != QFont::TypeWriter) {
    f.setFamily("Monospace"); // Force Monospaced font
    f.setStyleHint(QFont::TypeWriter);
  }
  return f;
}

void QHexView::establishSignal(QHexDocument *doc) {
  connect(doc, &QHexDocument::documentChanged, this, [&]() {
    this->adjustScrollBars();
    this->viewport()->update();
  });

  connect(doc->cursor(), &QHexCursor::positionChanged, this,
          &QHexView::moveToSelection);
  connect(doc->cursor(), &QHexCursor::insertionModeChanged, this,
          &QHexView::renderCurrentLine);
  connect(doc, &QHexDocument::canUndoChanged, this, &QHexView::canUndoChanged);
  connect(doc, &QHexDocument::canRedoChanged, this, &QHexView::canRedoChanged);
  connect(doc, &QHexDocument::documentSaved, this, &QHexView::documentSaved);
  connect(doc, &QHexDocument::bookMarkChanged, this,
          &QHexView::documentBookMarkChanged);
  connect(doc, &QHexDocument::metaLineChanged, this,
          &QHexView::documentmetaDataChanged);

  emit canUndoChanged(doc->canUndo());
  emit canRedoChanged(doc->canRedo());
  emit cursorLocationChanged();
  emit documentSwitched();
  emit documentBookMarkChanged();
  emit documentmetaDataChanged();
  emit documentSaved(doc->isDocSaved());
  emit documentKeepSize(doc->isKeepSize());
  emit documentLockedFile(doc->isLocked());
}

/*======================*/

QHexView::QHexView(QWidget *parent)
    : QAbstractScrollArea(parent), m_document(nullptr), m_renderer(nullptr) {
  QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);

  if (f.styleHint() != QFont::TypeWriter) {
    f.setFamily("Monospace"); // Force Monospaced font
    f.setStyleHint(QFont::TypeWriter);
  }

  this->setFont(f);
  this->setFocusPolicy(Qt::StrongFocus);
  this->setMouseTracking(true);
  this->verticalScrollBar()->setSingleStep(1);
  this->verticalScrollBar()->setPageStep(1);

  m_blinktimer = new QTimer(this);
  m_blinktimer->setInterval(CURSOR_BLINK_INTERVAL);

  connect(m_blinktimer, &QTimer::timeout, this, &QHexView::blinkCursor);

  this->setDocument(
      QHexDocument::fromMemory<QMemoryBuffer>(QByteArray(), false, this));
}

QHexDocument *QHexView::document() { return m_document; }

// modified by wingsummer
void QHexView::setDocument(QHexDocument *document) {

  // modified by wingsummer
  //  if (m_renderer)
  //    m_renderer->deleteLater();

  //  if (m_document)
  //    m_document->deleteLater();

  if (m_document) {
    m_document->disconnect();
    m_document->cursor()->disconnect();
  }

  m_document = document;
  m_renderer = new QHexRenderer(m_document, this->fontMetrics(), this);

  establishSignal(m_document);

  this->adjustScrollBars();
  this->viewport()->update();
}

bool QHexView::event(QEvent *e) {
  if (m_renderer && (e->type() == QEvent::FontChange)) {
    m_renderer->updateMetrics(QFontMetricsF(this->font()));
    return true;
  }

  if (m_document && m_renderer && (e->type() == QEvent::ToolTip)) {
    QHelpEvent *helpevent = static_cast<QHelpEvent *>(e);
    QHexPosition position;

    QPoint abspos = absolutePosition(helpevent->pos());
    if (m_renderer->hitTest(abspos, &position, this->firstVisibleLine())) {
      QString comments =
          m_document->metadata()->comments(position.line, position.column);

      if (!comments.isEmpty())
        QToolTip::showText(helpevent->globalPos(), comments, this);
    }

    return true;
  }

  return QAbstractScrollArea::event(e);
}

void QHexView::keyPressEvent(QKeyEvent *e) {
  if (!m_renderer || !m_document) {
    QAbstractScrollArea::keyPressEvent(e);
    return;
  }

  QHexCursor *cur = m_document->cursor();

  m_blinktimer->stop();
  m_renderer->enableCursor();

  bool handled = this->processMove(cur, e);
  if (!handled)
    handled = this->processAction(cur, e);
  if (!handled)
    handled = this->processTextInput(cur, e);
  if (!handled)
    QAbstractScrollArea::keyPressEvent(e);

  m_blinktimer->start();
}

QPoint QHexView::absolutePosition(const QPoint &pos) const {
  QPoint shift(horizontalScrollBar()->value(), 0);
  return pos + shift;
}

void QHexView::mousePressEvent(QMouseEvent *e) {
  QAbstractScrollArea::mousePressEvent(e);

  if (!m_renderer || (e->buttons() != Qt::LeftButton))
    return;

  QHexPosition position;
  QPoint abspos = absolutePosition(e->pos());

  if (!m_renderer->hitTest(abspos, &position, this->firstVisibleLine()))
    return;

  m_renderer->selectArea(abspos);

  if (m_renderer->editableArea(m_renderer->selectedArea()))
    m_document->cursor()->moveTo(position);

  e->accept();
}

void QHexView::mouseMoveEvent(QMouseEvent *e) {
  QAbstractScrollArea::mouseMoveEvent(e);
  if (!m_renderer || !m_document)
    return;

  QPoint abspos = absolutePosition(e->pos());

  if (e->buttons() == Qt::LeftButton) {
    if (m_blinktimer->isActive()) {
      m_blinktimer->stop();
      m_renderer->enableCursor(false);
    }

    QHexCursor *cursor = m_document->cursor();
    QHexPosition position;

    if (!m_renderer->hitTest(abspos, &position, this->firstVisibleLine()))
      return;

    cursor->select(position.line, position.column, 0);
    e->accept();
  }

  if (e->buttons() != Qt::NoButton)
    return;

  int hittest = m_renderer->hitTestArea(abspos);

  if (m_renderer->editableArea(hittest))
    this->setCursor(Qt::IBeamCursor);
  else
    this->setCursor(Qt::ArrowCursor);
}

void QHexView::mouseReleaseEvent(QMouseEvent *e) {
  QAbstractScrollArea::mouseReleaseEvent(e);
  if (e->button() != Qt::LeftButton)
    return;
  if (!m_blinktimer->isActive())
    m_blinktimer->start();
  e->accept();
}

void QHexView::focusInEvent(QFocusEvent *e) {
  QAbstractScrollArea::focusInEvent(e);
  if (!m_renderer)
    return;

  m_renderer->enableCursor();
  m_blinktimer->start();
}

void QHexView::focusOutEvent(QFocusEvent *e) {
  QAbstractScrollArea::focusOutEvent(e);
  if (!m_renderer)
    return;

  m_blinktimer->stop();
  m_renderer->enableCursor(true); // modified by wingsummer : false -> true
}

void QHexView::wheelEvent(QWheelEvent *e) {
  if (e->angleDelta().y() == 0) {
    int value = this->verticalScrollBar()->value();

    if (e->angleDelta().x() < 0) // Scroll Down
      this->verticalScrollBar()->setValue(value + DOCUMENT_WHEEL_LINES);
    else if (e->angleDelta().x() > 0) // Scroll Up
      this->verticalScrollBar()->setValue(value - DOCUMENT_WHEEL_LINES);

    return;
  }

  QAbstractScrollArea::wheelEvent(e);
}

void QHexView::resizeEvent(QResizeEvent *e) {
  QAbstractScrollArea::resizeEvent(e);
  this->adjustScrollBars();
}

void QHexView::paintEvent(QPaintEvent *e) {
  if (!m_document)
    return;

  QPainter painter(this->viewport());
  painter.setFont(this->font());

  const QRect &r = e->rect();

  const quint64 firstVisible = this->firstVisibleLine();
  const int lineHeight = m_renderer->lineHeight();
  const int headerCount = m_renderer->headerLineCount();

  // these are lines from the point of view of the visible rect
  // where the first "headerCount" are taken by the header
  const int first = (r.top() / lineHeight);              // included
  const int lastPlusOne = (r.bottom() / lineHeight) + 1; // excluded

  // compute document lines, adding firstVisible and removing the header
  // the max is necessary if the rect covers the header
  const quint64 begin =
      firstVisible + quint64(std::max(first - headerCount, 0));
  const quint64 end =
      firstVisible + quint64(std::max(lastPlusOne - headerCount, 0));

  painter.save();
  painter.translate(-this->horizontalScrollBar()->value(), 0);
  m_renderer->render(&painter, begin, end, firstVisible);
  m_renderer->renderFrame(&painter);
  painter.restore();
}

void QHexView::moveToSelection() {
  QHexCursor *cur = m_document->cursor();

  if (!this->isLineVisible(cur->currentLine())) {
    QScrollBar *vscrollbar = this->verticalScrollBar();
    int scrollPos = static_cast<int>(
        std::max(quint64(0), (cur->currentLine() - this->visibleLines() / 2)) /
        quint64(documentSizeFactor()));
    vscrollbar->setValue(scrollPos);
  } else
    this->viewport()->update();

  emit cursorLocationChanged(); // added by wingsummer
}

void QHexView::blinkCursor() {
  if (!m_renderer)
    return;
  m_renderer->blinkCursor();
  this->renderCurrentLine();
}

void QHexView::moveNext(bool select) {
  QHexCursor *cur = m_document->cursor();
  quint64 line = cur->currentLine();
  int column = cur->currentColumn();
  bool lastcell = (line >= m_renderer->documentLastLine()) &&
                  (column >= m_renderer->documentLastColumn());

  if ((m_renderer->selectedArea() == QHexRenderer::AsciiArea) && lastcell)
    return;

  int nibbleindex = cur->currentNibble();

  if (m_renderer->selectedArea() == QHexRenderer::HexArea) {
    if (lastcell && !nibbleindex)
      return;

    if (cur->position().offset() >= m_document->length() && nibbleindex)
      return;
  }

  if ((m_renderer->selectedArea() == QHexRenderer::HexArea)) {
    nibbleindex++;
    nibbleindex %= 2;

    if (nibbleindex)
      column++;
  } else {
    nibbleindex = 1;
    column++;
  }

  if (column > m_renderer->hexLineWidth() - 1) {
    line = std::min(m_renderer->documentLastLine(), line + 1);
    column = 0;
    nibbleindex = 1;
  }

  if (select)
    cur->select(line, std::min(m_renderer->hexLineWidth() - 1, column),
                nibbleindex);
  else
    cur->moveTo(line, std::min(m_renderer->hexLineWidth() - 1, column),
                nibbleindex);
}

void QHexView::movePrevious(bool select) {
  QHexCursor *cur = m_document->cursor();
  quint64 line = cur->currentLine();
  int column = cur->currentColumn();
  bool firstcell = !line && !column;

  if ((m_renderer->selectedArea() == QHexRenderer::AsciiArea) && firstcell)
    return;

  int nibbleindex = cur->currentNibble();

  if ((m_renderer->selectedArea() == QHexRenderer::HexArea) && firstcell &&
      nibbleindex)
    return;

  if ((m_renderer->selectedArea() == QHexRenderer::HexArea)) {
    nibbleindex--;
    nibbleindex %= 2;
    if (!nibbleindex)
      column--;
  } else {
    nibbleindex = 1;
    column--;
  }

  if (column < 0) {
    line = std::max(quint64(0), line - 1);
    column = m_renderer->hexLineWidth() - 1;
    nibbleindex = 0;
  }

  if (select)
    cur->select(line, std::max(0, column), nibbleindex);
  else
    cur->moveTo(line, std::max(0, column), nibbleindex);
}

void QHexView::renderCurrentLine() {
  if (m_document)
    this->renderLine(m_document->cursor()->currentLine());
}

bool QHexView::processAction(QHexCursor *cur, QKeyEvent *e) {
  if (isReadOnly() || isLocked())
    return false;

  if (e->modifiers() != Qt::NoModifier) {
    if (e->matches(QKeySequence::SelectAll)) {
      m_document->cursor()->moveTo(0, 0);
      m_document->cursor()->select(m_renderer->documentLastLine(),
                                   m_renderer->documentLastColumn() - 1);
    } else if (e->matches(QKeySequence::Undo))
      m_document->undo();
    else if (e->matches(QKeySequence::Redo))
      m_document->redo();
    else if (e->matches(QKeySequence::Cut))
      m_document->Cut((m_renderer->selectedArea() == QHexRenderer::HexArea));
    else if (e->matches(QKeySequence::Copy))
      m_document->copy((m_renderer->selectedArea() == QHexRenderer::HexArea));
    else if (e->matches(QKeySequence::Paste))
      m_document->Paste((m_renderer->selectedArea() == QHexRenderer::HexArea));
    else
      return false;

    return true;
  }

  if ((e->key() == Qt::Key_Backspace) || (e->key() == Qt::Key_Delete)) {
    if (!cur->hasSelection()) {
      const QHexPosition &pos = cur->position();

      if (pos.offset() <= 0)
        return true;

      // modified by wingsummer
      if (isKeepSize()) {
        if (e->key() == Qt::Key_Backspace)
          m_document->Replace(cur->position().offset() - 1, uchar(0));
        else
          m_document->Replace(cur->position().offset(), uchar(0));
      } else {
        if (e->key() == Qt::Key_Backspace)
          m_document->Remove(cur->position().offset() - 1, 1);
        else
          m_document->Remove(cur->position().offset(), 1);
      }

    } else {
      QHexPosition oldpos = cur->selectionStart();
      m_document->RemoveSelection();
      cur->moveTo(oldpos.line, oldpos.column + 1);
    }

    if (e->key() == Qt::Key_Backspace) {
      if (m_renderer->selectedArea() == QHexRenderer::HexArea)
        this->movePrevious();

      this->movePrevious();
    }
  } else if (e->key() == Qt::Key_Insert)
    cur->switchInsertionMode();
  else
    return false;

  return true;
}

bool QHexView::processMove(QHexCursor *cur, QKeyEvent *e) {
  if (e->matches(QKeySequence::MoveToNextChar) ||
      e->matches(QKeySequence::SelectNextChar))
    this->moveNext(e->matches(QKeySequence::SelectNextChar));
  else if (e->matches(QKeySequence::MoveToPreviousChar) ||
           e->matches(QKeySequence::SelectPreviousChar))
    this->movePrevious(e->matches(QKeySequence::SelectPreviousChar));
  else if (e->matches(QKeySequence::MoveToNextLine) ||
           e->matches(QKeySequence::SelectNextLine)) {
    if (m_renderer->documentLastLine() == cur->currentLine())
      return true;

    quint64 nextline = cur->currentLine() + 1;

    if (e->matches(QKeySequence::MoveToNextLine))
      cur->moveTo(nextline, cur->currentColumn());
    else
      cur->select(nextline, cur->currentColumn());
  } else if (e->matches(QKeySequence::MoveToPreviousLine) ||
             e->matches(QKeySequence::SelectPreviousLine)) {
    if (!cur->currentLine())
      return true;

    quint64 prevline = cur->currentLine() - 1;

    if (e->matches(QKeySequence::MoveToPreviousLine))
      cur->moveTo(prevline, cur->currentColumn());
    else
      cur->select(prevline, cur->currentColumn());
  } else if (e->matches(QKeySequence::MoveToNextPage) ||
             e->matches(QKeySequence::SelectNextPage)) {
    if (m_renderer->documentLastLine() == cur->currentLine())
      return true;

    quint64 pageline = std::min(m_renderer->documentLastLine(),
                                cur->currentLine() + this->visibleLines());

    if (e->matches(QKeySequence::MoveToNextPage))
      cur->moveTo(pageline, cur->currentColumn());
    else
      cur->select(pageline, cur->currentColumn());
  } else if (e->matches(QKeySequence::MoveToPreviousPage) ||
             e->matches(QKeySequence::SelectPreviousPage)) {
    if (!cur->currentLine())
      return true;

    quint64 pageline =
        std::max(quint64(0), cur->currentLine() - this->visibleLines());

    if (e->matches(QKeySequence::MoveToPreviousPage))
      cur->moveTo(pageline, cur->currentColumn());
    else
      cur->select(pageline, cur->currentColumn());
  } else if (e->matches(QKeySequence::MoveToStartOfDocument) ||
             e->matches(QKeySequence::SelectStartOfDocument)) {
    if (!cur->currentLine())
      return true;

    if (e->matches(QKeySequence::MoveToStartOfDocument))
      cur->moveTo(0, 0);
    else
      cur->select(0, 0);
  } else if (e->matches(QKeySequence::MoveToEndOfDocument) ||
             e->matches(QKeySequence::SelectEndOfDocument)) {
    if (m_renderer->documentLastLine() == cur->currentLine())
      return true;

    if (e->matches(QKeySequence::MoveToEndOfDocument))
      cur->moveTo(m_renderer->documentLastLine(),
                  m_renderer->documentLastColumn());
    else
      cur->select(m_renderer->documentLastLine(),
                  m_renderer->documentLastColumn());
  } else if (e->matches(QKeySequence::MoveToStartOfLine) ||
             e->matches(QKeySequence::SelectStartOfLine)) {
    if (e->matches(QKeySequence::MoveToStartOfLine))
      cur->moveTo(cur->currentLine(), 0);
    else
      cur->select(cur->currentLine(), 0);
  } else if (e->matches(QKeySequence::MoveToEndOfLine) ||
             e->matches(QKeySequence::SelectEndOfLine)) {
    if (e->matches(QKeySequence::MoveToEndOfLine)) {
      if (cur->currentLine() == m_renderer->documentLastLine())
        cur->moveTo(cur->currentLine(), m_renderer->documentLastColumn());
      else
        cur->moveTo(cur->currentLine(), m_renderer->hexLineWidth() - 1, 0);
    } else {
      if (cur->currentLine() == m_renderer->documentLastLine())
        cur->select(cur->currentLine(), m_renderer->documentLastColumn());
      else
        cur->select(cur->currentLine(), m_renderer->hexLineWidth() - 1, 0);
    }
  } else
    return false;

  return true;
}

bool QHexView::processTextInput(QHexCursor *cur, QKeyEvent *e) {
  if (isReadOnly() || isLocked() || (e->modifiers() & Qt::ControlModifier))
    return false;

  uchar key = static_cast<uchar>(e->text()[0].toLatin1());

  if ((m_renderer->selectedArea() == QHexRenderer::HexArea)) {
    if (!((key >= '0' && key <= '9') ||
          (key >= 'a' && key <= 'f'))) // Check if is a Hex Char
      return false;

    uchar val = uchar(QString(static_cast<char>(key)).toUInt(nullptr, 16));
    m_document->RemoveSelection();

    if (m_document->atEnd() ||
        (cur->currentNibble() && !isKeepSize() &&
         cur->insertionMode() == QHexCursor::InsertMode)) {
      m_document->Insert(cur->position().offset(), uchar(val << 4)); // X0 byte
      this->moveNext();
      return true;
    }

    uchar ch = uchar(m_document->at(int(cur->position().offset())));

    if (cur->currentNibble()) // X0
      val = uchar((ch & 0x0F) | (val << 4));
    else // 0X
      val = (ch & 0xF0) | val;

    m_document->Replace(cur->position().offset(), val);
    this->moveNext();
    return true;
  }

  if ((m_renderer->selectedArea() == QHexRenderer::AsciiArea)) {
    if (!(key >= 0x20 && key <= 0x7E)) // Check if is a Printable Char
      return false;

    m_document->RemoveSelection();

    if (!m_document->atEnd() &&
        (cur->insertionMode() == QHexCursor::OverwriteMode))
      m_document->Replace(cur->position().offset(), key);
    else
      m_document->Insert(cur->position().offset(), key);

    QKeyEvent keyevent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    qApp->sendEvent(this, &keyevent);
    return true;
  }

  return false;
}

void QHexView::adjustScrollBars() {
  QScrollBar *vscrollbar = this->verticalScrollBar();
  int sizeFactor = this->documentSizeFactor();
  vscrollbar->setSingleStep(sizeFactor);

  quint64 docLines = m_renderer->documentLines();
  quint64 visLines = this->visibleLines();

  // modified by wingsummer,fix the scrollbar bug
  if (docLines > visLines && !m_document->isEmpty()) {
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    vscrollbar->setMaximum(int((docLines - visLines) / uint(sizeFactor) + 1));
  } else {
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vscrollbar->setValue(0);
    vscrollbar->setMaximum(0);
  }

  QScrollBar *hscrollbar = this->horizontalScrollBar();
  int documentWidth = m_renderer->documentWidth();
  int viewportWidth = viewport()->width();

  if (documentWidth > viewportWidth) {
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    // +1 to see the rightmost vertical line, +2 seems more pleasant to the eyes
    hscrollbar->setMaximum(documentWidth - viewportWidth + 2);
  } else {
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    hscrollbar->setValue(0);
    hscrollbar->setMaximum(documentWidth);
  }
}

void QHexView::renderLine(quint64 line) {
  if (!this->isLineVisible(line))
    return;
  this->viewport()->update(
      /*m_renderer->getLineRect(line, this->firstVisibleLine())*/);
}

quint64 QHexView::firstVisibleLine() const {
  return quint64(this->verticalScrollBar()->value()) *
         uint(documentSizeFactor());
}
quint64 QHexView::lastVisibleLine() const {
  return this->firstVisibleLine() + this->visibleLines() - 1;
}

quint64 QHexView::visibleLines() const {
  quint64 visLines =
      quint64(std::ceil(this->height() / m_renderer->lineHeight()) -
              m_renderer->headerLineCount());
  return std::min(visLines, m_renderer->documentLines());
}

bool QHexView::isLineVisible(quint64 line) const {
  if (!m_document)
    return false;
  if (line < this->firstVisibleLine())
    return false;
  if (line > this->lastVisibleLine())
    return false;
  return true;
}

int QHexView::documentSizeFactor() const {
  int factor = 1;

  if (m_document) {
    quint64 docLines = m_renderer->documentLines();
    if (docLines >= INT_MAX)
      factor = int(docLines / INT_MAX) + 1;
  }

  return factor;
}
