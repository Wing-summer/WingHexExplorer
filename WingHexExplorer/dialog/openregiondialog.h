#ifndef OPENREGIONDIALOG_H
#define OPENREGIONDIALOG_H

#include <DDialog>
#include <DFileChooserEdit>
#include <DMainWindow>
#include <DSpinBox>

DWIDGET_USE_NAMESPACE

struct RegionFileResult {
  QString filename;
  qint64 start;
  qint64 length;
};

class OpenRegionDialog : public DDialog {
  Q_OBJECT
public:
  OpenRegionDialog(DMainWindow *parent = nullptr);
  RegionFileResult getResult();

private:
  void on_accept();
  void on_reject();

private:
  DFileChooserEdit *filepath;
  DSpinBox *sbStart, *sbLength;

  RegionFileResult res;

protected:
  void closeEvent(QCloseEvent *event) override;
};

#endif // OPENREGIONDIALOG_H
