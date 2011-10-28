/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv8profilerservice_p.h"
#include "qdeclarativedebugservice_p_p.h"
#include <private/qdeclarativeengine_p.h>
#include <private/qv8profiler_p.h>

#include <QtCore/QHash>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QV8ProfilerService, v8ServiceInstance)

class ByteArrayOutputStream : public v8::OutputStream
{
    QByteArray *_buffer;
public:
    ByteArrayOutputStream(QByteArray *buffer)
        : v8::OutputStream(),
          _buffer(buffer) {}
    void EndOfStream() {}
    WriteResult WriteAsciiChunk(char *data, int size)
    {
        QByteArray b(data, size);
        _buffer->append(b);
        return kContinue;
    }
};

// convert to a QByteArray that can be sent to the debug client
QByteArray QV8ProfilerData::toByteArray() const
{
    QByteArray data;
    //### using QDataStream is relatively expensive
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << messageType << filename << functionname << lineNumber << totalTime << selfTime << treeLevel;

    return data;
}

class QV8ProfilerServicePrivate : public QDeclarativeDebugServicePrivate
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
    QList<QDeclarativeEngine *> engines;
};

QV8ProfilerService::QV8ProfilerService(QObject *parent)
    : QDeclarativeDebugService(*(new QV8ProfilerServicePrivate()), QLatin1String("V8Profiler"), parent)
{
    Q_D(QV8ProfilerService);
    if (status() == Enabled) {
        // ,block mode, client attached
        while (!d->initialized)
            waitForMessage();
    }
}

QV8ProfilerService::~QV8ProfilerService()
{
}

QV8ProfilerService *QV8ProfilerService::instance()
{
    return v8ServiceInstance();
}

void QV8ProfilerService::addEngine(QDeclarativeEngine *engine)
{
    Q_D(QV8ProfilerService);
    Q_ASSERT(engine);
    Q_ASSERT(!d->engines.contains(engine));

    d->engines.append(engine);
}

void QV8ProfilerService::removeEngine(QDeclarativeEngine *engine)
{
    Q_D(QV8ProfilerService);
    Q_ASSERT(engine);
    Q_ASSERT(d->engines.contains(engine));

    d->engines.removeOne(engine);
}

void QV8ProfilerService::messageReceived(const QByteArray &message)
{
    Q_D(QV8ProfilerService);

    QDataStream ds(message);
    QByteArray command;
    QByteArray option;
    QByteArray title;
    ds >> command >> option;

    if (command == "V8PROFILER") {
        ds >>  title;
        if (option == "start") {
            d->initialized = true;
            startProfiling(QString::fromUtf8(title));
        } else if (option == "stop") {
            stopProfiling(QString::fromUtf8(title));
            // Send messages to client
            d->sendMessages();
        }
    }

    if (command == "V8SNAPSHOT") {
        if (option == "full")
            d->takeSnapshot(v8::HeapSnapshot::kFull);
        else if (option == "delete") {
            v8::HeapProfiler::DeleteAllSnapshots();
        }
    }

    QDeclarativeDebugService::messageReceived(message);
}

void QV8ProfilerService::startProfiling(const QString &title)
{
    // Start Profiling
    v8::HandleScope handle_scope;
    v8::Handle<v8::String> v8title = v8::String::New(reinterpret_cast<const uint16_t*>(title.data()), title.size());
    v8::CpuProfiler::StartProfiling(v8title);
}

void QV8ProfilerService::stopProfiling(const QString &title)
{
    Q_D(QV8ProfilerService);
    // Stop profiling
    v8::HandleScope handle_scope;
    v8::Handle<v8::String> v8title = v8::String::New(reinterpret_cast<const uint16_t*>(title.data()), title.size());
    const v8::CpuProfile *cpuProfile = v8::CpuProfiler::StopProfiling(v8title);
    if (cpuProfile) {
        // can happen at start
        const v8::CpuProfileNode *rootNode = cpuProfile->GetTopDownRoot();
        d->printProfileTree(rootNode);
    }
}

void QV8ProfilerServicePrivate::printProfileTree(const v8::CpuProfileNode *node, int level)
{
    for (int index = 0 ; index < node->GetChildrenCount() ; index++) {
        const v8::CpuProfileNode* childNode = node->GetChild(index);
        if (QV8Engine::toStringStatic(childNode->GetScriptResourceName()).length() > 0) {

            QV8ProfilerData rd = {(int)QV8ProfilerService::V8Entry, QV8Engine::toStringStatic(childNode->GetScriptResourceName()),
                QV8Engine::toStringStatic(childNode->GetFunctionName()),
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

    QByteArray jsonSnapshot;
    ByteArrayOutputStream bos(&jsonSnapshot);
    const v8::HeapSnapshot *snapshot = v8::HeapProfiler::TakeSnapshot(title, snapshotType);
    snapshot->Serialize(&bos, v8::HeapSnapshot::kJSON);

    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << (int)QV8ProfilerService::V8Snapshot << jsonSnapshot;

    q->sendMessage(data);
}

void QV8ProfilerServicePrivate::sendMessages()
{
    Q_Q(QV8ProfilerService);

    for (int i = 0; i < m_data.count(); ++i)
        q->sendMessage(m_data.at(i).toByteArray());
    m_data.clear();

    //indicate completion
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << (int)QV8ProfilerService::V8Complete;

    q->sendMessage(data);
}


QT_END_NAMESPACE
