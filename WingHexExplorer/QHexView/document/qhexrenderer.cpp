#include "qhexrenderer.h"
#include "document/commands/encodingchangecommand.h"
#include <QApplication>
#include <QTextCodec>
#include <QTextCursor>
#include <QWidget>
#include <cctype>
#include <cmath>

#define HEX_UNPRINTABLE_CHAR '.'

/*===================================*/
// added by wingsummer

bool QHexRenderer::stringVisible() { return m_asciiVisible; }

void QHexRenderer::setStringVisible(bool b) {
  m_asciiVisible = b;
  m_document->documentChanged();
  m_document->setDocSaved(false);
}

bool QHexRenderer::headerVisible() { return m_headerVisible; }

void QHexRenderer::setHeaderVisible(bool b) {
  m_headerVisible = b;
  m_document->documentChanged();
  m_document->setDocSaved(false);
}

bool QHexRenderer::addressVisible() { return m_addressVisible; }

void QHexRenderer::setAddressVisible(bool b) {
  m_addressVisible = b;
  m_document->documentChanged();
  m_document->setDocSaved(false);
}

QString QHexRenderer::encoding() { return m_encoding; }

void QHexRenderer::SetEncoding(QString encoding) {
  m_document->addUndoCommand(
      new EncodingChangeCommand(this, m_encoding, encoding));
}

void QHexRenderer::switchDoc(QHexDocument *doc) {
  if (doc)
    m_document = doc;
}

bool QHexRenderer::setEncoding(QString encoding) {
  if (QTextCodec::codecForName(encoding.toUtf8())) {
    m_encoding = encoding;
    m_document->documentChanged();
    return true;
  }
  return false;
}
/*===================================*/

QHexRenderer::QHexRenderer(QHexDocument *document,
                           const QFontMetricsF &fontmetrics, QObject *parent)
    : QObject(parent), m_document(document), m_fontmetrics(fontmetrics),
      m_encoding("ASCII") {
  m_selectedarea = QHexRenderer::HexArea;
  m_cursorenabled = false;

  /*===================================*/
  // added by wingsummer

  m_asciiVisible = true;
  m_addressVisible = true;
  m_headerVisible = true;

  /*===================================*/
}

// modified by wingsummer
void QHexRenderer::renderFrame(QPainter *painter) {
  QRect rect = painter->window();
  int hexx = this->getHexColumnX();
  int asciix = this->getAsciiColumnX();
  int endx = this->getEndColumnX();

  // x coordinates are in absolute space
  // y coordinates are in viewport space
  // see QHexView::paintEvent where the painter has been shifted horizontally

  if (m_headerVisible)
    painter->drawLine(0, this->headerLineCount() * this->lineHeight() - 1, endx,
                      this->headerLineCount() * this->lineHeight() - 1);
  if (m_addressVisible)
    painter->drawLine(hexx, rect.top(), hexx, rect.bottom());

  painter->drawLine(asciix, rect.top(), asciix, rect.bottom());

  if (m_asciiVisible)
    painter->drawLine(endx, rect.top(), endx, rect.bottom());
}

// modified by wingsummer
void QHexRenderer::render(QPainter *painter, quint64 begin, quint64 end,
                          quint64 firstline) {
  QPalette palette = qApp->palette();

  if (m_headerVisible)
    this->drawHeader(painter, palette);

  quint64 documentLines = this->documentLines();
  for (quint64 line = begin; line < std::min(end, documentLines); line++) {
    QRect linerect = this->getLineRect(line, firstline);
    if (line % 2)
      painter->fillRect(linerect, palette.brush(QPalette::Window));
    else
      painter->fillRect(linerect, palette.brush(QPalette::Base));

    if (m_addressVisible)
      this->drawAddress(painter, palette, linerect, line);

    this->drawHex(painter, palette, linerect, line);

    if (m_asciiVisible)
      this->drawString(painter, palette, linerect, line, m_encoding);
  }
}

void QHexRenderer::updateMetrics(const QFontMetricsF &fm) {
  m_fontmetrics = fm;
}
void QHexRenderer::enableCursor(bool b) { m_cursorenabled = b; }

void QHexRenderer::selectArea(const QPoint &pt) {
  int area = this->hitTestArea(pt);
  if (!this->editableArea(area))
    return;

  m_selectedarea = area;
}

