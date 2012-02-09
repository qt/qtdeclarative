/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMLPROFILERAPPLICATION_H
#define QMLPROFILERAPPLICATION_H

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

#include "profileclient.h"

class QmlProfilerApplication : public QCoreApplication
{
    Q_OBJECT
public:
    QmlProfilerApplication(int &argc, char **argv);
    ~QmlProfilerApplication();

    bool parseArguments();
    void printUsage();
    int exec();

public slots:
    void userCommand(const QString &command);

private slots:
    void run();
    void tryToConnect();
    void connected();
    void connectionStateChanged(QAbstractSocket::SocketState state);
    void connectionError(QAbstractSocket::SocketError error);
    void processHasOutput();
    void processFinished();

    void traceClientEnabled();
    void profilerClientEnabled();
    void traceFinished();
    void recordingChanged();

    void print(const QString &line);
    void logError(const QString &error);
    void logStatus(const QString &status);

    void declarativeComplete();
    void v8Complete();

private:
    void printCommands();
    QString traceFileName() const;

    enum ApplicationMode {
        LaunchMode,
        AttachMode
    } m_runMode;

    // LaunchMode
    QString m_programPath;
    QStringList m_programArguments;
    QProcess *m_process;
    QString m_tracePrefix;

    QString m_hostName;
    quint16 m_port;
    bool m_verbose;
    bool m_quitAfterSave;

    QDeclarativeDebugConnection m_connection;
    DeclarativeProfileClient m_declarativeProfilerClient;
    V8ProfileClient m_v8profilerClient;
    ProfileData m_profileData;
    QTimer m_connectTimer;
    uint m_connectionAttempts;

    bool m_declarativeDataReady;
    bool m_v8DataReady;
};

#endif // QMLPROFILERAPPLICATION_H
