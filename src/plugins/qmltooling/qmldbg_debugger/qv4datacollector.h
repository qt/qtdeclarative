/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV4DATACOLLECTOR_H
#define QV4DATACOLLECTOR_H

#include <private/qv4engine_p.h>
#include <private/qv4debugging_p.h>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

QT_BEGIN_NAMESPACE

class QV4DataCollector
{
public:
    typedef uint Ref;
    typedef QVector<uint> Refs;
    static const Ref s_invalidRef;

    QV4::CallContext *findContext(int frame);
    static QV4::Heap::CallContext *findScope(QV4::ExecutionContext *ctxt, int scope);
    QVector<QV4::Heap::ExecutionContext::ContextType> getScopeTypes(int frame);
    static int encodeScopeType(QV4::Heap::ExecutionContext::ContextType scopeType);

    QV4DataCollector(QV4::ExecutionEngine *engine);
    ~QV4DataCollector();

    Ref collect(const QV4::ScopedValue &value);
    bool isValidRef(Ref ref) const;
    QJsonObject lookupRef(Ref ref);

    Ref addFunctionRef(const QString &functionName);
    Ref addScriptRef(const QString &scriptName);

    bool collectScope(QJsonObject *dict, int frameNr, int scopeNr);
    QJsonObject buildFrame(const QV4::StackFrame &stackFrame, int frameNr);

    QV4::ExecutionEngine *engine() const { return m_engine; }
    QJsonArray flushCollectedRefs();

private:
    Ref addRef(QV4::Value value, bool deduplicate = true);
    QV4::ReturnedValue getValue(Ref ref);
    bool lookupSpecialRef(Ref ref, QJsonObject *dict);

    QJsonArray collectProperties(const QV4::Object *object);
    QJsonObject collectAsJson(const QString &name, const QV4::ScopedValue &value);
    void collectArgumentsInContext();

    QV4::ExecutionEngine *m_engine;
    Refs collectedRefs;
    QV4::PersistentValue values;
    typedef QHash<Ref, QJsonObject> SpecialRefs;
    SpecialRefs specialRefs;
};

class CollectJob : public QV4::Debugging::V4Debugger::Job
{
protected:
    QV4DataCollector *collector;
    QJsonObject result;
    QJsonArray collectedRefs;
public:
    CollectJob(QV4DataCollector *collector) : collector(collector) {}
    const QJsonObject &returnValue() const { return result; }
    const QJsonArray &refs() const { return collectedRefs; }
};

class BacktraceJob: public CollectJob
{
    int fromFrame;
    int toFrame;
public:
    BacktraceJob(QV4DataCollector *collector, int fromFrame, int toFrame);
    void run();
};

class FrameJob: public CollectJob
{
    int frameNr;
    bool success;

public:
    FrameJob(QV4DataCollector *collector, int frameNr);
    void run();
    bool wasSuccessful() const;
};

class ScopeJob: public CollectJob
{
    int frameNr;
    int scopeNr;
    bool success;

public:
    ScopeJob(QV4DataCollector *collector, int frameNr, int scopeNr);
    void run();
    bool wasSuccessful() const;
};

class ValueLookupJob: public CollectJob
{
    const QJsonArray handles;
    QString exception;

public:
    ValueLookupJob(const QJsonArray &handles, QV4DataCollector *collector);
    void run();
    const QString &exceptionMessage() const;
};

class ExpressionEvalJob: public QV4::Debugging::V4Debugger::JavaScriptJob
{
    QV4DataCollector *collector;
    QString exception;
    QJsonObject result;
    QJsonArray collectedRefs;

public:
    ExpressionEvalJob(QV4::ExecutionEngine *engine, int frameNr, const QString &expression,
                      QV4DataCollector *collector);
    virtual void handleResult(QV4::ScopedValue &value);
    const QString &exceptionMessage() const;
    const QJsonObject &returnValue() const;
    const QJsonArray &refs() const;
};

class GatherSourcesJob: public QV4::Debugging::V4Debugger::Job
{
    QV4::ExecutionEngine *engine;
    const int seq;

public:
    GatherSourcesJob(QV4::ExecutionEngine *engine, int seq);
    void run();
};

QT_END_NAMESPACE

#endif // QV4DATACOLLECTOR_H
