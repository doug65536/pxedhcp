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
    explicit PXEService(const QString &serverRoot, QObject *parent = 0);
    
signals:
    
public slots:
    void on_dhcp_message(const QString &) const;
};

#endif // PXESERVICE_H