bool QHexRenderer::hitTest(const QPoint &pt, QHexPosition *position,
                           quint64 firstline) const {
  int area = this->hitTestArea(pt);
  if (!this->editableArea(area))
    return false;

  position->line = std::min(firstline + quint64(pt.y() / this->lineHeight()) -
                                quint64(headerLineCount()),
                            this->documentLastLine());
  position->lineWidth = quint8(this->hexLineWidth());

  if (area == QHexRenderer::HexArea) {
    int relx = pt.x() - this->getHexColumnX() - this->borderSize();
    int column = int(relx / this->getCellWidth());
    position->column = column / 3;
    // first char is nibble 1, 2nd and space are 0
    position->nibbleindex = (column % 3 == 0) ? 1 : 0;
  } else {
    int relx = pt.x() - this->getAsciiColumnX() - this->borderSize();
    position->column = int(relx / this->getCellWidth());
    position->nibbleindex = 1;
  }

  if (position->line == this->documentLastLine()) // Check last line's columns
  {
    QByteArray ba = this->getLine(position->line);
    position->column =
        std::min(position->column, static_cast<int>(ba.length()));
  } else
    position->column = std::min(position->column, hexLineWidth() - 1);

  return true;
}

int QHexRenderer::hitTestArea(const QPoint &pt) const {
  if (pt.y() < headerLineCount() * lineHeight())
    return QHexRenderer::HeaderArea;

  if ((pt.x() >= this->borderSize()) &&
      (pt.x() <= (this->getHexColumnX() - this->borderSize())))
    return QHexRenderer::AddressArea;

  if ((pt.x() > (this->getHexColumnX() + this->borderSize())) &&
      (pt.x() < (this->getAsciiColumnX() - this->borderSize())))
    return QHexRenderer::HexArea;

  if ((pt.x() > (this->getAsciiColumnX() + this->borderSize())) &&
      (pt.x() < (this->getEndColumnX() - this->borderSize())))
    return QHexRenderer::AsciiArea;

  return QHexRenderer::ExtraArea;
}

int QHexRenderer::selectedArea() const { return m_selectedarea; }
bool QHexRenderer::editableArea(int area) const {
  return (area == QHexRenderer::HexArea || area == QHexRenderer::AsciiArea);
}
quint64 QHexRenderer::documentLastLine() const {
  return this->documentLines() - 1;
}
int QHexRenderer::documentLastColumn() const {
  return this->getLine(this->documentLastLine()).length();
}
quint64 QHexRenderer::documentLines() const {
  return quint64(std::ceil(this->rendererLength() / float(hexLineWidth())));
}
int QHexRenderer::documentWidth() const { return this->getEndColumnX(); }
int QHexRenderer::lineHeight() const { return qRound(m_fontmetrics.height()); }

QRect QHexRenderer::getLineRect(quint64 line, quint64 firstline) const {
  return QRect(0,
               int((line - firstline + quint64(headerLineCount())) *
                   quint64(lineHeight())),
               this->getEndColumnX(), lineHeight());
}

// modified by wingsummer
int QHexRenderer::headerLineCount() const { return m_headerVisible ? 1 : 0; }

int QHexRenderer::borderSize() const {
  if (m_document)
    return this->getNCellsWidth(m_document->areaIndent());
  return this->getNCellsWidth(DEFAULT_AREA_IDENTATION);
}

int QHexRenderer::hexLineWidth() const {
  if (m_document)
    return m_document->hexLineWidth();
  return DEFAULT_HEX_LINE_LENGTH;
}

QString QHexRenderer::hexString(quint64 line, QByteArray *rawline) const {
  QByteArray lrawline = this->getLine(line);
  if (rawline)
    *rawline = lrawline;

  return lrawline.toHex(' ').toUpper() + " ";
}

// modified by wingsummer
QString QHexRenderer::decodeString(quint64 line, QString encoding,
                                   QByteArray *rawline) const {
  QByteArray lrawline = this->getLine(line);
  if (rawline)
    *rawline = lrawline;

  if (encoding.toLower() == "ascii") {
    QByteArray ascii = lrawline;
    this->unprintableChars(ascii);
    return ascii;
  } else {
    auto enc = QTextCodec::codecForName(encoding.toUtf8());
    auto d = enc->makeDecoder();
    auto unicode = d->toUnicode(lrawline);
    this->unprintableWChars(unicode);
    return unicode;
  }
}

