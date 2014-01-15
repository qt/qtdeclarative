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

#include "qqmltypewrapper_p.h"
#include <private/qqmlcontextwrapper_p.h>
#include <private/qv8engine_p.h>

#include <private/qqmlengine_p.h>
#include <private/qqmlcontext_p.h>

#include <private/qjsvalue_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4objectproto_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

DEFINE_OBJECT_VTABLE(QmlTypeWrapper);

QmlTypeWrapper::QmlTypeWrapper(QV8Engine *engine)
    : Object(QV8Engine::getV4(engine)),
      v8(engine), mode(IncludeEnums), type(0), typeNamespace(0), importNamespace(0)
{
    setVTable(staticVTable());
}

QmlTypeWrapper::~QmlTypeWrapper()
{
    if (typeNamespace)
        typeNamespace->release();
}

bool QmlTypeWrapper::isSingleton() const
{
    return type && type->isSingleton();
}

QVariant QmlTypeWrapper::toVariant() const
{
    if (type && type->isSingleton()) {
        QQmlEngine *e = v8->engine();
        QQmlType::SingletonInstanceInfo *siinfo = type->singletonInstanceInfo();
        siinfo->init(e); // note: this will also create QJSValue singleton which isn't strictly required.
        QObject *qobjectSingleton = siinfo->qobjectApi(e);
        if (qobjectSingleton) {
            return QVariant::fromValue<QObject*>(qobjectSingleton);
        }
    }

    // only QObject Singleton Type can be converted to a variant.
    return QVariant();
}


// Returns a type wrapper for type t on o.  This allows access of enums, and attached properties.
ReturnedValue QmlTypeWrapper::create(QV8Engine *v8, QObject *o, QQmlType *t, TypeNameMode mode)
{
    Q_ASSERT(t);
    ExecutionEngine *v4 = QV8Engine::getV4(v8);
    Scope scope(v4);

    Scoped<QmlTypeWrapper> w(scope, new (v4->memoryManager) QmlTypeWrapper(v8));
    w->mode = mode; w->object = o; w->type = t;
    return w.asReturnedValue();
}

// Returns a type wrapper for importNamespace (of t) on o.  This allows nested resolution of a type in a
// namespace.
ReturnedValue QmlTypeWrapper::create(QV8Engine *v8, QObject *o, QQmlTypeNameCache *t, const void *importNamespace, TypeNameMode mode)
{
    Q_ASSERT(t);
    Q_ASSERT(importNamespace);
    ExecutionEngine *v4 = QV8Engine::getV4(v8);
    Scope scope(v4);

    Scoped<QmlTypeWrapper> w(scope, new (v4->memoryManager) QmlTypeWrapper(v8));
    w->mode = mode; w->object = o; w->typeNamespace = t; w->importNamespace = importNamespace;
    t->addref();
    return w.asReturnedValue();
}


ReturnedValue QmlTypeWrapper::get(Managed *m, const StringRef name, bool *hasProperty)
{
    QV4::ExecutionEngine *v4 = m->engine();
    QV4::Scope scope(v4);

    Scoped<QmlTypeWrapper> w(scope,  m->as<QmlTypeWrapper>());
    if (!w)
        return v4->currentContext()->throwTypeError();


    if (hasProperty)
        *hasProperty = true;

    QV8Engine *v8engine = w->v8;
    QQmlContextData *context = v8engine->callingContext();

    QObject *object = w->object;

    if (w->type) {
        QQmlType *type = w->type;

        // singleton types are handled differently to other types.
        if (type->isSingleton()) {
            QQmlEngine *e = v8engine->engine();
            QQmlType::SingletonInstanceInfo *siinfo = type->singletonInstanceInfo();
            siinfo->init(e);

            QObject *qobjectSingleton = siinfo->qobjectApi(e);
            if (qobjectSingleton) {
                // check for enum value
                if (name->startsWithUpper()) {
                    if (w->mode == IncludeEnums) {
                        // ### Optimize
                        QByteArray enumName = name->toQString().toUtf8();
                        const QMetaObject *metaObject = qobjectSingleton->metaObject();
                        for (int ii = metaObject->enumeratorCount() - 1; ii >= 0; --ii) {
                            QMetaEnum e = metaObject->enumerator(ii);
                            bool ok;
                            int value = e.keyToValue(enumName.constData(), &ok);
                            if (ok)
                                return QV4::Primitive::fromInt32(value).asReturnedValue();
                        }
                    }
                }

                // check for property.
                return QV4::QObjectWrapper::getQmlProperty(v4->currentContext(), context, qobjectSingleton, name.getPointer(), QV4::QObjectWrapper::IgnoreRevision, hasProperty);
            } else if (!siinfo->scriptApi(e).isUndefined()) {
                // NOTE: if used in a binding, changes will not trigger re-evaluation since non-NOTIFYable.
                QV4::ScopedObject o(scope, QJSValuePrivate::get(siinfo->scriptApi(e))->getValue(v4));
                if (!!o)
                    return o->get(name);
            }

            // Fall through to base implementation

        } else {

            if (name->startsWithUpper()) {
                bool ok = false;
                int value = type->enumValue(name, &ok);
                if (ok)
                    return QV4::Primitive::fromInt32(value).asReturnedValue();

                // Fall through to base implementation

            } else if (w->object) {
                QObject *ao = qmlAttachedPropertiesObjectById(type->attachedPropertiesId(), object);
                if (ao)
                    return QV4::QObjectWrapper::getQmlProperty(v4->currentContext(), context, ao, name.getPointer(), QV4::QObjectWrapper::IgnoreRevision, hasProperty);

                // Fall through to base implementation
            }

            // Fall through to base implementation
        }

        // Fall through to base implementation

    } else if (w->typeNamespace) {
        Q_ASSERT(w->importNamespace);
        QQmlTypeNameCache::Result r = w->typeNamespace->query(name, w->importNamespace);

        if (r.isValid()) {
            QQmlContextData *context = v8engine->callingContext();
            if (r.type) {
                return create(w->v8, object, r.type, w->mode);
            } else if (r.scriptIndex != -1) {
                QV4::ScopedObject scripts(scope, context->importedScripts);
                return scripts->getIndexed(r.scriptIndex);
            } else if (r.importNamespace) {
                return create(w->v8, object, context->imports, r.importNamespace);
            }

            return QV4::Encode::undefined();

        }

        // Fall through to base implementation

    } else {
        Q_ASSERT(!"Unreachable");
    }

    if (hasProperty)
        *hasProperty = false;
    return Object::get(m, name, hasProperty);
}


