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

#include "pxeresponder.h"

#include <QtEndian>
#include <QPair>
#include <QVector>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>
#include <algorithm>

bool DHCPPacketHeader::IsValid() const
{
    return magic == qToBigEndian(0x63825363U);
}

quint32 DHCPPacket::TransactionId() const
{
    return header.xid;
}

void DHCPPacket::CopyHardwareAddrTo(DHCPPacketHeader &dest) const
{
    std::copy(std::begin(header.chaddr), 
              std::end(header.chaddr), std::begin(dest.chaddr));
}

DHCPPacket::OptionMap DHCPPacket::ParseOptions(
        quint8 *options, quint32 options_len)
{
    OptionMap result;

    quint8 *options_end = (quint8 *)options + options_len;

    // We loop until p + 1 < options because we must read the length byte
    for (quint8 *p = options; *p != 255 && p + 1 < options_end; )
    {
        quint8 option_id = p[0];
        quint8 option_len = p[1];

        // Skip option id and length
        p += 2;

        // If length causes overrun, break
        if (p + option_len > options_end)
            break;

        OptionData option_payload(option_len);
        std::copy(p, p + option_len, option_payload.begin());

        if (option_id != 0)
            result.insert(option_id, option_payload);

        p += option_len;
    }

    return result;
}

QString DHCPPacket::LookupOptionName(quint8 id) const
{
    switch (id)
    {
    case 97: return "Client machine UUID";
    case 61: return "Client machine UUID";
    case 94: return "Network interface ID";
    case 93: return "Client system architecture";
    case 55: return "Parameter request list";
    case 60: return "Class ID";
    case 43: return "Vendor options";
    case 53: return "Message type";
    case 54: return "Server ID";
    case 57: return "Max message length";
    case 255: return "End of options";
    default: return QString("Unknown (%1)").arg(id);
    }
}

bool DHCPPacket::Read(QUdpSocket *socket)
{
    qint64 packet_len;
    packet_len = socket->pendingDatagramSize();
    if (packet_len < 0)
        return false;

    // Packet must be at least the size of a DHCP header
    if (packet_len < (int)sizeof(header))
        return false;

    buffer.resize(packet_len);
    if (socket->readDatagram((char *)buffer.data(), packet_len,
            &sourceAddress, &sourcePort) != packet_len)
        return false;

    memcpy(&header, buffer.data(), sizeof(header));
    if (!header.IsValid())
        return false;

    options = ParseOptions(buffer.data() + sizeof(DHCPPacketHeader),
            packet_len - sizeof(DHCPPacketHeader));

    return true;
}

QHostAddress DHCPPacket::GetSourceAddress() const
{
    return sourceAddress;
}

quint16 DHCPPacket::GetSourcePort() const
{
    return sourcePort;
}

bool DHCPPacket::IsPxeRequest() const
{
    OptionMap::const_iterator i = options.find(60);
    if (i == options.end())
        return false;

    if (i.value().size() >= 9 && !memcmp(i.value().data(), "PXEClient", 9))
        return true;

    return false;
}

quint8 DHCPPacket::GetMessageType() const
{
    if (!OptionExists(53))
        return false;

    const OptionData &data = OptionContent(53);

    if (data.size() < 1)
        return false;

    return data[0];
}

bool DHCPPacket::IsMessageType(quint8 type) const
{
    return GetMessageType() == type;
}

bool DHCPPacket::IsDhcpDiscover() const
{
    return IsMessageType(1);
}

bool DHCPPacket::IsDhcpRequest() const
{
    return IsMessageType(3);
}

bool DHCPPacket::OptionExists(quint8 option_id) const
{
    return options.find(option_id) != options.end();
}

const DHCPPacket::OptionData &DHCPPacket::OptionContent(quint8 option_id) const
{
    OptionMap::const_iterator i = options.find(option_id);
    return i.value();
}

