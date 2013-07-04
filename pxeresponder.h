#ifndef PXERESPONDER_H
#define PXERESPONDER_H

#include <QSettings>
#include <QtNetwork/QUdpSocket>
#include <QSignalMapper>
#include <QtNetwork/QNetworkInterface>

class PXEResponder : public QObject
{
    Q_OBJECT

    QHostAddress firstNic;
    QString firstNicIpString;
    QUdpSocket *listener;

public:

    PXEResponder(QObject *parent = 0);

public slots:
    void on_packet();

public:
    void on_packet(QUdpSocket*);

signals:
    void verboseEvent(const QString &);
    void errorEvent(const QString &);
    void warningEvent(const QString &);
};

struct DHCPPacketHeader
{
    quint8 op;
    quint8 htype;
    quint8 hlen;
    quint8 hops;

    quint32 xid;

    quint16 secs;
    quint16 flags;

    quint32 ciaddr;
    quint32 yiaddr;
    quint32 siaddr;
    quint32 giaddr;
    quint8 chaddr[16];

    qint8 sname[64];
    qint8 file[128];
    //quint8 dummy[192];

    quint32 magic;

    bool IsValid() const;
};

class DHCPPacket : public QObject
{
    Q_OBJECT

    QVector<quint8> buffer;

    DHCPPacketHeader header;

public:
    typedef QVector<quint8> OptionData;

private:
    typedef QMap<quint8,OptionData> OptionMap;

    OptionMap ParseOptions(quint8 *options, quint32 options_len);
    QString LookupOptionName(quint8 id) const;

    QHostAddress sourceAddress;
    quint16 sourcePort;

    OptionMap options;

    quint8 GetMessageType() const;
    bool IsMessageType(quint8 type) const;

public:
    DHCPPacket(QObject *parent) : QObject(parent) {}
    ~DHCPPacket() {}

    quint32 TransactionId() const;
    void CopyHardwareAddrTo(DHCPPacketHeader &dest) const;

    bool Read(QUdpSocket *socket);

    QHostAddress GetSourceAddress() const;
    quint16 GetSourcePort() const;

    bool IsPxeRequest() const;

    bool IsDhcpDiscover() const;
    bool IsDhcpRequest() const;

    bool OptionExists(quint8 option_id) const;
    const OptionData &OptionContent(quint8 option_id) const;

    QList<QString> PacketDetailDump() const;
};

#endif // PXERESPONDER_H
