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

#include "pxeservice.h"

#include <QSettings>
#include <iostream>

PXEService::PXEService(const QString &serverRoot, QObject *parent)
    : QObject(parent)
{
    responder = new PXEResponder(this);
    connect(responder, SIGNAL(verboseEvent(QString)), this, SLOT(on_dhcp_message(QString)));
    connect(responder, SIGNAL(warningEvent(QString)), this, SLOT(on_dhcp_message(QString)));
    connect(responder, SIGNAL(errorEvent(QString)), this, SLOT(on_dhcp_message(QString)));

    tftpServer = new TFTPServer(serverRoot, this);
    connect(tftpServer, SIGNAL(verboseEvent(QString)), this, SLOT(on_dhcp_message(QString)));
    connect(tftpServer, SIGNAL(warningEvent(QString)), this, SLOT(on_dhcp_message(QString)));
    connect(tftpServer, SIGNAL(errorEvent(QString)), this, SLOT(on_dhcp_message(QString)));
}

void PXEService::init()
{
    responder->init();
    tftpServer->init();
}

void PXEService::on_dhcp_message(const QString &msg) const
{
    emit message(msg);
//    std::cerr << msg.toLocal8Bit().data() << std::endl;
}
