// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4variantobject_p.h"
#include "qv4functionobject_p.h"
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qv4qobjectwrapper_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

DEFINE_OBJECT_VTABLE(VariantObject);

void Heap::VariantObject::init()
{
    Object::init();
    scarceData = new ExecutionEngine::ScarceResourceData;
}

void Heap::VariantObject::init(const QMetaType type, const void *data)
{
    Object::init();
    scarceData = new ExecutionEngine::ScarceResourceData(type, data);
    if (isScarce())
        removeVmePropertyReference();
}

bool VariantObject::Data::isScarce() const
{
    int t = data().userType();
    return t == QMetaType::QPixmap || t == QMetaType::QImage;
}

bool VariantObject::virtualIsEqualTo(Managed *m, Managed *other)
{
    Q_ASSERT(m->as<QV4::VariantObject>());
    QV4::VariantObject *lv = static_cast<QV4::VariantObject *>(m);

    if (QV4::VariantObject *rv = other->as<QV4::VariantObject>())
        return lv->d()->data() == rv->d()->data();

    if (QV4::QQmlValueTypeWrapper *v = other->as<QQmlValueTypeWrapper>())
        return v->isEqual(lv->d()->data());

    return false;
}

void VariantObject::addVmePropertyReference() const
{
    if (d()->isScarce() && ++d()->vmePropertyReferenceCount == 1) {
        // remove from the ep->scarceResources list
        // since it is now no longer eligible to be
        // released automatically by the engine.
        d()->addVmePropertyReference();
    }
}

void VariantObject::removeVmePropertyReference() const
{
    if (d()->isScarce() && --d()->vmePropertyReferenceCount == 0) {
        // and add to the ep->scarceResources list
        // since it is now eligible to be released
        // automatically by the engine.
        d()->removeVmePropertyReference();
    }
}


void VariantPrototype::init()
{
    defineDefaultProperty(QStringLiteral("preserve"), method_preserve, 0);
    defineDefaultProperty(QStringLiteral("destroy"), method_destroy, 0);
    defineDefaultProperty(engine()->id_valueOf(), method_valueOf, 0);
    defineDefaultProperty(engine()->id_toString(), method_toString, 0);
}

ReturnedValue VariantPrototype::method_preserve(const FunctionObject *, const Value *thisObject, const Value *, int)
{
    const VariantObject *o = thisObject->as<QV4::VariantObject>();
    if (o && o->d()->isScarce())
        o->d()->addVmePropertyReference();
    RETURN_UNDEFINED();
}

ReturnedValue VariantPrototype::method_destroy(const FunctionObject *, const Value *thisObject, const Value *, int)
{
    const VariantObject *o = thisObject->as<QV4::VariantObject>();
    if (o) {
        if (o->d()->isScarce())
            o->d()->addVmePropertyReference();
        o->d()->data() = QVariant();
    }
    RETURN_UNDEFINED();
}

ReturnedValue VariantPrototype::method_toString(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    const VariantObject *o = thisObject->as<QV4::VariantObject>();
    if (!o)
        RETURN_UNDEFINED();
    const QVariant variant = o->d()->data();
    QString result = variant.toString();
    if (result.isEmpty() && !variant.canConvert(QMetaType(QMetaType::QString))) {
        QDebug dbg(&result);
        dbg << variant;
        // QDebug appends a space, we're not interested in continuing the stream so we chop it off.
        // Can't use nospace() because it would affect the debug-stream operator of the variant.
        result.chop(1);
    }
    return Encode(v4->newString(result));
}

ReturnedValue VariantPrototype::method_valueOf(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    const VariantObject *o = thisObject->as<QV4::VariantObject>();
    if (o) {
        QVariant v = o->d()->data();
        switch (v.userType()) {
        case QMetaType::UnknownType:
            return Encode::undefined();
        case QMetaType::QString:
            return Encode(b->engine()->newString(v.toString()));
        case QMetaType::Int:
            return Encode(v.toInt());
        case QMetaType::Double:
        case QMetaType::UInt:
            return Encode(v.toDouble());
        case QMetaType::Bool:
            return Encode(v.toBool());
        default:
            if (QMetaType(v.metaType()).flags() & QMetaType::IsEnumeration)
                if (v.metaType().sizeOf() <= qsizetype(sizeof(int)))
                    return Encode(v.toInt());
            if (v.canConvert<double>())
                return Encode(v.toDouble());
            if (v.canConvert<int>())
                return Encode(v.toInt());
            if (v.canConvert<uint>())
                return Encode(v.toUInt());
            if (v.canConvert<bool>())
                return Encode(v.toBool());
            if (v.canConvert<QString>())
                return Encode(b->engine()->newString(v.toString()));
            break;
        }
    }
    return thisObject->asReturnedValue();
}

QT_END_NAMESPACE
