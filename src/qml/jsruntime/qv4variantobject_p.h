/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QV4VARIANTOBJECT_P_H
#define QV4VARIANTOBJECT_P_H

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

#include <QtCore/qglobal.h>
#include <QtQml/qqmllist.h>
#include <QtCore/qvariant.h>

#include <private/qv4value_p.h>
#include <private/qv4object_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct VariantObject : Object
{
    void init();
    void init(const QVariant &value);
    void destroy() {
        Q_ASSERT(scarceData);
        if (isScarce())
            addVmePropertyReference();
        delete scarceData;
        Object::destroy();
    }
    bool isScarce() const;
    int vmePropertyReferenceCount;

    const QVariant &data() const { return scarceData->data; }
    QVariant &data() { return scarceData->data; }

    void addVmePropertyReference() { scarceData->node.remove(); }
    void removeVmePropertyReference() { internalClass->engine->scarceResources.insert(scarceData); }

private:
    ExecutionEngine::ScarceResourceData *scarceData;
};

}

struct VariantObject : Object
{
    V4_OBJECT2(VariantObject, Object)
    V4_PROTOTYPE(variantPrototype)
    V4_NEEDS_DESTROY

    void addVmePropertyReference() const;
    void removeVmePropertyReference() const;

protected:
    static bool virtualIsEqualTo(Managed *m, Managed *other);
};

struct VariantPrototype : VariantObject
{
public:
    V4_PROTOTYPE(objectPrototype)
    void init();

    static ReturnedValue method_preserve(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_destroy(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_valueOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif

