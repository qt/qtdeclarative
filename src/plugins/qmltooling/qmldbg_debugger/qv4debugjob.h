/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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

