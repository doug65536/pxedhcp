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