QList<QString> DHCPPacket::PacketDetailDump() const
{
    QList<QString> r;

    r.append(header.op ? "BOOTREQUEST" : "BOOTREPLY");
    r.append(header.htype == 1? "Using MAC addresses" : "Unknown address type");
    r.append(QString("Address length: %1").arg(header.hlen));
    r.append(QString("Hops: %1").arg(header.hops));
    r.append(QString("Transaction id: 0x%1")
             .arg((uint)header.xid, 8, 16, QChar('0')));
    r.append(QString("Seconds since transaction start: %1")
             .arg(qFromBigEndian((header.secs))));
    r.append(QString("Flags: %1").arg(header.flags, 4, 16, QChar('0')));
    r.append(QString("Client IP address: 0x%1").arg(header.ciaddr, 8, 16, QChar('0')));
    r.append(QString("Your IP address: 0x%1").arg(header.yiaddr, 8, 16, QChar('0')));
    r.append(QString("Next server: 0x%1").arg(header.siaddr, 8, 16, QChar('0')));
    r.append(QString("Relay agent IP: 0x%1").arg(header.giaddr, 8, 16, QChar('0')));

    {
        QString hardwareAddr;
        hardwareAddr.append("Client addr: ");
        for (int i = 0; i < header.hlen; ++i)
        {
            hardwareAddr.append(QString("%1")
                .arg(header.chaddr[i], 2, 16, QChar('0')));
            if (i != header.hlen-1)
                hardwareAddr.append(':');
        }
        r.append(hardwareAddr);
    }

    r.append(QString("Server host name: %1").arg((char*)header.sname));
    r.append(QString("Boot file: %1").arg((char*)header.file));

    for (OptionMap::ConstIterator i = options.begin(); i != options.end(); ++i)
    {
        const OptionData &data = i.value();

        r.append(QString("Option %1, length %2, %3")
                 .arg((uint)i.key())
                 .arg(data.size())
                 .arg(LookupOptionName(i.key())));

        QString option_content(" ");
        for (int i = 0; i < data.size(); ++i)
        {
            option_content.append(QString("%1 ").arg((uint)data[i], 2, 16, QChar('0')));

            if (i && ((i & 15) == 15))
            {
                r.append(option_content);
                option_content.clear();
                option_content.append(' ');
            }
        }
        if (option_content.length())
            r.append(option_content);
    }

    return r;
}

PXEResponder::PXEResponder(QObject *parent)
    : QObject(parent)
{
}

void PXEResponder::init()
{
    // Get network interface list
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

    emit verboseEvent(QString("Total interfaces: %1").arg(addresses.size()));

    for (int i = 0; i < addresses.size(); ++i)
    {
        // Only support IPv4
        if (addresses[i].protocol() != QUdpSocket::IPv4Protocol)
            continue;

        // Don't other looking at the loopback interface
        if (addresses[i] == QHostAddress(QHostAddress::LocalHost))
            continue;

        interfaces.append(Interface());
        Interface& interface = interfaces.back();

        interface.addr = addresses[i];
        interface.addrString = addresses[i].toString();

        emit verboseEvent(QString("Added interface %1").arg(interface.addrString));

        // Create a UDP socket and bind it to port 67
        interface.listener = new QUdpSocket(this);

        // We don't bind to an address because we want to receive broadcasts
        if (!interface.listener->bind(67))
//        if (!interface.listener->bind(4011))
        {
            emit errorEvent("Failed to bind DHCP listener, giving up");
        }
        else
        {
            // Connect the readyRead signal to our slot
            connect(interface.listener, SIGNAL(readyRead()), this, SLOT(on_packet()));
            emit verboseEvent("Listening for DHCP requests");
        }

        return;
    }

    emit errorEvent("Could not find suitable network interface");
}

