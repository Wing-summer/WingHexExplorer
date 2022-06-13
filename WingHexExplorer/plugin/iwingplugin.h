#ifndef IWINGPLUGIN_H
#define IWINGPLUGIN_H

#include "hexviewshadow.h"
#include <QCryptographicHash>
#include <QDockWidget>
#include <QList>
#include <QMenu>
#include <QObject>
#include <QWidget>
#include <QtCore>

#define extractHexViewShadow(shadowWrapper)                                    \
  static_cast<HexViewShadow *>(shadowWrapper[0].value<QObject *>())

enum class WingPluginMessage {
  PluginLoading,
  PluginLoaded,
  PluginUnLoading,
  PluginUnLoaded,
  ErrorMessage,
  GetHexViewShadow,
  HexViewShadowTimeout,
  MessageResponse,
  HookMessage
};

enum class ResponseMsg { UnImplement, Success, ErrorParams, Permission };

enum HookIndex {
  None = 0,
  OpenFileBegin = 1,
  OpenFileEnd = 2,
  OpenDriverBegin = 4,
  OpenDriverEnd = 8,
  CloseFileBegin = 16,
  CloseFileEnd = 32,
  NewFileBegin = 64,
  NewFileEnd = 128
};

Q_DECLARE_METATYPE(WingPluginMessage)
Q_DECLARE_METATYPE(ResponseMsg)
Q_DECLARE_METATYPE(HookIndex)

class IWingPlugin : public QObject {
  Q_OBJECT
public:
  virtual bool init(QList<IWingPlugin *> loadedplugins) = 0;
  virtual ~IWingPlugin() {}
  virtual void unload() = 0;
  virtual QMenu *registerMenu() = 0;
  virtual QDockWidget *registerDockWidget() = 0;
  virtual Qt::DockWidgetArea registerDockWidgetDockArea() = 0;
  virtual QString pluginName() = 0;
  virtual QString pluginAuthor() = 0;
  virtual uint pluginVersion() = 0;
  virtual QString puid() = 0;
  virtual QString signature() = 0;
  virtual QString comment() = 0;
  virtual QList<QVariant> optionalInfos() = 0;
  virtual HookIndex getHookSubscribe() = 0;
  IWingPlugin *self;

signals:
  void host2MessagePipe(IWingPlugin *sender, WingPluginMessage type,
                        QList<QVariant> msg);
public slots:
  virtual void plugin2MessagePipe(WingPluginMessage type,
                                  QList<QVariant> msg) = 0;
};

#define WINGSUMMER "wingsummer"

class PluginUtils {
public:
  static QString GetPUID(IWingPlugin *plugin) {
    auto str = QString("%1%2%3%4%5")
                   .arg(WINGSUMMER)
                   .arg(plugin->pluginName())
                   .arg(plugin->pluginAuthor())
                   .arg(plugin->comment())
                   .arg(plugin->pluginVersion());
    return QCryptographicHash::hash(str.toLatin1(), QCryptographicHash::Md5)
        .toHex();
  }

  static QString GetPuid(QString pluginName, QString author, QString comment,
                         uint version) {
    auto str = QString("%1%2%3%4%5")
                   .arg(WINGSUMMER)
                   .arg(pluginName)
                   .arg(author)
                   .arg(comment)
                   .arg(version);
    return QCryptographicHash::hash(str.toLatin1(), QCryptographicHash::Md5)
        .toHex();
  }
};

#define IWINGPLUGIN_INTERFACE_IID "com.wingsummer.iwingplugin"
Q_DECLARE_INTERFACE(IWingPlugin, IWINGPLUGIN_INTERFACE_IID)

#endif // IWINGPLUGIN_H
