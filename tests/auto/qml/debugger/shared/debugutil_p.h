
/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef DEBUGUTIL_H
#define DEBUGUTIL_H

#include <QEventLoop>
#include <QTimer>
#include <QThread>
#include <QTest>
#include <QProcess>

#include <QtQml/qqmlengine.h>

#include <private/qqmldebugclient_p.h>
#include <private/qqmldebugservice_p.h>

class QQmlDebugTest
{
public:
    static bool waitForSignal(QObject *receiver, const char *member, int timeout = 5000);
};

class QQmlDebugTestService : public QQmlDebugService
{
    Q_OBJECT
public:
    QQmlDebugTestService(const QString &s, float version = 1, QObject *parent = 0);

signals:
    void stateHasChanged();

protected:
    virtual void messageReceived(const QByteArray &ba);
    virtual void stateChanged(State state);
};

class QQmlDebugTestClient : public QQmlDebugClient
{
    Q_OBJECT
public:
    QQmlDebugTestClient(const QString &s, QQmlDebugConnection *c);

    QByteArray waitForResponse();

signals:
    void stateHasChanged();
    void serverMessage(const QByteArray &);

protected:
    virtual void stateChanged(State state);
    virtual void messageReceived(const QByteArray &ba);

private:
    QByteArray lastMsg;
};

class QQmlDebugProcess : public QObject
{
    Q_OBJECT
public:
    QQmlDebugProcess(const QString &executable);
    ~QQmlDebugProcess();

    void setEnvironment(const QStringList &environment);

    void start(const QStringList &arguments);
    bool waitForSessionStart();

    QString output() const;
    void stop();

private slots:
    void processAppOutput();

private:
    QString m_executable;
    QProcess m_process;
    QString m_outputBuffer;
    QString m_output;
    QTimer m_timer;
    QEventLoop m_eventLoop;
    QMutex m_mutex;
    bool m_started;
    QStringList m_environment;
};

#endif // DEBUGUTIL_H
