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

#include "qv4datacollector.h"

#include <private/qv4script_p.h>
#include <private/qv4string_p.h>
#include <private/qv4objectiterator_p.h>
#include <private/qv4identifier_p.h>
#include <private/qv4runtime_p.h>

#include <QtCore/qjsonarray.h>

QT_BEGIN_NAMESPACE

QV4::Heap::CallContext *QV4DataCollector::findContext(QV4::ExecutionEngine *engine, int frame)
{
    QV4::ExecutionContext *ctx = engine->currentExecutionContext;
    while (ctx) {
        QV4::CallContext *cCtxt = ctx->asCallContext();
        if (cCtxt && cCtxt->d()->function) {
            if (frame < 1)
                return cCtxt->d();
            --frame;
        }
        ctx = engine->parentContext(ctx);
    }

    return 0;
}

QV4::Heap::CallContext *QV4DataCollector::findScope(QV4::Heap::ExecutionContext *ctxt, int scope)
{
    if (!ctxt)
        return 0;

    QV4::Scope s(ctxt->engine);
    QV4::ScopedContext ctx(s, ctxt);
    for (; scope > 0 && ctx; --scope)
        ctx = ctx->d()->outer;

    return (ctx && ctx->d()) ? ctx->asCallContext()->d() : 0;
}

QVector<QV4::Heap::ExecutionContext::ContextType> QV4DataCollector::getScopeTypes(
        QV4::ExecutionEngine *engine, int frame)
{
    QVector<QV4::Heap::ExecutionContext::ContextType> types;

    QV4::Scope scope(engine);
    QV4::Scoped<QV4::CallContext> sctxt(scope, findContext(engine, frame));
    if (!sctxt || sctxt->d()->type < QV4::Heap::ExecutionContext::Type_QmlContext)
        return types;

    QV4::ScopedContext it(scope, sctxt->d());
    for (; it; it = it->d()->outer)
        types.append(it->d()->type);

    return types;
}


QV4DataCollector::QV4DataCollector(QV4::ExecutionEngine *engine)
    : m_engine(engine), m_collectedRefs(Q_NULLPTR)
{
    values.set(engine, engine->newArrayObject());
}

QV4DataCollector::~QV4DataCollector()
{
}

void QV4DataCollector::collect(const QV4::ScopedValue &value)
{
    if (m_collectedRefs)
        m_collectedRefs->append(addRef(value));
}

const QV4::Object *collectProperty(const QV4::ScopedValue &value, QV4::ExecutionEngine *engine,
                                   QJsonObject &dict)
{
    QV4::Scope scope(engine);
    QV4::ScopedValue typeString(scope, QV4::Runtime::typeofValue(engine, value));
    dict.insert(QStringLiteral("type"), typeString->toQStringNoThrow());

    const QLatin1String valueKey("value");
    switch (value->type()) {
    case QV4::Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
        return 0;
    case QV4::Value::Undefined_Type:
        dict.insert(valueKey, QJsonValue::Undefined);
        return 0;
    case QV4::Value::Null_Type:
        // "null" is not the correct type, but we leave this in until QtC can deal with "object"
        dict.insert(QStringLiteral("type"), QStringLiteral("null"));
        dict.insert(valueKey, QJsonValue::Null);
        return 0;
    case QV4::Value::Boolean_Type:
        dict.insert(valueKey, value->booleanValue());
        return 0;
    case QV4::Value::Managed_Type:
        if (const QV4::String *s = value->as<QV4::String>()) {
            dict.insert(valueKey, s->toQString());
        } else if (const QV4::ArrayObject *a = value->as<QV4::ArrayObject>()) {
            // size of an array is number of its numerical properties; We don't consider free form
            // object properties here.
            dict.insert(valueKey, qint64(a->getLength()));
            return a;
        } else if (const QV4::Object *o = value->as<QV4::Object>()) {
            int numProperties = 0;
            QV4::ObjectIterator it(scope, o, QV4::ObjectIterator::EnumerableOnly);
            QV4::PropertyAttributes attrs;
            uint index;
            QV4::ScopedProperty p(scope);
            QV4::ScopedString name(scope);
            while (true) {
                it.next(name.getRef(), &index, p, &attrs);
                if (attrs.isEmpty())
                    break;
                else
                    ++numProperties;
            }
            dict.insert(valueKey, numProperties);
            return o;
        } else {
            Q_UNREACHABLE();
        }
        return 0;
    case QV4::Value::Integer_Type:
        dict.insert(valueKey, value->integerValue());
        return 0;
    default: // double
        dict.insert(valueKey, value->doubleValue());
        return 0;
    }
}

