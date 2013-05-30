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

#include "qqmllistwrapper_p.h"
#include <private/qv8engine_p.h>
#include <private/qqmllist_p.h>

#include <private/qv4functionobject_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

DEFINE_MANAGED_VTABLE(QmlListWrapper);

QmlListWrapper::QmlListWrapper(QV8Engine *engine)
    : Object(QV8Engine::getV4(engine)),
      v8(engine)
{
    type = Type_QmlListWrapper;
    vtbl = &static_vtbl;
}

QmlListWrapper::~QmlListWrapper()
{
}

Value QmlListWrapper::create(QV8Engine *v8, QObject *object, int propId, int propType)
{
    if (!object || propId == -1)
        return Value::nullValue();

    ExecutionEngine *v4 = QV8Engine::getV4(v8);

    QmlListWrapper *r = new (v4->memoryManager) QmlListWrapper(v8);
    r->object = object;
    r->propertyType = propType;
    void *args[] = { &r->property, 0 };
    QMetaObject::metacall(object, QMetaObject::ReadProperty, propId, args);
    return Value::fromObject(r);
}

Value QmlListWrapper::create(QV8Engine *v8, const QQmlListProperty<QObject> &prop, int propType)
{
    ExecutionEngine *v4 = QV8Engine::getV4(v8);

    QmlListWrapper *r = new (v4->memoryManager) QmlListWrapper(v8);
    r->object = prop.object;
    r->property = prop;
    r->propertyType = propType;
    return Value::fromObject(r);
}

QVariant QmlListWrapper::toVariant() const
{
    if (!object)
        return QVariant();

    return QVariant::fromValue(QQmlListReferencePrivate::init(property, propertyType, v8->engine()));
}


Value QmlListWrapper::get(Managed *m, ExecutionContext *ctx, String *name, bool *hasProperty)
{
    QmlListWrapper *w = m->asQmlListWrapper();
    if (!w)
        ctx->throwTypeError();

    if (name == ctx->engine->id_length && !w->object.isNull()) {
        quint32 count = w->property.count ? w->property.count(&w->property) : 0;
        return Value::fromUInt32(count);
    }

    return Value::undefinedValue();
}

Value QmlListWrapper::getIndexed(Managed *m, ExecutionContext *ctx, uint index, bool *hasProperty)
{
    QmlListWrapper *w = m->asQmlListWrapper();
    if (!w)
        ctx->throwTypeError();

    quint32 count = w->property.count ? w->property.count(&w->property) : 0;
    if (index < count && w->property.at)
        return w->v8->newQObject(w->property.at(&w->property, index));

    return Value::undefinedValue();
}

void QmlListWrapper::put(Managed *m, ExecutionContext *ctx, String *name, const Value &value)
{
    // doesn't do anything. Should we throw?
    Q_UNUSED(m);
    Q_UNUSED(ctx);
    Q_UNUSED(name);
    Q_UNUSED(value);
}

void QmlListWrapper::destroy(Managed *that)
{
    QmlListWrapper *w = that->asQmlListWrapper();
    w->~QmlListWrapper();
}

#if 0
// ### does this need porting?
v8::Handle<v8::Array> QV8ListWrapper::Enumerator(const v8::AccessorInfo &info)
{
    QV8ListResource *resource = v8_resource_cast<QV8ListResource>(info.This());

    if (!resource || resource->object.isNull()) return v8::Array::New();

    quint32 count = resource->property.count?resource->property.count(&resource->property):0;

    v8::Handle<v8::Array> rv = v8::Array::New(count);

    for (uint ii = 0; ii < count; ++ii)
        rv->Set(ii, Value::fromDouble(ii));

    return rv;
}
#endif

QT_END_NAMESPACE