void PXEResponder::on_packet()
{
    DHCPPacket *dhcp = new DHCPPacket(this);

    for (InterfaceList::iterator i = interfaces.begin(), 
         e = interfaces.end(); i != e; ++i)
    {
        Interface& interface = *i;
        while (interface.listener->hasPendingDatagrams())
        {
            if (!dhcp->Read(interface.listener))
            {
                emit errorEvent("Error decoding DHCP packet");
                continue;
            }

            // Ignore requests that are not from a PXE client
            if (!dhcp->IsPxeRequest())
            {
                emit verboseEvent("Ignoring non PXE packet");
                continue;
            }

            emit verboseEvent(QString("From %1")
                              .arg(dhcp->GetSourceAddress().toString()));

            QList<QString> detail = dhcp->PacketDetailDump();
            for (QList<QString>::Iterator i = detail.begin(); 
                 i != detail.end(); ++i) {
                emit verboseEvent(*i);
            }

            // FIXME: hard coded
            //const char bootfile[] = "pxelinux.0";
            //const char bootfile[] = "Boot/wdsnbp.com";
            const char bootfile[] = "pxeboot.com";

            
            //emit verboseEvent(QString("Message type is %1")
            //                  .arg(dhcp->GetMessageType()));

            if (dhcp->IsDhcpDiscover())
            {
                emit verboseEvent("Got discover");

                // Send DHCPOFFER response

                DHCPPacketHeader offer;
                memset(&offer, 0, sizeof(offer));
                offer.op = 2;
                offer.htype = 1;
                offer.hlen = 6;
                offer.hops = 0;
                offer.xid = dhcp->TransactionId();

                // Proxy DHCP must set IP to 0.0.0.0
                offer.ciaddr = 0;

                // Next-server address
                offer.siaddr = qToBigEndian(interface.addr.toIPv4Address());

                // Copy client hardware address
                dhcp->CopyHardwareAddrTo(offer);

                // Build options
                QByteArray offer_options;

                offer_options.append((char*)&offer, sizeof(offer));

                // Need:
                // boot server list
                // discovery control options (if provided)

                offer_options.append(53);       // 53=DHCP Message Type
                offer_options.append(1);
                offer_options.append(2);        // 2=DHCPOFFER

                // Option #54, server identifier
                quint32 selfaddr = interface.addr.toIPv4Address();
                offer_options.append(54);
                offer_options.append(4);
                offer_options.append((selfaddr >> 24) & 0xFF);
                offer_options.append((selfaddr >> 16) & 0xFF);
                offer_options.append((selfaddr >>  8) & 0xFF);
                offer_options.append((selfaddr      ) & 0xFF);

                // Option #97, Client machine identifier
                //const DHCPPacket::OptionData &client_id = dhcp->OptionContent(97);
                //offer_options.append(97);
                //offer_options.append(client_id.size());
                //offer_options.append((const char *)client_id.data(), client_id.size());

                // Option #61, Client machine identifier (too)
                //offer_options.append(61);
                //offer_options.append(client_id.size());
                //offer_options.append((const char *)client_id.data(), client_id.size());

                // Option #60
                offer_options.append(60);
                offer_options.append(9);
                offer_options.append("PXEClient", 9);

                // Option #67, boot file
                offer_options.append(67);
                offer_options.append(strlen(bootfile));
                offer_options.append(bootfile, strlen(bootfile));

                // Vendor options option
                QByteArray vendor_options;

                // Option #6, PXE_DISCOVERY_CONTROL
                vendor_options.append(6);
                vendor_options.append(1);
                // bit 3: just use boot filename, no prompt/menu/discover
                // bit 2: only use/accept servers in PXE_BOOT_SERVERS
                // bit 1: disable multicast discovery
                vendor_options.append((1 << 3) | /*(1 << 2) |*/ (1 << 1));
                //vendor_options.append(7);  // use boot server list, broadcast discovery

                // Vendor option #8, server list
                vendor_options.append(8);
                vendor_options.append(7);
                vendor_options.append((char)0x80);  // type (hi)
                vendor_options.append((char)0);  // type (lo)
                vendor_options.append(1);           // 1 address
                vendor_options.append((selfaddr >> 24) & 0xFF);
                vendor_options.append((selfaddr >> 16) & 0xFF);
                vendor_options.append((selfaddr >>  8) & 0xFF);
                vendor_options.append((selfaddr      ) & 0xFF);

                vendor_options.append((char)255);

                offer_options.append(43);
                offer_options.append(vendor_options.size());
                offer_options.append(vendor_options);

                offer_options.append((char)255);

                //offer_options.append();

                // Source address is probably 0.0.0.0, because the client hasn't had an address assigned yet!
                auto targetAddress = dhcp->GetSourceAddress();
                if (targetAddress.toIPv4Address() == 0)
                    targetAddress = QHostAddress::Broadcast;

                auto bytesSent = interface.listener->writeDatagram(
                            offer_options, targetAddress, 68);
                if (bytesSent < 0)
                    emit verboseEvent(
                            QString("Sent DHCPOFFER (%1 bytes, error=%2)")
                            .arg(bytesSent)
                            .arg(interface.listener->errorString()));
                else
                    emit verboseEvent(
                            QString("Sent DHCPOFFER (%1 bytes)")
                            .arg(bytesSent));
            }
            else if (dhcp->IsDhcpRequest())
            {
                // Send DHCPACK
                emit verboseEvent("Got request!");

                DHCPPacketHeader offer;
                memset(&offer, 0, sizeof(offer));
                offer.op = 2;
                offer.htype = 1;
                offer.hlen = 6;
                offer.hops = 0;
                offer.xid = dhcp->TransactionId();

                // Proxy DHCP must set IP to 0.0.0.0
                offer.ciaddr = 0;

                // Next-server address
                offer.siaddr = qToBigEndian(interface.addr.toIPv4Address());

                // Copy client hardware address
                dhcp->CopyHardwareAddrTo(offer);

                QByteArray addressString = interface.addrString.toLocal8Bit();
                std::fill(std::begin(offer.sname), std::end(offer.sname), 0);
                std::copy_n(str.constBegin(),
                            std::min(sizeof(offer.sname)-1,
                                     size_t(str.size())),
                            offer.sname);

                strcpy((char*)offer.file, bootfile);

                // Build options
                QByteArray offer_options;

                offer_options.append((char*)&offer, sizeof(offer));

                // Option #53, message type
                offer_options.append(53);       // 53=DHCP Message Type
                offer_options.append(1);
                offer_options.append(5);        // 5=DHCPACK

                // Option #60
                offer_options.append(60);
                offer_options.append(9);
                offer_options.append("PXEClient", 9);

                // Need:
                // boot server list
                // discovery control options (if provided)

                // Option #54, server identifier
                quint32 selfaddr = interface.addr.toIPv4Address();
                offer_options.append(54);
                offer_options.append(4);
                offer_options.append((char)((selfaddr >> 24) & 0xFF));
                offer_options.append((char)((selfaddr >> 16) & 0xFF));
                offer_options.append((char)((selfaddr >>  8) & 0xFF));
                offer_options.append((char)((selfaddr      ) & 0xFF));

                // Option #66, server name
                offer_options.append(66);
                //offer_options.append(4);
                //offer_options.append((selfaddr >> 24) & 0xFF);
                //offer_options.append((selfaddr >> 16) & 0xFF);
                //offer_options.append((selfaddr >>  8) & 0xFF);
                //offer_options.append((selfaddr      ) & 0xFF);
                offer_options.append(interface.addrString.length());
                offer_options.append(interface.addrString.toLocal8Bit());

                // Option #97, Client machine identifier
                //const DHCPPacket::OptionData &client_id = dhcp->OptionContent(97);
                //offer_options.append(97);
                //offer_options.append(client_id.size());
                //offer_options.append((const char *)client_id.data(),
                //                     client_id.size());

                // Option #61, Client machine identifier (too)
                //offer_options.append(61);
                //offer_options.append(client_id.size());
                //offer_options.append((const char *)client_id.data(),
                //                     client_id.size());

                // Option #67, boot file
                offer_options.append(67);
                offer_options.append(strlen(bootfile));
                offer_options.append(bootfile, strlen(bootfile));

                // Vendor options option
                QByteArray vendor_options;

                vendor_options.append(6);
                vendor_options.append(1);
                // bit 3: just use boot filename, no prompt/menu/discover
                // bit 2: only use/accept servers in PXE_BOOT_SERVERS
                // bit 1: disable multicast discovery
                vendor_options.append((1 << 3) | (0 << 2) | (1 << 1));
                //vendor_options.append(7);  // use boot server list, broadcast discovery

                vendor_options.append((char)255);

                // Option #43, vendor options
                offer_options.append(43);
                offer_options.append(vendor_options.size());
                offer_options.append(vendor_options);

                offer_options.append((char)255);

                auto bytesSent = interface.listener->writeDatagram(
                            offer_options, dhcp->GetSourceAddress(), 68);

                if (bytesSent < 0)
                    emit verboseEvent(QString("Sent DHCPACK"
                                              " (%1 bytes, error=%2)")
                                      .arg(bytesSent)
                                      .arg(interface.listener->errorString()));
                else
                    emit verboseEvent(QString("Sent DHCPACK (%1 bytes)")
                                      .arg(bytesSent));
            }
            else
            {
                emit verboseEvent("Got something else?");
            }
        }
    }
}
