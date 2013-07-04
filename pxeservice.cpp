#include "pxeservice.h"

#include <QSettings>
#include <iostream>

PXEService::PXEService(const QString &serverRoot, QObject *parent)
    : QObject(parent)
{
    responder = new PXEResponder(this);
    connect(responder, SIGNAL(verboseEvent(QString)), this, SLOT(on_dhcp_message(QString)));
    connect(responder, SIGNAL(warningEvent(QString)), this, SLOT(on_dhcp_message(QString)));

    tftpServer = new TFTPServer(serverRoot, this);
    connect(tftpServer, SIGNAL(VerboseEvent(QString)), this, SLOT(on_dhcp_message(QString)));
}

void PXEService::on_dhcp_message(const QString &msg) const
{
    std::cerr << msg.toAscii().data() << std::endl;
}
