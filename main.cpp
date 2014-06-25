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

#include <QtWidgets>    //Core/QCoreApplication>
#include <QStringList>

#include <signal.h>

#include "pxeservice.h"
#include "mainwindow.h"

void OnControlCSignal(int)
{
    QCoreApplication::quit();
}

//class QStringToStdout : public QObject
//{
//public slots:
//    void message(QString const &msg) const
//    {
//        std::cout << msg << std::endl;
//    }
//};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString serverRoot;

    QStringList args = QCoreApplication::arguments();
    int opt = args.indexOf("--dir");
    if (opt != -1 && opt + 1 < args.size())
        serverRoot = args[opt+1];

    signal(SIGINT, OnControlCSignal);

    PXEService s(serverRoot, &a);

    MainWindow mw;
    mw.show();

    //QStringToStdout emitter;
    s.connect(&s, SIGNAL(message(QString)), &mw, SLOT(message(QString)));

    s.init();

    return a.exec();
}
