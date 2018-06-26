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

#include "tftptransfer.h"

#include <QFile>
#include <QFileInfo>
#include <QtEndian>

// DATA and ACK packets start with this structure
// Error packets do too, except "block" holds the error code
struct BlockHeader
{
    quint16 opcode;
    quint16 block;
};

TFTPTransfer::TFTPTransfer(QObject *parent)
    : QObject(parent)
    , sock(nullptr)
    , blockSize(512)
{
}

void TFTPTransfer::SendErrorPacket(QUdpSocket *target,
    const QHostAddress &address, quint16 port,
    quint16 errorCode, const QString &errorMessage)
{
    QByteArray packet;
    packet.resize(sizeof(BlockHeader) + errorMessage.length() + 1);
    BlockHeader *header;
    header = (BlockHeader*)packet.data();
    header->opcode = qToBigEndian((quint16)ERROR);
    header->block = qToBigEndian((quint16)errorCode);
    memcpy((char*)(header+1), errorMessage.toLocal8Bit().constData(), 
           errorMessage.length() + 1);

    quint16 sentSize = target->writeDatagram(packet, address, port);

    if (sentSize != packet.size())
        emit ErrorEvent("Outbound error packet truncated!");
}

void TFTPTransfer::SendErrorFileNotFound(QUdpSocket *target,
    const QHostAddress &addr, quint16 port)
{
    SendErrorPacket(target, addr, port, FILENOTFOUND, "File not found");
}

QString TFTPTransfer::TranslateFilename(
        const QString &serverRoot, const char *filename)
{
    QString result(filename);
    result = result.replace(QChar('\\'), QChar('/'));

    if (result.contains("/..") || result.contains("../"))
        return "";

    if (result[0] != '/')
        result = serverRoot + '/' + result;
    else
        result = serverRoot + result;

    return result;
}

bool TFTPTransfer::StartTransfer(QUdpSocket *,
    const QHostAddress &addr, quint16 port,
    quint16 opcode, const QString &serverRoot,
    const TFTPServer::OptionList &options)
{
    sock = new QUdpSocket(this);

    connect(sock, SIGNAL(readyRead()), this, SLOT(OnPacketReceived()));

    if (!sock->bind(0))
    {
        emit ErrorEvent("bind(0) failed!");
        deleteLater();
        return false;
    }

    if (opcode != RRQ)
    {
        emit ErrorEvent("opcode is not RRQ!");
        SendErrorPacket(sock, addr, port,
                        ILLEGALOPERATION, "Unsupported operation");
        deleteLater();
        return false;
    }

    const char *requestFilename;
    requestFilename = options[0].second;

    // mode is ignored
    //const char *mode;
    //mode = options[1].second;

    QString filename;
    filename = TranslateFilename(serverRoot, requestFilename);

    file = new QFile(filename, this);
    if (!file)
    {
        emit ErrorEvent("Out of memory");
        deleteLater();
        return false;
    }

    if (!file->open(QFile::ReadOnly))
    {
        emit ErrorEvent(QString("File %1 not found: %2")
            .arg(filename).arg(file->errorString()));
        SendErrorFileNotFound(sock, addr, port);
        deleteLater();
        return false;
    }

    emit verboseEvent(QString("TFTP: File \"%1\" opened").arg(filename));

    // File must be world readable
    if ((file->permissions() & QFile::ReadOther) == 0)
    {
        SendErrorPacket(sock, addr, port, ACCESSVIOLATION, "Permission denied");
        deleteLater();
        return false;
    }

    // Prepare OACK
    QByteArray oack;
    oack.append((char)0);
    oack.append(OACK);

    const char *tsizeOption = TFTPServer::LookupOption(options, "tsize");

    if (tsizeOption)
    {
        qint64 fileSize;
        fileSize = file->size();

        emit verboseEvent(QString("Response file size=%1").arg(fileSize));

        oack.append("tsize");
        oack.append((char)0);
        oack.append(QString("%1").arg(fileSize));
        oack.append((char)0);
    }

    const char *blockSizeOption = TFTPServer::LookupOption(options, "blksize");

    if (blockSizeOption)
    {
        blockSize = atoi(blockSizeOption);

        emit verboseEvent(QString("Setting blksize to %1").arg(blockSize));

        oack.append("blksize");
        oack.append((char)0);
        oack.append(QString("%1").arg(blockSize));
        oack.append((char)0);
    }

    qint64 sentSize;

    // Send OACK if we set any options
    if (oack.size() > 2)
    {
        sentSize = sock->writeDatagram(oack, addr, port);

        if (sentSize != oack.size())
            emit ErrorEvent("Outbound OACK packet truncated!");
    }

    clientAddr = addr;
    clientPort = port;

    // Size the packet buffer
    sendBuffer.resize(sizeof(BlockHeader) + blockSize);

    // Start at block 1
    block = 1;

    // Prepare packet
    BlockHeader *header;
    header = (BlockHeader*)sendBuffer.data();

    header->opcode = qToBigEndian((quint16)DATA);
    header->block = qToBigEndian(block);

    // Read data directly into packet buffer
    qint16 readSize;
    readSize = file->read((char*)(header + 1), blockSize);

    // Remember packet size for possible retransmit
    sendSize = (quint16)(sizeof(BlockHeader) + readSize);

    // Don't send initial packet if there was an OACK sent
    // (The client will send an ACK for block 0 to acknowledge OACK)
    if (oack.size() > 2)
    {
        // Send initial DATA datagram
        sentSize = sock->writeDatagram((char*)header, sendSize,
            clientAddr, clientPort);

        if (sentSize != sendSize)
            emit ErrorEvent("Outbound initial data packet truncated!");
    }

    return true;
}

