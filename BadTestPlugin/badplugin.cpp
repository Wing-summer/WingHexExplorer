#include "badplugin.h"

BadPlugin::BadPlugin(QObject *parent) {}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(BadTestPlugin, BadPlugin)
#endif // QT_VERSION < 0x050000