QJsonObject QV4DataCollector::lookupRef(Ref ref)
{
    QJsonObject dict;
    if (lookupSpecialRef(ref, &dict))
        return dict;

    dict.insert(QStringLiteral("handle"), qint64(ref));
    QV4::Scope scope(engine());
    QV4::ScopedValue value(scope, getValue(ref));

    if (const QV4::Object *o = collectProperty(value, engine(), dict))
        dict.insert(QStringLiteral("properties"), collectProperties(o));

    return dict;
}

QV4DataCollector::Ref QV4DataCollector::addFunctionRef(const QString &functionName)
{
    Ref ref = addRef(QV4::Primitive::emptyValue(), false);

    QJsonObject dict;
    dict.insert(QStringLiteral("handle"), qint64(ref));
    dict.insert(QStringLiteral("type"), QStringLiteral("function"));
    dict.insert(QStringLiteral("name"), functionName);
    specialRefs.insert(ref, dict);

    return ref;
}

QV4DataCollector::Ref QV4DataCollector::addScriptRef(const QString &scriptName)
{
    Ref ref = addRef(QV4::Primitive::emptyValue(), false);

    QJsonObject dict;
    dict.insert(QStringLiteral("handle"), qint64(ref));
    dict.insert(QStringLiteral("type"), QStringLiteral("script"));
    dict.insert(QStringLiteral("name"), scriptName);
    specialRefs.insert(ref, dict);

    return ref;
}

void QV4DataCollector::collectScope(QJsonObject *dict, QV4::Debugging::Debugger *debugger,
                                    int frameNr, int scopeNr)
{
    QStringList names;

    Refs refs;
    if (debugger->state() == QV4::Debugging::Debugger::Paused) {
        RefHolder holder(this, &refs);
        ArgumentCollectJob argumentsJob(m_engine, this, &names, frameNr, scopeNr);
        debugger->runInEngine(&argumentsJob);
        LocalCollectJob localsJob(m_engine, this, &names, frameNr, scopeNr);
        debugger->runInEngine(&localsJob);
    }

    QV4::Scope scope(engine());
    QV4::ScopedObject scopeObject(scope, engine()->newObject());

    Q_ASSERT(names.size() == refs.size());
    for (int i = 0, ei = refs.size(); i != ei; ++i)
        scopeObject->put(engine(), names.at(i),
                         QV4::Value::fromReturnedValue(getValue(refs.at(i))));

    Ref scopeObjectRef = addRef(scopeObject);
    dict->insert(QStringLiteral("ref"), qint64(scopeObjectRef));
    if (m_collectedRefs)
        m_collectedRefs->append(scopeObjectRef);
}

QV4DataCollector::Ref QV4DataCollector::addRef(QV4::Value value, bool deduplicate)
{
    class ExceptionStateSaver
    {
        quint32 *hasExceptionLoc;
        quint32 hadException;

    public:
        ExceptionStateSaver(QV4::ExecutionEngine *engine)
            : hasExceptionLoc(&engine->hasException)
            , hadException(false)
        { std::swap(*hasExceptionLoc, hadException); }

        ~ExceptionStateSaver()
        { std::swap(*hasExceptionLoc, hadException); }
    };

    // if we wouldn't do this, the putIndexed won't work.
    ExceptionStateSaver resetExceptionState(engine());
    QV4::Scope scope(engine());
    QV4::ScopedObject array(scope, values.value());
    if (deduplicate) {
        for (Ref i = 0; i < array->getLength(); ++i) {
            if (array->getIndexed(i) == value.rawValue() && !specialRefs.contains(i))
                return i;
        }
    }
    Ref ref = array->getLength();
    array->putIndexed(ref, value);
    Q_ASSERT(array->getLength() - 1 == ref);
    return ref;
}

QV4::ReturnedValue QV4DataCollector::getValue(Ref ref)
{
    QV4::Scope scope(engine());
    QV4::ScopedObject array(scope, values.value());
    Q_ASSERT(ref < array->getLength());
    return array->getIndexed(ref, Q_NULLPTR);
}

bool QV4DataCollector::lookupSpecialRef(Ref ref, QJsonObject *dict)
{
    SpecialRefs::const_iterator it = specialRefs.find(ref);
    if (it == specialRefs.end())
        return false;

    *dict = it.value();
    return true;
}

QJsonArray QV4DataCollector::collectProperties(const QV4::Object *object)
{
    QJsonArray res;

    QV4::Scope scope(engine());
    QV4::ObjectIterator it(scope, object, QV4::ObjectIterator::EnumerableOnly);
    QV4::ScopedValue name(scope);
    QV4::ScopedValue value(scope);
    while (true) {
        QV4::Value v;
        name = it.nextPropertyNameAsString(&v);
        if (name->isNull())
            break;
        QString key = name->toQStringNoThrow();
        value = v;
        res.append(collectAsJson(key, value));
    }

    return res;
}

