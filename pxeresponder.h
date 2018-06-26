#ifndef PXERESPONDER_H
#define PXERESPONDER_H

/*
 * This file is part of PXEDHCP.
 * Copyright 2013 A. Douglas Gale
 *
 * PXEDHCP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PXEDHCP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <QSettings>
#include <QtNetwork/QUdpSocket>
#include <QSignalMapper>
#include <QtNetwork/QNetworkInterface>

class DHCPPacket;

class PXEResponder : public QObject
{
    Q_OBJECT
    
    class Interface;

public:
    PXEResponder(const QString &bootFile, QObject *parent = 0);
    void init();

public slots:
    void on_packet();

public:
    void on_packet(QUdpSocket*);

signals:
    void verboseEvent(const QString &);
    void errorEvent(const QString &);
    void warningEvent(const QString &);

private:
    void sendDhcpOffer(DHCPPacket *dhcp, Interface &interface);
    void sendDhcpAck(DHCPPacket *dhcp, Interface& interface);
    
    struct Interface
    {
        QHostAddress addr;
        QString addrString;
        QUdpSocket *listener;

        Interface() : listener(0) {}
        ~Interface() { delete listener; }
    };

    typedef QList<Interface> InterfaceList;
    InterfaceList interfaces;
    
    QByteArray bootFileUtf8;
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
