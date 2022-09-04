#include "finddialog.h"
#include "utilities.h"
#include <DButtonBox>
#include <DDialogButtonBox>
#include <DPushButton>
#include <QShortcut>
#include <QTextCodec>

FindDialog::FindDialog(bool sel, DMainWindow *parent) : DDialog(parent) {
  this->setFixedSize(500, 600);
  this->setWindowTitle(tr("find"));

  m_string = new DRadioButton(this);
  m_string->setText(tr("findstring"));
  addContent(m_string);
  addSpacing(3);

  m_encodings = new DComboBox(this);
  m_encodings->addItems(Utilities::getEncodings());
  m_encodings->setCurrentIndex(0);
  m_encodings->setEnabled(false);
  connect(m_string, &DRadioButton::toggled, m_encodings,
          &DComboBox::setEnabled);
  addContent(m_encodings);
  addSpacing(3);

  m_lineeditor = new DLineEdit(this);
  m_lineeditor->setEnabled(false);
  connect(m_string, &DRadioButton::toggled, m_lineeditor,
          &DLineEdit::setEnabled);
  addContent(m_lineeditor);
  addSpacing(3);

  m_hex = new DRadioButton(this);
  m_hex->setText(tr("findhex"));
  addContent(m_hex);
  addSpacing(3);

  m_hexeditor = new QHexView(this);
  m_hexeditor->setAsciiVisible(false);
  m_hexeditor->setAddressVisible(false);
  m_hexeditor->setEnabled(true);
  connect(m_hex, &DRadioButton::toggled, m_hexeditor, &QHexView::setEnabled);
  addContent(m_hexeditor);
  addSpacing(10);

  m_hex->setChecked(true);

  auto group = new DButtonBox(this);

  QList<DButtonBoxButton *> blist;
  auto b = new DButtonBoxButton(tr("BeforeCursor"), this);
  connect(b, &DButtonBoxButton::toggled, [=](bool b) {
    if (b)
      _dir = SearchDirection::Foreword;
  });
  blist.push_back(b);
  b = new DButtonBoxButton(tr("AfterCursor"), this);
  connect(b, &DButtonBoxButton::toggled, [=](bool b) {
    if (b)
      _dir = SearchDirection::Backword;
  });

  blist.push_back(b);
  b = new DButtonBoxButton(tr("Selection"), this);
  if (sel) {
    connect(b, &DButtonBoxButton::toggled, [=](bool b) {
      if (b)
        _dir = SearchDirection::Selection;
    });
  } else {
    b->setEnabled(false);
  }
  blist.push_back(b);
  b = new DButtonBoxButton(tr("None"), this);
  connect(b, &DButtonBoxButton::toggled, [=](bool b) {
    if (b)
      _dir = SearchDirection::None;
  });
  blist.push_front(b);
  group->setButtonList(blist, true);
  b->setChecked(true);

  addContent(group);
  addSpacing(20);
  auto dbbox = new DDialogButtonBox(
      DDialogButtonBox::Ok | DDialogButtonBox::Cancel, this);
  connect(dbbox, &DDialogButtonBox::accepted, this, &FindDialog::on_accept);
  connect(dbbox, &DDialogButtonBox::rejected, this, &FindDialog::on_reject);
  auto key = QKeySequence(Qt::Key_Return);
  auto s = new QShortcut(key, this);
  connect(s, &QShortcut::activated, this, &FindDialog::on_accept);
  addContent(dbbox);
}

QByteArray FindDialog::getResult(SearchDirection &dir) {
  dir = _dir;
  return _findarr;
}

void FindDialog::on_accept() {
  if (m_string->isChecked()) {
    auto en = QTextCodec::codecForName(m_encodings->currentText().toUtf8());
    auto e = en->makeEncoder();
    _findarr = e->fromUnicode(m_lineeditor->text());
  } else {
    _findarr =
        m_hexeditor->document()->read(0, int(m_hexeditor->documentBytes()));
  }
  done(1);
}

void FindDialog::on_reject() {
  _findarr.clear();
  done(0);
}

void FindDialog::closeEvent(QCloseEvent *event) {
  Q_UNUSED(event);
  done(0);
}
