#ifndef TFTPSERVER_H
#define TFTPSERVER_H

#include <QObject>
#include <QSettings>
#include <QUdpSocket>
#include <QPair>
#include <QList>

class TFTPServer : public QObject
{
    Q_OBJECT

    QUdpSocket *listener;
    bool failed;

    QByteArray readBuffer;

    void ParseListenerDatagram(int size, QHostAddress &addr, quint16 port);

    QString serverRoot;

public:
    explicit TFTPServer(const QString &serverRoot, QObject *parent = 0);

    typedef QPair<const char *,const char *> OptionPair;
    typedef QList<quint16> OptionOffsetList;
    typedef QList<OptionPair> OptionList;
    
    static const char *LookupOption(const OptionList &options, const char *option);

signals:
    void VerboseEvent(const QString &msg);
    
public slots:
    void OnPacketReceived();
    void OnTransferVerboseEvent(const QString &msg);
};

#endif // TFTPSERVER_H
