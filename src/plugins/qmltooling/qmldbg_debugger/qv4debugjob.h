// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4DEBUGJOB_H
#define QV4DEBUGJOB_H

#include "qv4datacollector.h"
#include <private/qv4engine_p.h>

#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>

QT_BEGIN_NAMESPACE

class QV4DataCollector;
class QV4DebugJob
{
public:
    virtual ~QV4DebugJob();
    virtual void run() = 0;
};

class JavaScriptJob : public QV4DebugJob
{
    QV4::ExecutionEngine *engine;
    int frameNr;
    int context;
    const QString &script;
    bool resultIsException;

public:
    JavaScriptJob(QV4::ExecutionEngine *engine, int frameNr, int context, const QString &script);
    void run() override;
    bool hasExeption() const;

protected:
    virtual void handleResult(QV4::ScopedValue &result) = 0;
};

class CollectJob : public QV4DebugJob
{
protected:
    QV4DataCollector *collector;
    QJsonObject result;

public:
    CollectJob(QV4DataCollector *collector) : collector(collector) {}
    const QJsonObject &returnValue() const { return result; }
};

class BacktraceJob: public CollectJob
{
    int fromFrame;
    int toFrame;
public:
    BacktraceJob(QV4DataCollector *collector, int fromFrame, int toFrame);
    void run() override;
};

class FrameJob: public CollectJob
{
    int frameNr;
    bool success;

public:
    FrameJob(QV4DataCollector *collector, int frameNr);
    void run() override;
    bool wasSuccessful() const;
};

class ScopeJob: public CollectJob
{
    int frameNr;
    int scopeNr;
    bool success;

public:
    ScopeJob(QV4DataCollector *collector, int frameNr, int scopeNr);
    void run() override;
    bool wasSuccessful() const;
};

class ValueLookupJob: public CollectJob
{
    const QJsonArray handles;
    QString exception;

public:
    ValueLookupJob(const QJsonArray &handles, QV4DataCollector *collector);
    void run() override;
    const QString &exceptionMessage() const;
};

class ExpressionEvalJob: public JavaScriptJob
{
    QV4DataCollector *collector;
    QString exception;
    QJsonObject result;

public:
    ExpressionEvalJob(QV4::ExecutionEngine *engine, int frameNr, int context,
                      const QString &expression, QV4DataCollector *collector);
    void handleResult(QV4::ScopedValue &value) override;
    const QString &exceptionMessage() const;
    const QJsonObject &returnValue() const;
};

class GatherSourcesJob: public QV4DebugJob
{
    QV4::ExecutionEngine *engine;
    QStringList sources;

public:
    GatherSourcesJob(QV4::ExecutionEngine *engine);
    void run() override;
    const QStringList &result() const;
};

class EvalJob: public JavaScriptJob
{
    bool result;

public:
    EvalJob(QV4::ExecutionEngine *engine, const QString &script);

    void handleResult(QV4::ScopedValue &result) override;
    bool resultAsBoolean() const;
};

QT_END_NAMESPACE

#endif // QV4DEBUGJOB_H

