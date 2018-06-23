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

#ifndef TFTPTRANSFER_H
#define TFTPTRANSFER_H

#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>
#include <QFile>

#include "tftpserver.h"

class TFTPTransfer : public QObject
{
    Q_OBJECT

    enum Opcodes
    {
        RRQ = 1,
        WRQ = 2,
        DATA = 3,
        ACK = 4,
        ERROR = 5,
        OACK = 6
    };

    enum ErrorCode
    {
        NOTDEFINED,
        FILENOTFOUND,
        ACCESSVIOLATION,
        DISKFULL,
        ILLEGALOPERATION,
        UNKNOWNTRANSFERID,
        FILEEXISTS,
        NOSUCHUSER
    };

    QByteArray recvBuffer;

    QFile *file;
    QUdpSocket *sock;
    QByteArray sendBuffer;
    quint16 sendSize;
    quint16 block;
    quint16 blockSize;

    QHostAddress clientAddr;
    quint16 clientPort;

public:
    explicit TFTPTransfer(QObject *parent = 0);

    void SendErrorPacket(QUdpSocket *target,
        const QHostAddress &address, quint16 port,
        quint16 errorCode, const QString &errorMessage);

    void SendErrorFileNotFound(QUdpSocket *target,
        const QHostAddress &addr, quint16 port);

    bool StartTransfer(
            QUdpSocket *listener, const QHostAddress &addr, quint16 port,
            quint16 opcode, const QString &serverRoot,
            const TFTPServer::OptionList &options);
    
    static QString TranslateFilename(
            const QString &serverRoot, const char *filename);

signals:
    void VerboseEvent(const QString &msg);
    void ErrorEvent(const QString &msg);
    
public slots:
    void OnPacketReceived();
};

#endif // TFTPTRANSFER_H
