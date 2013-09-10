/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4debugservice_p.h"
#include "qqmldebugservice_p_p.h"
#include "qqmlengine.h"
#include "qv4debugging_p.h"
#include "qv4engine_p.h"

#include <private/qv8engine_p.h>

const char *V4_DEBUGGER_KEY_CONNECT = "connect";

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QV4DebugService, v4ServiceInstance)

class QV4DebuggerAgent : public QV4::Debugging::DebuggerAgent
{
public slots:
    virtual void debuggerPaused(QV4::Debugging::Debugger *debugger);
};

class QV4DebugServicePrivate : public QQmlDebugServicePrivate
{
    Q_DECLARE_PUBLIC(QV4DebugService)

public:
    QMutex initializeMutex;
    QWaitCondition initializeCondition;
    QV4DebuggerAgent debuggerAgent;
};

QV4DebugService::QV4DebugService(QObject *parent)
    : QQmlDebugService(*(new QV4DebugServicePrivate()),
                       QStringLiteral("V4Debugger"), 1, parent)
{
    Q_D(QV4DebugService);

    // don't execute stateChanged, messageReceived in parallel
    QMutexLocker lock(&d->initializeMutex);

    if (registerService() == Enabled
            && QQmlDebugService::blockingMode()) {
        // let's wait for first message ...
        d->initializeCondition.wait(&d->initializeMutex);
    }
}

QV4DebugService::~QV4DebugService()
{
}

QV4DebugService *QV4DebugService::instance()
{
    return v4ServiceInstance();
}

void QV4DebugService::addEngine(const QQmlEngine *engine)
{
    // just make sure that the service is properly registered
    if (engine)
        v4ServiceInstance()->addEngine(QV8Engine::getV4(engine->handle()));
}

void QV4DebugService::removeEngine(const QQmlEngine *engine)
{
    // just make sure that the service is properly registered
    if (engine)
        v4ServiceInstance()->removeEngine(QV8Engine::getV4(engine->handle()));
}

void QV4DebugService::addEngine(const QV4::ExecutionEngine *engine)
{
    Q_D(QV4DebugService);
    if (engine)
        d->debuggerAgent.addDebugger(engine->debugger);
}

void QV4DebugService::removeEngine(const QV4::ExecutionEngine *engine)
{
    Q_D(QV4DebugService);
    if (engine)
        d->debuggerAgent.removeDebugger(engine->debugger);
}

void QV4DebugService::stateChanged(QQmlDebugService::State newState)
{
    Q_D(QV4DebugService);
    QMutexLocker lock(&d->initializeMutex);

    if (newState != Enabled) {
        // wake up constructor in blocking mode
        // (we might got disabled before first message arrived)
        d->initializeCondition.wakeAll();
    }
}

void QV4DebugService::messageReceived(const QByteArray &message)
{
    Q_D(QV4DebugService);
    QMutexLocker lock(&d->initializeMutex);

    QQmlDebugStream ds(message);
    QByteArray header;
    ds >> header;

    if (header == "V4DEBUG") {
        QByteArray command;
        QByteArray data;
        ds >> command >> data;

        if (command == V4_DEBUGGER_KEY_CONNECT) {
            // wake up constructor in blocking mode
            d->initializeCondition.wakeAll();
        }
    }
}

void QV4DebuggerAgent::debuggerPaused(QV4::Debugging::Debugger *debugger)
{
}

QT_END_NAMESPACE
