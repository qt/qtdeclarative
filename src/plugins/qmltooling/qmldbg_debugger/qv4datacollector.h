// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4DATACOLLECTOR_H
#define QV4DATACOLLECTOR_H

#include <private/qv4engine_p.h>
#include <private/qv4persistent_p.h>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

QT_BEGIN_NAMESPACE

class QV4Debugger;
class QV4DataCollector
{
public:
    typedef uint Ref;
    typedef QVector<uint> Refs;

    static QV4::Heap::ExecutionContext *findScope(QV4::Heap::ExecutionContext *ctxt, int scope);
    static int encodeScopeType(QV4::Heap::ExecutionContext::ContextType scopeType);

    QVector<QV4::Heap::ExecutionContext::ContextType> getScopeTypes(int frame);
    QV4::Heap::ExecutionContext *findContext(int frame);
    QV4::CppStackFrame *findFrame(int frame);

    QV4DataCollector(QV4::ExecutionEngine *engine);

    Ref addValueRef(const QV4::ScopedValue &value);

    bool isValidRef(Ref ref) const;
    QJsonObject lookupRef(Ref ref);

    bool collectScope(QJsonObject *dict, int frameNr, int scopeNr);
    QJsonObject buildFrame(const QV4::StackFrame &stackFrame, int frameNr);

    QV4::ExecutionEngine *engine() const { return m_engine; }
    void clear();

private:
    Ref addRef(QV4::Value value, bool deduplicate = true);
    QV4::ReturnedValue getValue(Ref ref);

    QJsonArray collectProperties(const QV4::Object *object);
    QJsonObject collectAsJson(const QString &name, const QV4::ScopedValue &value);

    QV4::ExecutionEngine *m_engine;
    QV4::PersistentValue m_values;
};

QT_END_NAMESPACE

#endif // QV4DATACOLLECTOR_H
