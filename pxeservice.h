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

#ifndef PXESERVICE_H
#define PXESERVICE_H

#include <QObject>
#include "pxeresponder.h"
#include "tftpserver.h"

class PXEService : public QObject
{
    Q_OBJECT

    PXEResponder *responder;
    TFTPServer *tftpServer;

public:
    PXEService(const QString &serverRoot, const QString &bootFile,
               QObject *parent = 0);
    void init();
    
signals:
    void message(const QString &message) const;

public slots:
    void on_dhcp_message(const QString &) const;
};

#endif // PXESERVICE_H
