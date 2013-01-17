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

#include "qv8profilerservice_p.h"
#include "qqmldebugservice_p_p.h"
#include "private/qjsconverter_impl_p.h"
#include <private/qv8profiler_p.h>

#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QV8ProfilerService, v8ProfilerInstance)

class DebugServiceOutputStream : public v8::OutputStream
{
public:
    DebugServiceOutputStream()
        : v8::OutputStream() {}
    void EndOfStream() {}
    WriteResult WriteAsciiChunk(char *rawData, int size)
    {
        QByteArray data;
        QQmlDebugStream ds(&data, QIODevice::WriteOnly);
        ds << QV8ProfilerService::V8SnapshotChunk << QByteArray(rawData, size);
        messages.append(data);
        return kContinue;
    }
    QList<QByteArray> messages;
};

// convert to a QByteArray that can be sent to the debug client
QByteArray QV8ProfilerData::toByteArray() const
{
    QByteArray data;
    //### using QDataStream is relatively expensive
    QQmlDebugStream ds(&data, QIODevice::WriteOnly);
    ds << messageType << filename << functionname << lineNumber << totalTime << selfTime << treeLevel;

    return data;
}

class QV8ProfilerServicePrivate : public QQmlDebugServicePrivate
{
    Q_DECLARE_PUBLIC(QV8ProfilerService)

public:
    QV8ProfilerServicePrivate()
        :initialized(false)
    {
    }

    void takeSnapshot(v8::HeapSnapshot::Type);

    void printProfileTree(const v8::CpuProfileNode *node, int level = 0);
    void sendMessages();

    QList<QV8ProfilerData> m_data;

    bool initialized;
    QMutex initializeMutex;
    QWaitCondition initializeCondition;
    QList<QString> m_ongoing;
};

QV8ProfilerService::QV8ProfilerService(QObject *parent)
    : QQmlDebugService(*(new QV8ProfilerServicePrivate()), QStringLiteral("V8Profiler"), 1, parent)
{
    Q_D(QV8ProfilerService);

    QMutexLocker lock(&d->initializeMutex);

    if (registerService() == Enabled
            && QQmlDebugService::blockingMode()) {
        // let's wait for first message ...
        d->initializeCondition.wait(&d->initializeMutex);
    }
}

QV8ProfilerService::~QV8ProfilerService()
{
}

QV8ProfilerService *QV8ProfilerService::instance()
{
    return v8ProfilerInstance();
}

void QV8ProfilerService::initialize()
{
    // just make sure that the service is properly registered
    v8ProfilerInstance();
}

void QV8ProfilerService::stateAboutToBeChanged(QQmlDebugService::State newState)
{
    Q_D(QV8ProfilerService);

    if (state() == newState)
        return;

    if (state() == Enabled) {
        foreach (const QString &title, d->m_ongoing) {
            QMetaObject::invokeMethod(this, "stopProfiling", Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, title));
        }
        QMetaObject::invokeMethod(this, "sendProfilingData", Qt::BlockingQueuedConnection);
    } else {
        // wake up constructor in blocking mode
        // (we might got disabled before first message arrived)
        d->initializeCondition.wakeAll();
    }
}

void QV8ProfilerService::messageReceived(const QByteArray &message)
{
    Q_D(QV8ProfilerService);

    QQmlDebugStream ds(message);
    QByteArray command;
    QByteArray option;
    QByteArray title;
    ds >> command >> option;

    QMutexLocker lock(&d->initializeMutex);

    if (command == "V8PROFILER") {
        ds >>  title;
        QString titleStr = QString::fromUtf8(title);
        if (option == "start") {
            QMetaObject::invokeMethod(this, "startProfiling", Qt::QueuedConnection, Q_ARG(QString, titleStr));
        } else if (option == "stop" && d->initialized) {
            QMetaObject::invokeMethod(this, "stopProfiling", Qt::QueuedConnection, Q_ARG(QString, titleStr));
            QMetaObject::invokeMethod(this, "sendProfilingData", Qt::QueuedConnection);
        }
        d->initialized = true;
    }

    if (command == "V8SNAPSHOT") {
        if (option == "full")
            QMetaObject::invokeMethod(this, "takeSnapshot", Qt::QueuedConnection);
        else if (option == "delete") {
            QMetaObject::invokeMethod(this, "deleteSnapshots", Qt::QueuedConnection);
        }
    }

    // wake up constructor in blocking mode
    d->initializeCondition.wakeAll();

    QQmlDebugService::messageReceived(message);
}

