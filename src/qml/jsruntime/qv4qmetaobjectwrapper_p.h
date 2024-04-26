// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4QMETAOBJECTWRAPPER_P_H
#define QV4QMETAOBJECTWRAPPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qv4functionobject_p.h>
#include <private/qv4value_p.h>

#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyData;

namespace QV4 {
namespace Heap {

struct QMetaObjectWrapper : FunctionObject {
    const QMetaObject* metaObject;
    QQmlPropertyData *constructors;
    int constructorCount;

    void init(const QMetaObject* metaObject);
    void destroy();
    void ensureConstructorsCache();
};

} // namespace Heap

struct Q_QML_EXPORT QMetaObjectWrapper : public FunctionObject
{
    V4_OBJECT2(QMetaObjectWrapper, FunctionObject)
    V4_NEEDS_DESTROY

    static ReturnedValue create(ExecutionEngine *engine, const QMetaObject* metaObject);
    const QMetaObject *metaObject() const { return d()->metaObject; }

protected:
    static ReturnedValue virtualCallAsConstructor(
            const FunctionObject *, const Value *argv, int argc, const Value *);
    static bool virtualIsEqualTo(Managed *a, Managed *b);

private:
    void init(ExecutionEngine *engine);
    ReturnedValue constructInternal(const Value *argv, int argc) const;
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4QMETAOBJECTWRAPPER_P_H