void TFTPTransfer::OnPacketReceived()
{
    QHostAddress sourceAddr;
    quint16 sourcePort;

    while (sock->hasPendingDatagrams())
    {
        int size = sock->pendingDatagramSize();
        if (recvBuffer.size() < size)
            recvBuffer.resize(size);

        size = sock->readDatagram(recvBuffer.data(), size,
            &sourceAddr, &sourcePort);

        emit verboseEvent(QString("Datagram received, size=%1").arg(size));

        // Drop packets from wrong client
        if (sourceAddr != clientAddr || sourcePort != clientPort)
        {
            emit ErrorEvent("Dropped packet from wrong source");
            continue;
        }

        BlockHeader header, *headerPtr;
        headerPtr = (BlockHeader*)recvBuffer.data();

        header.opcode = qFromBigEndian(headerPtr->opcode);
        header.block = qFromBigEndian(headerPtr->block);

        // Drop packets that are not acknowledgements
        if (header.opcode != ACK)
        {
            emit ErrorEvent(QString("Receive error ACK,"
                                    " opcode=%1, error=%2, message=%3")
                            .arg(header.opcode)
                            .arg(header.block)
                            .arg((char*)(headerPtr+1)));
            deleteLater();
            return;
        }

        // Remove spam message
        //emit verboseEvent(QString("Received ACK for %1").arg(header.block));

        qint64 sentSize;

        // Acknowledgement for the previous block is a request
        // to retransmit
        if (header.block == (quint16)(block-1))
        {
            // Retransmit current packet
            sentSize = sock->writeDatagram(sendBuffer.data(), sendSize,
                clientAddr, clientPort);

            emit verboseEvent(QString("Retransmitted packet %1").arg(block));

            if (sentSize != sendSize)
                emit ErrorEvent("Outbound retransmitted packet truncated!");

            continue;
        }

        // Drop acknowledgements that are for wrong block
        if (header.block != block)
        {
            emit verboseEvent("Dropped acknowledgement"
                              " for unexpected block number");
            continue;
        }

        if (sendSize < sizeof(BlockHeader) + blockSize)
        {
            emit verboseEvent("Transfer completed");
            // Last send was partial packet, we're done
            sock->close();
            deleteLater();
            return;
        }

        ++block;

        // Removed spam event
        //emit verboseEvent(QString("Sending block %1").arg(block));

        // Prepare a new send packet
        headerPtr = (BlockHeader*)sendBuffer.data();
        headerPtr->opcode = qToBigEndian((quint16)DATA);
        headerPtr->block = qToBigEndian(block);

        qint64 readSize;
        readSize = file->read((char*)(headerPtr+1), blockSize);

        sendSize = sizeof(BlockHeader) + readSize;

        sentSize = sock->writeDatagram((char*)sendBuffer.data(), sendSize,
            clientAddr, clientPort);

        if (sentSize != sendSize)
            emit ErrorEvent("Outbound DATA packet truncated!");
    }
}

