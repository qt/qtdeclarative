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
    const QString &script;
    bool resultIsException;

public:
    JavaScriptJob(QV4::ExecutionEngine *engine, int frameNr, const QString &script);
    void run();
    bool hasExeption() const;

protected:
    virtual void handleResult(QV4::ScopedValue &result) = 0;
};

class ValueLookupJob: public QV4DebugJob
{
    QV4DataCollector *collector;
    const QJsonArray handles;
    QJsonObject result;
    QJsonArray collectedRefs;
    QString exception;

public:
    ValueLookupJob(const QJsonArray &handles, QV4DataCollector *collector);
    void run();
    const QString &exceptionMessage() const;
    const QJsonObject &returnValue() const;
    const QJsonArray &refs() const;
};

class ExpressionEvalJob: public JavaScriptJob
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

class GatherSourcesJob: public QV4DebugJob
{
    QV4::ExecutionEngine *engine;
    QStringList sources;

public:
    GatherSourcesJob(QV4::ExecutionEngine *engine);
    void run();
    const QStringList &result() const;
};

class ArgumentCollectJob: public QV4DebugJob
{
    QV4::ExecutionEngine *engine;
    QV4DataCollector *collector;
    QStringList *names;
    int frameNr;
    int scopeNr;

public:
    ArgumentCollectJob(QV4::ExecutionEngine *engine, QV4DataCollector *collector,
                       QStringList *names, int frameNr, int scopeNr);
    void run();
};

class LocalCollectJob: public QV4DebugJob
{
    QV4::ExecutionEngine *engine;
    QV4DataCollector *collector;
    QStringList *names;
    int frameNr;
    int scopeNr;

public:
    LocalCollectJob(QV4::ExecutionEngine *engine, QV4DataCollector *collector, QStringList *names,
                    int frameNr, int scopeNr);
    void run();
};

class ThisCollectJob: public QV4DebugJob
{
    QV4::ExecutionEngine *engine;
    QV4DataCollector *collector;
    int frameNr;
    QV4DataCollector::Ref thisRef;

public:
    ThisCollectJob(QV4::ExecutionEngine *engine, QV4DataCollector *collector, int frameNr);
    void run();
    QV4DataCollector::Ref foundRef() const;
};

class ExceptionCollectJob: public QV4DebugJob
{
    QV4::ExecutionEngine *engine;
    QV4DataCollector *collector;
    QV4DataCollector::Ref exception;

public:
    ExceptionCollectJob(QV4::ExecutionEngine *engine, QV4DataCollector *collector);
    void run();
    QV4DataCollector::Ref exceptionValue() const;
};

class EvalJob: public JavaScriptJob
{
    bool result;

public:
    EvalJob(QV4::ExecutionEngine *engine, const QString &script);

    virtual void handleResult(QV4::ScopedValue &result);
    bool resultAsBoolean() const
;
};

QT_END_NAMESPACE

#endif // QV4DEBUGJOB_H

