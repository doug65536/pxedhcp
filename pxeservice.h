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
