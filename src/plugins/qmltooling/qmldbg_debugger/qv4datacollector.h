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
#include <private/qv4persistent_p.h>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

QT_BEGIN_NAMESPACE

class QV4Debugger;
class QV4DataCollector
{
public:
    typedef uint Ref;
    typedef QVector<uint> Refs;
    static const Ref s_invalidRef;

    static QV4::CallContext *findContext(QV4::ExecutionEngine *engine, int frame);
    static QV4::Heap::CallContext *findScope(QV4::ExecutionContext *ctxt, int scope);
    static QVector<QV4::Heap::ExecutionContext::ContextType> getScopeTypes(
            QV4::ExecutionEngine *engine, int frame);

    QV4DataCollector(QV4::ExecutionEngine *engine);

    Ref collect(const QV4::ScopedValue &value);
    Ref addFunctionRef(const QString &functionName);
    Ref addScriptRef(const QString &scriptName);

    bool isValidRef(Ref ref) const;
    QJsonObject lookupRef(Ref ref);

    void collectScope(QJsonObject *dict, QV4Debugger *debugger, int frameNr, int scopeNr);

    QV4::ExecutionEngine *engine() const { return m_engine; }
    QJsonArray flushCollectedRefs();
    void clear();

private:
    Ref addRef(QV4::Value value, bool deduplicate = true);
    QV4::ReturnedValue getValue(Ref ref);
    bool lookupSpecialRef(Ref ref, QJsonObject *dict);

    QJsonArray collectProperties(const QV4::Object *object);
    QJsonObject collectAsJson(const QString &name, const QV4::ScopedValue &value);
    void collectArgumentsInContext();

    QV4::ExecutionEngine *m_engine;
    Refs m_collectedRefs;
    QV4::PersistentValue m_values;
    typedef QHash<Ref, QJsonObject> SpecialRefs;
    SpecialRefs m_specialRefs;
};

QT_END_NAMESPACE

#endif // QV4DATACOLLECTOR_H
