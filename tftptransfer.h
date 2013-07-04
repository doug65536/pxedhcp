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

    bool StartTransfer(QUdpSocket *listener, const QHostAddress &addr, quint16 port,
        quint16 opcode, const QString &serverRoot,
        const TFTPServer::OptionList &options);
    
    static QString TranslateFilename(const QString &serverRoot, const char *filename);

signals:
    void VerboseEvent(const QString &msg);
    
public slots:
    void OnPacketReceived();
};

#endif // TFTPTRANSFER_H