void QmlTypeWrapper::put(Managed *m, const StringRef name, const ValueRef value)
{
    QmlTypeWrapper *w = m->as<QmlTypeWrapper>();
    QV4::ExecutionEngine *v4 = m->engine();
    if (v4->hasException)
        return;
    if (!w) {
        v4->currentContext()->throwTypeError();
        return;
    }

    QV4::Scope scope(v4);
    QV8Engine *v8engine = v4->v8Engine;
    QQmlContextData *context = v8engine->callingContext();

    QQmlType *type = w->type;
    if (type && !type->isSingleton() && w->object) {
        QObject *object = w->object;
        QObject *ao = qmlAttachedPropertiesObjectById(type->attachedPropertiesId(), object);
        if (ao)
            QV4::QObjectWrapper::setQmlProperty(v4->currentContext(), context, ao, name.getPointer(), QV4::QObjectWrapper::IgnoreRevision, value);
    } else if (type && type->isSingleton()) {
        QQmlEngine *e = v8engine->engine();
        QQmlType::SingletonInstanceInfo *siinfo = type->singletonInstanceInfo();
        siinfo->init(e);

        QObject *qobjectSingleton = siinfo->qobjectApi(e);
        if (qobjectSingleton) {
            QV4::QObjectWrapper::setQmlProperty(v4->currentContext(), context, qobjectSingleton, name.getPointer(), QV4::QObjectWrapper::IgnoreRevision, value);
        } else if (!siinfo->scriptApi(e).isUndefined()) {
            QV4::ScopedObject apiprivate(scope, QJSValuePrivate::get(siinfo->scriptApi(e))->value);
            if (!apiprivate) {
                QString error = QLatin1String("Cannot assign to read-only property \"") + name->toQString() + QLatin1Char('\"');
                v4->currentContext()->throwError(error);
                return;
            } else {
                apiprivate->put(name, value);
            }
        }
    }
}

PropertyAttributes QmlTypeWrapper::query(const Managed *m, StringRef name)
{
    // ### Implement more efficiently.
    Scope scope(m->engine());
    ScopedString n(scope, name);
    bool hasProperty = false;
    static_cast<Object *>(const_cast<Managed*>(m))->get(n, &hasProperty);
    return hasProperty ? Attr_Data : Attr_Invalid;
}

void QmlTypeWrapper::destroy(Managed *that)
{
    static_cast<QmlTypeWrapper *>(that)->~QmlTypeWrapper();
}

bool QmlTypeWrapper::isEqualTo(Managed *a, Managed *b)
{
    QV4::QmlTypeWrapper *qmlTypeWrapperA = a->asObject()->as<QV4::QmlTypeWrapper>();
    if (QV4::QmlTypeWrapper *qmlTypeWrapperB = b->asObject()->as<QV4::QmlTypeWrapper>())
        return qmlTypeWrapperA->toVariant() == qmlTypeWrapperB->toVariant();
    else if (QV4::QObjectWrapper *qobjectWrapper = b->as<QV4::QObjectWrapper>())
        return qmlTypeWrapperA->toVariant().value<QObject*>() == qobjectWrapper->object();

    return false;
}

QT_END_NAMESPACE
