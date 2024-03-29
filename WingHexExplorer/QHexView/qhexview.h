#ifndef QHEXVIEW_H
#define QHEXVIEW_H

#define QHEXVIEW_VERSION 3.0

#include "document/qhexdocument.h"
#include "document/qhexrenderer.h"
#include <QAbstractScrollArea>
#include <QTimer>

class QHexView : public QAbstractScrollArea {
  Q_OBJECT

public:
  explicit QHexView(QWidget *parent = nullptr);
  QHexDocument *document();

  void setDocument(QHexDocument *document);

  /*=============================*/
  // added by wingsummer

  QHexRenderer *renderer();
  void switchDocument(QHexDocument *document, QHexRenderer *renderer,
                      int vBarValue);
  bool setLockedFile(bool b);
  bool setKeepSize(bool b);
  bool isReadOnly();
  bool isKeepSize();
  bool isLocked();
  quint64 documentLines();
  quint64 documentBytes();
  quint64 currentRow();
  quint64 currentColumn();
  quint64 currentOffset();
  quint64 selectlength();

  void setAsciiVisible(bool b);
  bool asciiVisible();
  void setAddressVisible(bool b);
  bool addressVisible();
  void setHeaderVisible(bool b);
  bool headerVisible();

  quint64 addressBase();
  void setAddressBase(quint64 base);

  bool isSaved();
  static QFont getHexeditorFont();
  void getStatus();

private:
  void establishSignal(QHexDocument *doc);

signals:
  void cursorLocationChanged();
  void documentSwitched();
  void canUndoChanged(bool canUndo);
  void canRedoChanged(bool canRedo);
  void documentSaved(bool saved);
  void documentLockedFile(bool locked);
  void documentKeepSize(bool keep);
  void documentBookMarkChanged(BookMarkModEnum flag, int index, qint64 pos,
                               QString comment);
  void metafgVisibleChanged(bool b);
  void metabgVisibleChanged(bool b);
  void metaCommentVisibleChanged(bool b);
  void metaStatusChanged();

  void copyLimitRaised();

  /*=============================*/

protected:
  virtual bool event(QEvent *e);
  virtual void keyPressEvent(QKeyEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);
  virtual void mouseMoveEvent(QMouseEvent *e);
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void focusInEvent(QFocusEvent *e);
  virtual void focusOutEvent(QFocusEvent *e);
  virtual void wheelEvent(QWheelEvent *e);
  virtual void resizeEvent(QResizeEvent *e);
  virtual void paintEvent(QPaintEvent *e);

private slots:
  void renderCurrentLine();
  void moveToSelection();
  void blinkCursor();

private:
  void moveNext(bool select = false);
  void movePrevious(bool select = false);

private:
  bool processMove(QHexCursor *cur, QKeyEvent *e);
  bool processTextInput(QHexCursor *cur, QKeyEvent *e);
  bool processAction(QHexCursor *cur, QKeyEvent *e);
  void adjustScrollBars();
  void renderLine(quint64 line);
  quint64 firstVisibleLine() const;
  quint64 lastVisibleLine() const;
  quint64 visibleLines() const;
  bool isLineVisible(quint64 line) const;

  int documentSizeFactor() const;

  QPoint absolutePosition(const QPoint &pos) const;

private:
  QHexDocument *m_document;
  QHexRenderer *m_renderer;
  QTimer *m_blinktimer;
};

#endif // QHEXVIEW_H
