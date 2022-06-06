#ifndef QHEXRENDERER_H
#define QHEXRENDERER_H

/*
 * Nibble encoding:
 *           AB -> [A][B]
 * Nibble Index:    1  0
 */

#include "qhexdocument.h"
#include <QPainter>
#include <QTextDocument>

class QHexRenderer : public QObject {
  Q_OBJECT

public:
  enum { HeaderArea, AddressArea, HexArea, AsciiArea, ExtraArea };

public:
  explicit QHexRenderer(QHexDocument *document,
                        const QFontMetricsF &fontmetrics,
                        QObject *parent = nullptr);
  void renderFrame(QPainter *painter);
  void render(QPainter *painter, quint64 start, quint64 end,
              quint64 firstline); // begin included, end excluded
  void updateMetrics(const QFontMetricsF &fm);
  void enableCursor(bool b = true);
  void selectArea(const QPoint &pt);

  /*==============================*/
  // added by wingsummer

  void setAsciiVisible(bool b);
  bool asciiVisible();
  void setAddressVisible(bool b);
  bool addressVisible();
  void setHeaderVisible(bool b);
  bool headerVisible();

  /*==============================*/

public:
  void blinkCursor();
  bool hitTest(const QPoint &pt, QHexPosition *position,
               quint64 firstline) const;
  int hitTestArea(const QPoint &pt) const;
  int selectedArea() const;
  bool editableArea(int area) const;
  quint64 documentLastLine() const;
  int documentLastColumn() const;
  quint64 documentLines() const;
  int documentWidth() const;
  int lineHeight() const;
  QRect getLineRect(quint64 line, quint64 firstline) const;
  int headerLineCount() const;
  int borderSize() const;
  int hexLineWidth() const;

private:
  QString hexString(quint64 line, QByteArray *rawline = nullptr) const;
  QString asciiString(quint64 line, QByteArray *rawline = nullptr) const;
  QByteArray getLine(quint64 line) const;
  qint64 rendererLength() const;
  int getAddressWidth() const;
  int getHexColumnX() const;
  int getAsciiColumnX() const;
  int getEndColumnX() const;
  qreal getCellWidth() const;
  int getNCellsWidth(int n) const;
  void unprintableChars(QByteArray &ascii) const;

private:
  enum Factor { Ascii = 1, Hex = 3 };

  void applyDocumentStyles(QPainter *painter,
                           QTextDocument *textdocument) const;
  void applyBasicStyle(QTextCursor &textcursor, const QByteArray &rawline,
                       Factor factor) const;
  void applyMetadata(QTextCursor &textcursor, quint64 line,
                     Factor factor) const;
  void applySelection(QTextCursor &textcursor, quint64 line,
                      Factor factor) const;
  void applyCursorAscii(QTextCursor &textcursor, quint64 line) const;
  void applyCursorHex(QTextCursor &textcursor, quint64 line) const;
  void drawAddress(QPainter *painter, const QPalette &palette,
                   const QRect &linerect, quint64 line);
  void drawHex(QPainter *painter, const QPalette &palette,
               const QRect &linerect, quint64 line);
  void drawAscii(QPainter *painter, const QPalette &palette,
                 const QRect &linerect, quint64 line);
  void drawHeader(QPainter *painter, const QPalette &palette);

private:
  QHexDocument *m_document;
  QFontMetricsF m_fontmetrics;
  int m_selectedarea;
  bool m_cursorenabled;

  /*==============================*/
  // added by wingsummer

  bool m_asciiVisible;
  bool m_addressVisible;
  bool m_headerVisible;

  /*==============================*/
};

#endif // QHEXRENDERER_H
