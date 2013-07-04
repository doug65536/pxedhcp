
#include <QtCore/QCoreApplication>
#include <QStringList>

#include <signal.h>

#include "pxeservice.h"

void OnControlCSignal(int)
{
    QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString serverRoot;

    QStringList args = QCoreApplication::arguments();
    int opt = args.indexOf("--dir");
    if (opt + 1 < args.size())
        serverRoot = args[opt+1];

    signal(SIGINT, OnControlCSignal);

    PXEService s(serverRoot, &a);
    return a.exec();
}
