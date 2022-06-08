#ifndef WINGHEXAPPLICATION_H
#define WINGHEXAPPLICATION_H

#include <DApplication>

DWIDGET_USE_NAMESPACE

class WingHexApplication : public DApplication {
public:
  WingHexApplication(int &argc, char **argv);

private:
  bool notify(QObject *obj, QEvent *event) override;
};

#endif // WINGHEXAPPLICATION_H