QByteArray QHexRenderer::getLine(quint64 line) const {
  return m_document->read(qint64(line * quint64(hexLineWidth())),
                          hexLineWidth());
}
void QHexRenderer::blinkCursor() { m_cursorenabled = !m_cursorenabled; }
qint64 QHexRenderer::rendererLength() const { return m_document->length() + 1; }

// modified by wingsummer
int QHexRenderer::getAddressWidth() const {
  auto base = m_document->baseAddress();
  quint64 maxAddr = base + quint64(this->rendererLength());
  if (base <= 0xFFFFFFFF && maxAddr <= 0xFFFFFFFF)
    return 8;
  else
    return 16;
  return QString::number(maxAddr, 16).length();
}

int QHexRenderer::getHexColumnX() const {
  if (m_addressVisible) {
    return this->getNCellsWidth(this->getAddressWidth()) +
           2 * this->borderSize();
  }
  return 0;
}
int QHexRenderer::getAsciiColumnX() const {
  return this->getHexColumnX() + this->getNCellsWidth(hexLineWidth() * 3) +
         2 * this->borderSize();
}

int QHexRenderer::getEndColumnX() const {
  if (m_asciiVisible) {
    return this->getAsciiColumnX() + this->getNCellsWidth(hexLineWidth()) +
           2 * this->borderSize();
  }
  return this->getAsciiColumnX();
}

qreal QHexRenderer::getCellWidth() const {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  return m_fontmetrics.horizontalAdvance(" ");
#else
  return m_fontmetrics.width(" ");
#endif
}

int QHexRenderer::getNCellsWidth(int n) const {
  return qRound(n * getCellWidth());
}

void QHexRenderer::unprintableChars(QByteArray &ascii) const {
  for (char &ch : ascii) {
    if (std::isprint(static_cast<unsigned char>(ch)))
      continue;

    ch = HEX_UNPRINTABLE_CHAR;
  }
}

// added by wingsummer
void QHexRenderer::unprintableWChars(QString &unicode) const {
  for (QChar &ch : unicode) {
    if (std::iswprint(ch.unicode()))
      continue;
    ch = HEX_UNPRINTABLE_CHAR;
  }
}

void QHexRenderer::applyDocumentStyles(QPainter *painter,
                                       QTextDocument *textdocument) const {
  textdocument->setDocumentMargin(0);
  textdocument->setUndoRedoEnabled(false);
  textdocument->setDefaultFont(painter->font());
}

void QHexRenderer::applyBasicStyle(QTextCursor &textcursor,
                                   const QByteArray &rawline,
                                   Factor factor) const {
  QPalette palette = qApp->palette();
  QColor color = palette.color(QPalette::WindowText);

  if (color.lightness() < 50) {
    if (color == Qt::black)
      color = Qt::gray;
    else
      color = color.darker();
  } else
    color = color.lighter();

  QTextCharFormat charformat;
  charformat.setForeground(color);

  for (int i = 0; i < rawline.length(); i++) {
    if ((rawline[i] != 0x00) && (uchar(rawline[i]) != 0xFF))
      continue;

    textcursor.setPosition(i * factor);
    textcursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                            factor);
    textcursor.mergeCharFormat(charformat);
  }
}

void QHexRenderer::applyMetadata(QTextCursor &textcursor, quint64 line,
                                 Factor factor) const {
  QHexMetadata *metadata = m_document->metadata();

  if (!metadata->lineHasMetadata(line))
    return;

  const QHexLineMetadata &linemetadata = metadata->get(line);
  for (const QHexMetadataItem &mi : linemetadata) {
    QTextCharFormat charformat;
    if (m_document->metabgVisible() && mi.background.isValid() &&
        mi.background.rgba())
      charformat.setBackground(mi.background);
    if (m_document->metafgVisible() && mi.foreground.isValid() &&
        mi.foreground.rgba())
      charformat.setForeground(mi.foreground);
    if (m_document->metaCommentVisible() && !mi.comment.isEmpty())
      charformat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    textcursor.setPosition(mi.start * factor);
    textcursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                            (mi.length * factor) - (factor > 1 ? 1 : 0));
    textcursor.mergeCharFormat(charformat);
  }
}

