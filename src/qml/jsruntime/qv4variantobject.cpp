/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4variantobject_p.h"
#include "qv4functionobject_p.h"
#include "qv4objectproto_p.h"
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qv8engine_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

DEFINE_OBJECT_VTABLE(VariantObject);

VariantObject::VariantObject(InternalClass *ic)
    : Object(ic)
    , ExecutionEngine::ScarceResourceData(QVariant())
    , m_vmePropertyReferenceCount(0)
{
}

VariantObject::VariantObject(ExecutionEngine *engine, const QVariant &value)
    : Object(engine->variantClass)
    , ExecutionEngine::ScarceResourceData(value)
    , m_vmePropertyReferenceCount(0)
{
    if (isScarce())
        engine->scarceResources.insert(this);
}

QVariant VariantObject::toVariant(const QV4::ValueRef v)
{
    if (v->asObject())
        return v->engine()->v8Engine->variantFromJS(v);

    if (v->isString())
        return QVariant(v->stringValue()->toQString());
    if (v->isBoolean())
        return QVariant(v->booleanValue());
    if (v->isNumber()) {
        QV4::Value val;
        val = v;
        if (val.isInt32())
            return QVariant(val.integerValue());
        return QVariant(v->asDouble());
    }
    if (v->isNull())
        return QVariant(QMetaType::VoidStar, 0);
    Q_ASSERT(v->isUndefined());
    return QVariant();
}

bool VariantObject::isScarce() const
{
    QVariant::Type t = data.type();
    return t == QVariant::Pixmap || t == QVariant::Image;
}

void VariantObject::destroy(Managed *that)
{
    VariantObject *v = static_cast<VariantObject *>(that);
    if (v->isScarce())
        v->node.remove();
    v->~VariantObject();
}

bool VariantObject::isEqualTo(Managed *m, Managed *other)
{
    QV4::VariantObject *lv = m->as<QV4::VariantObject>();
    assert(lv);

    if (QV4::VariantObject *rv = other->as<QV4::VariantObject>())
        return lv->data == rv->data;

    if (QV4::QmlValueTypeWrapper *v = other->as<QmlValueTypeWrapper>())
        return v->isEqual(lv->data);

    return false;
}

void VariantObject::addVmePropertyReference()
{
    if (isScarce() && ++m_vmePropertyReferenceCount == 1) {
        // remove from the ep->scarceResources list
        // since it is now no longer eligible to be
        // released automatically by the engine.
        node.remove();
    }
}

void VariantObject::removeVmePropertyReference()
{
    if (isScarce() && --m_vmePropertyReferenceCount == 0) {
        // and add to the ep->scarceResources list
        // since it is now eligible to be released
        // automatically by the engine.
        internalClass->engine->scarceResources.insert(this);
    }
}


VariantPrototype::VariantPrototype(InternalClass *ic)
    : VariantObject(ic)
{
}

void VariantPrototype::init()
{
    defineDefaultProperty(QStringLiteral("preserve"), method_preserve, 0);
    defineDefaultProperty(QStringLiteral("destroy"), method_destroy, 0);
    defineDefaultProperty(engine()->id_valueOf, method_valueOf, 0);
    defineDefaultProperty(engine()->id_toString, method_toString, 0);
}

QV4::ReturnedValue VariantPrototype::method_preserve(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<VariantObject> o(scope, ctx->callData->thisObject.as<QV4::VariantObject>());
    if (o && o->isScarce())
        o->node.remove();
    return Encode::undefined();
}

QV4::ReturnedValue VariantPrototype::method_destroy(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<VariantObject> o(scope, ctx->callData->thisObject.as<QV4::VariantObject>());
    if (o) {
        if (o->isScarce())
            o->node.remove();
        o->data = QVariant();
    }
    return Encode::undefined();
}

QV4::ReturnedValue VariantPrototype::method_toString(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<VariantObject> o(scope, ctx->callData->thisObject.as<QV4::VariantObject>());
    if (!o)
        return Encode::undefined();
    QString result = o->data.toString();
    if (result.isEmpty() && !o->data.canConvert(QVariant::String))
        result = QString::fromLatin1("QVariant(%0)").arg(QString::fromLatin1(o->data.typeName()));
    return Encode(ctx->engine->newString(result));
}

QV4::ReturnedValue VariantPrototype::method_valueOf(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<VariantObject> o(scope, ctx->callData->thisObject.as<QV4::VariantObject>());
    if (o) {
        QVariant v = o->data;
        switch (v.type()) {
        case QVariant::Invalid:
            return Encode::undefined();
        case QVariant::String:
            return Encode(ctx->engine->newString(v.toString()));
        case QVariant::Int:
            return Encode(v.toInt());
        case QVariant::Double:
        case QVariant::UInt:
            return Encode(v.toDouble());
        case QVariant::Bool:
            return Encode(v.toBool());
        default:
            break;
        }
    }
    return ctx->callData->thisObject.asReturnedValue();
}

QT_END_NAMESPACE