QJsonObject QV4DataCollector::collectAsJson(const QString &name, const QV4::ScopedValue &value)
{
    QJsonObject dict;
    if (!name.isNull())
        dict.insert(QStringLiteral("name"), name);
    if (value->isManaged() && !value->isString()) {
        Ref ref = addRef(value);
        dict.insert(QStringLiteral("ref"), qint64(ref));
        if (m_collectedRefs)
            m_collectedRefs->append(ref);
    }

    collectProperty(value, engine(), dict);
    return dict;
}

ExpressionEvalJob::ExpressionEvalJob(QV4::ExecutionEngine *engine, int frameNr,
                                     const QString &expression,
                                     QV4DataCollector *collector)
    : JavaScriptJob(engine, frameNr, expression)
    , collector(collector)
{
}

void ExpressionEvalJob::handleResult(QV4::ScopedValue &result)
{
    if (hasExeption())
        exception = result->toQStringNoThrow();
    collector->collect(result);
}

const QString &ExpressionEvalJob::exceptionMessage() const
{
    return exception;
}

GatherSourcesJob::GatherSourcesJob(QV4::ExecutionEngine *engine, int seq)
    : engine(engine)
    , seq(seq)
{}

void GatherSourcesJob::run()
{
    QStringList sources;

    foreach (QV4::CompiledData::CompilationUnit *unit, engine->compilationUnits) {
        QString fileName = unit->fileName();
        if (!fileName.isEmpty())
            sources.append(fileName);
    }

    QV4::Debugging::Debugger *debugger = engine->debugger;
    emit debugger->sourcesCollected(debugger, sources, seq);
}

ArgumentCollectJob::ArgumentCollectJob(QV4::ExecutionEngine *engine, QV4DataCollector *collector,
                                       QStringList *names, int frameNr, int scopeNr)
    : engine(engine)
    , collector(collector)
    , names(names)
    , frameNr(frameNr)
    , scopeNr(scopeNr)
{}

void ArgumentCollectJob::run()
{
    if (frameNr < 0)
        return;

    QV4::Scope scope(engine);
    QV4::Scoped<QV4::CallContext> ctxt(
                scope, QV4DataCollector::findScope(
                    QV4DataCollector::findContext(engine, frameNr), scopeNr));
    if (!ctxt)
        return;

    QV4::ScopedValue v(scope);
    int nFormals = ctxt->formalCount();
    for (unsigned i = 0, ei = nFormals; i != ei; ++i) {
        QString qName;
        if (QV4::Identifier *name = ctxt->formals()[nFormals - i - 1])
            qName = name->string;
        names->append(qName);
        v = ctxt->argument(i);
        collector->collect(v);
    }
}

LocalCollectJob::LocalCollectJob(QV4::ExecutionEngine *engine, QV4DataCollector *collector,
                                 QStringList *names, int frameNr, int scopeNr)
    : engine(engine)
    , collector(collector)
    , names(names)
    , frameNr(frameNr)
    , scopeNr(scopeNr)
{}

void LocalCollectJob::run()
{
    if (frameNr < 0)
        return;

    QV4::Scope scope(engine);
    QV4::Scoped<QV4::CallContext> ctxt(
                scope, QV4DataCollector::findScope(
                    QV4DataCollector::findContext(engine, frameNr), scopeNr));
    if (!ctxt)
        return;

    QV4::ScopedValue v(scope);
    for (unsigned i = 0, ei = ctxt->variableCount(); i != ei; ++i) {
        QString qName;
        if (QV4::Identifier *name = ctxt->variables()[i])
            qName = name->string;
        names->append(qName);
        v = ctxt->d()->locals[i];
        collector->collect(v);
    }
}

ThisCollectJob::ThisCollectJob(QV4::ExecutionEngine *engine, QV4DataCollector *collector,
                               int frameNr, bool *foundThis)
    : engine(engine)
    , collector(collector)
    , frameNr(frameNr)
    , foundThis(foundThis)
{}

void ThisCollectJob::run()
{
    *foundThis = myRun();
}

bool ThisCollectJob::myRun()
{
    QV4::Scope scope(engine);
    QV4::ScopedContext ctxt(scope, QV4DataCollector::findContext(engine, frameNr));
    while (ctxt) {
        if (QV4::CallContext *cCtxt = ctxt->asCallContext())
            if (cCtxt->d()->activation)
                break;
        ctxt = ctxt->d()->outer;
    }

    if (!ctxt)
        return false;

    QV4::ScopedValue o(scope, ctxt->asCallContext()->d()->activation);
    collector->collect(o);
    return true;
}

ExceptionCollectJob::ExceptionCollectJob(QV4::ExecutionEngine *engine, QV4DataCollector *collector)
    : engine(engine)
    , collector(collector)
{}

void ExceptionCollectJob::run()
{
    QV4::Scope scope(engine);
    QV4::ScopedValue v(scope, *engine->exceptionValue);
    collector->collect(v);
}

QT_END_NAMESPACE