void QHexRenderer::applySelection(QTextCursor &textcursor, quint64 line,
                                  Factor factor) const {
  QHexCursor *cursor = m_document->cursor();
  if (!cursor->isLineSelected(line))
    return;

  const QHexPosition &startsel = cursor->selectionStart();
  const QHexPosition &endsel = cursor->selectionEnd();

  if (startsel.line == endsel.line) {
    textcursor.setPosition(startsel.column * factor);
    textcursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                            ((endsel.column - startsel.column + 1) * factor));
  } else {
    if (line == startsel.line)
      textcursor.setPosition(startsel.column * factor);
    else
      textcursor.setPosition(0);

    if (line == endsel.line)
      textcursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                              ((endsel.column + 1) * factor));
    else
      textcursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
  }

  if (factor == Hex)
    textcursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);

  QPalette palette = qApp->palette();

  QTextCharFormat charformat;
  charformat.setBackground(palette.color(QPalette::Highlight));
  charformat.setForeground(palette.color(QPalette::HighlightedText));
  textcursor.mergeCharFormat(charformat);
}

void QHexRenderer::applyCursorAscii(QTextCursor &textcursor,
                                    quint64 line) const {
  QHexCursor *cursor = m_document->cursor();
  if ((line != cursor->currentLine()) || !m_cursorenabled)
    return;

  textcursor.clearSelection();
  textcursor.setPosition(m_document->cursor()->currentColumn());
  textcursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);

  QPalette palette = qApp->palette();
  QTextCharFormat charformat;

  if ((cursor->insertionMode() == QHexCursor::OverwriteMode) ||
      (m_selectedarea != QHexRenderer::AsciiArea)) {
    charformat.setForeground(palette.color(QPalette::Window));
    if (m_selectedarea == QHexRenderer::AsciiArea)
      charformat.setBackground(palette.color(QPalette::WindowText));
    else
      charformat.setBackground(
          palette.color(QPalette::WindowText).lighter(250));
  } else
    charformat.setUnderlineStyle(
        QTextCharFormat::UnderlineStyle::SingleUnderline);

  textcursor.mergeCharFormat(charformat);
}

void QHexRenderer::applyCursorHex(QTextCursor &textcursor, quint64 line) const {
  QHexCursor *cursor = m_document->cursor();
  if ((line != cursor->currentLine()) || !m_cursorenabled)
    return;

  textcursor.clearSelection();
  textcursor.setPosition(m_document->cursor()->currentColumn() * 3);

  if ((m_selectedarea == QHexRenderer::HexArea) &&
      !m_document->cursor()->currentNibble())
    textcursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor);

  textcursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);

  if (m_selectedarea == QHexRenderer::AsciiArea)
    textcursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);

  QPalette palette = qApp->palette();
  QTextCharFormat charformat;

  if ((cursor->insertionMode() == QHexCursor::OverwriteMode) ||
      (m_selectedarea != QHexRenderer::HexArea)) {
    charformat.setForeground(palette.color(QPalette::Window));
    if (m_selectedarea == QHexRenderer::HexArea)
      charformat.setBackground(palette.color(QPalette::WindowText));
    else
      charformat.setBackground(
          palette.color(QPalette::WindowText).lighter(250));
  } else
    charformat.setUnderlineStyle(
        QTextCharFormat::UnderlineStyle::SingleUnderline);

  textcursor.setCharFormat(charformat);
}

void QHexRenderer::drawAddress(QPainter *painter, const QPalette &palette,
                               const QRect &linerect, quint64 line) {
  quint64 addr = line * quint64(hexLineWidth()) + m_document->baseAddress();
  QString addrStr =
      QString::number(addr, 16)
          .rightJustified(this->getAddressWidth(), QLatin1Char('0'))
          .toUpper();

  QRect addressrect = linerect;
  addressrect.setWidth(this->getHexColumnX());

  painter->save();
  painter->setPen(palette.color(QPalette::Highlight));
  painter->drawText(addressrect, Qt::AlignHCenter | Qt::AlignVCenter, addrStr);
  painter->restore();
}