void QV8ProfilerService::startProfiling(const QString &title)
{
    Q_D(QV8ProfilerService);
    // Start Profiling

    if (d->m_ongoing.contains(title))
        return;

    v8::HandleScope handle_scope;
    v8::Handle<v8::String> v8title = v8::String::New(reinterpret_cast<const uint16_t*>(title.data()), title.size());
    v8::CpuProfiler::StartProfiling(v8title);

    d->m_ongoing.append(title);

    // indicate profiling started
    QByteArray data;
    QQmlDebugStream ds(&data, QIODevice::WriteOnly);
    ds << (int)QV8ProfilerService::V8Started;

    sendMessage(data);
}

void QV8ProfilerService::stopProfiling(const QString &title)
{
    Q_D(QV8ProfilerService);
    // Stop profiling

    if (!d->m_ongoing.contains(title))
        return;
    d->m_ongoing.removeOne(title);

    v8::HandleScope handle_scope;
    v8::Handle<v8::String> v8title = v8::String::New(reinterpret_cast<const uint16_t*>(title.data()), title.size());
    const v8::CpuProfile *cpuProfile = v8::CpuProfiler::StopProfiling(v8title);
    if (cpuProfile) {
        // can happen at start
        const v8::CpuProfileNode *rootNode = cpuProfile->GetTopDownRoot();
        d->printProfileTree(rootNode);
    } else {
        // indicate completion, even without data
        QByteArray data;
        QQmlDebugStream ds(&data, QIODevice::WriteOnly);
        ds << (int)QV8ProfilerService::V8Complete;

        sendMessage(data);
    }
}

void QV8ProfilerService::takeSnapshot()
{
    Q_D(QV8ProfilerService);
    d->takeSnapshot(v8::HeapSnapshot::kFull);
}

void QV8ProfilerService::deleteSnapshots()
{
    v8::HeapProfiler::DeleteAllSnapshots();
}

void QV8ProfilerService::sendProfilingData()
{
    Q_D(QV8ProfilerService);
    // Send messages to client
    d->sendMessages();
}

void QV8ProfilerServicePrivate::printProfileTree(const v8::CpuProfileNode *node, int level)
{
    for (int index = 0 ; index < node->GetChildrenCount() ; index++) {
        const v8::CpuProfileNode* childNode = node->GetChild(index);
        QString scriptResourceName = QJSConverter::toString(childNode->GetScriptResourceName());
        if (scriptResourceName.length() > 0) {

            QV8ProfilerData rd = {(int)QV8ProfilerService::V8Entry, scriptResourceName,
                QJSConverter::toString(childNode->GetFunctionName()),
                childNode->GetLineNumber(), childNode->GetTotalTime(), childNode->GetSelfTime(), level};
            m_data.append(rd);

            // different nodes might have common children: fix at client side
            if (childNode->GetChildrenCount() > 0) {
                printProfileTree(childNode, level+1);
            }
        }
    }
}

void QV8ProfilerServicePrivate::takeSnapshot(v8::HeapSnapshot::Type snapshotType)
{
    Q_Q(QV8ProfilerService);

    v8::HandleScope scope;
    v8::Local<v8::String> title = v8::String::New("");

    DebugServiceOutputStream outputStream;
    const v8::HeapSnapshot *snapshot = v8::HeapProfiler::TakeSnapshot(title, snapshotType);
    snapshot->Serialize(&outputStream, v8::HeapSnapshot::kJSON);
    QList<QByteArray> messages = outputStream.messages;

    //indicate completion
    QByteArray data;
    QQmlDebugStream ds(&data, QIODevice::WriteOnly);
    ds << (int)QV8ProfilerService::V8SnapshotComplete;
    messages.append(data);

    q->sendMessages(messages);
}

void QV8ProfilerServicePrivate::sendMessages()
{
    Q_Q(QV8ProfilerService);

    QList<QByteArray> messages;
    for (int i = 0; i < m_data.count(); ++i)
        messages.append(m_data.at(i).toByteArray());
    m_data.clear();

    //indicate completion
    QByteArray data;
    QQmlDebugStream ds(&data, QIODevice::WriteOnly);
    ds << (int)QV8ProfilerService::V8Complete;
    messages.append(data);

    q->sendMessages(messages);
}


QT_END_NAMESPACE