void QHexRenderer::drawHex(QPainter *painter, const QPalette &palette,
                           const QRect &linerect, quint64 line) {
  Q_UNUSED(palette)
  QTextDocument textdocument;
  QTextCursor textcursor(&textdocument);
  QByteArray rawline;

  textcursor.insertText(this->hexString(line, &rawline));

  if (line == this->documentLastLine())
    textcursor.insertText(" ");

  QRect hexrect = linerect;
  hexrect.setX(this->getHexColumnX() + this->borderSize());

  this->applyDocumentStyles(painter, &textdocument);
  this->applyBasicStyle(textcursor, rawline, Hex);

  auto dis = !m_document->metabgVisible() && !m_document->metafgVisible() &&
             !m_document->metaCommentVisible();
  if (!dis)
    this->applyMetadata(textcursor, line, Hex);

  this->applyBookMark(textcursor, line, Hex);
  this->applySelection(textcursor, line, Hex);
  this->applyCursorHex(textcursor, line);

  painter->save();
  painter->translate(hexrect.topLeft());
  textdocument.drawContents(painter);
  painter->restore();
}

void QHexRenderer::applyBookMark(QTextCursor &textcursor, quint64 line,
                                 Factor factor) {

  if (!m_document->lineHasBookMark(line))
    return;

  auto pos = m_document->getsBookmarkPos(line);
  for (auto item : pos) {
    textcursor.setPosition(int((item % hexLineWidth()) * factor) + 2);
    auto charformat = textcursor.charFormat();
    textcursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                            factor - 1);
    charformat.setFontOverline(true);

    if (charformat.underlineStyle() ==
        QTextCharFormat::UnderlineStyle::NoUnderline) {
      charformat.setUnderlineStyle(
          QTextCharFormat::UnderlineStyle::DashUnderline);
    } else {
      charformat.setUnderlineStyle(
          QTextCharFormat::UnderlineStyle::WaveUnderline);
    }

    textcursor.setCharFormat(charformat);
  }
}

void QHexRenderer::drawString(QPainter *painter, const QPalette &palette,
                              const QRect &linerect, quint64 line,
                              QString encoding) {
  Q_UNUSED(palette)
  QTextDocument textdocument;
  QTextCursor textcursor(&textdocument);
  QByteArray rawline;
  // modified by wingsummer
  textcursor.insertText(this->decodeString(line, encoding, &rawline));

  if (line == this->documentLastLine())
    textcursor.insertText(" ");

  QRect asciirect = linerect;
  asciirect.setX(this->getAsciiColumnX() + this->borderSize());

  this->applyDocumentStyles(painter, &textdocument);
  this->applyBasicStyle(textcursor, rawline, String);

  auto dis = !m_document->metabgVisible() && !m_document->metafgVisible() &&
             !m_document->metaCommentVisible();
  if (!dis)
    this->applyMetadata(textcursor, line, String);

  this->applySelection(textcursor, line, String);
  this->applyCursorAscii(textcursor, line);

  painter->save();
  painter->translate(asciirect.topLeft());
  textdocument.drawContents(painter);
  painter->restore();
}

void QHexRenderer::drawHeader(QPainter *painter, const QPalette &palette) {
  QRect rect = QRect(0, 0, this->getEndColumnX(),
                     this->headerLineCount() * this->lineHeight());
  QString hexheader;

  for (quint8 i = 0; i < this->hexLineWidth(); i++)
    hexheader.append(
        QString("%1 ")
            .arg(QString::number(i, 16).rightJustified(2, QChar('0')))
            .toUpper());

  QRect addressrect = rect;
  addressrect.setWidth(this->getHexColumnX());

  QRect hexrect = rect;

  hexrect.setX(m_addressVisible ? this->getHexColumnX() + this->borderSize()
                                : this->borderSize());
  hexrect.setWidth(this->getNCellsWidth(hexLineWidth() * 3));

  QRect asciirect = rect;
  asciirect.setX(this->getAsciiColumnX());
  asciirect.setWidth(this->getEndColumnX() - this->getAsciiColumnX());

  painter->save();
  painter->setPen(palette.color(QPalette::Highlight));

  if (m_addressVisible)
    painter->drawText(addressrect, Qt::AlignHCenter | Qt::AlignVCenter,
                      QString("Offset"));

  // align left for maximum consistency with drawHex() which prints from the
  // left. so hex and positions are aligned vertically
  painter->drawText(hexrect, Qt::AlignLeft | Qt::AlignVCenter, hexheader);

  if (m_asciiVisible)
    painter->drawText(asciirect, Qt::AlignHCenter | Qt::AlignVCenter,
                      m_encoding);
  painter->restore();
}
