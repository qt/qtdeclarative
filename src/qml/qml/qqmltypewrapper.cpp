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

DEFINE_MANAGED_VTABLE(QmlTypeWrapper);

QmlTypeWrapper::QmlTypeWrapper(QV8Engine *engine)
    : Object(QV8Engine::getV4(engine)),
      v8(engine), mode(IncludeEnums), type(0), typeNamespace(0), importNamespace(0)
{
    vtbl = &static_vtbl;
}

QmlTypeWrapper::~QmlTypeWrapper()
{
    if (typeNamespace)
        typeNamespace->release();
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
Value QmlTypeWrapper::create(QV8Engine *v8, QObject *o, QQmlType *t, TypeNameMode mode)
{
    Q_ASSERT(t);
    ExecutionEngine *v4 = QV8Engine::getV4(v8);

    QmlTypeWrapper *w = new (v4->memoryManager) QmlTypeWrapper(v8);
    w->mode = mode; w->object = o; w->type = t;
    return Value::fromObject(w);
}

// Returns a type wrapper for importNamespace (of t) on o.  This allows nested resolution of a type in a
// namespace.
Value QmlTypeWrapper::create(QV8Engine *v8, QObject *o, QQmlTypeNameCache *t, const void *importNamespace, TypeNameMode mode)
{
    Q_ASSERT(t);
    Q_ASSERT(importNamespace);
    ExecutionEngine *v4 = QV8Engine::getV4(v8);

    QmlTypeWrapper *w = new (v4->memoryManager) QmlTypeWrapper(v8);
    w->mode = mode; w->object = o; w->typeNamespace = t; w->importNamespace = importNamespace;
    t->addref();
    return Value::fromObject(w);
}


ReturnedValue QmlTypeWrapper::get(Managed *m, String *name, bool *hasProperty)
{
    QmlTypeWrapper *w = m->as<QmlTypeWrapper>();
    QV4::ExecutionEngine *v4 = m->engine();
    if (!w)
        v4->current->throwTypeError();

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
                                return QV4::Value::fromInt32(value).asReturnedValue();
                        }
                    }
                }

                // check for property.
                return QV4::QObjectWrapper::getQmlProperty(v4->current, context, qobjectSingleton, name, QV4::QObjectWrapper::IgnoreRevision, hasProperty).asReturnedValue();
            } else if (!siinfo->scriptApi(e).isUndefined()) {
                QV4::ExecutionEngine *engine = QV8Engine::getV4(v8engine);
                // NOTE: if used in a binding, changes will not trigger re-evaluation since non-NOTIFYable.
                QV4::Object *o = QJSValuePrivate::get(siinfo->scriptApi(e))->getValue(engine).asObject();
                if (o)
                    return o->get(name);
            }

            // Fall through to base implementation

        } else {

            if (name->startsWithUpper()) {
                bool ok = false;
                int value = type->enumValue(name, &ok);
                if (ok)
                    return QV4::Value::fromInt32(value).asReturnedValue();

                // Fall through to base implementation

            } else if (w->object) {
                QObject *ao = qmlAttachedPropertiesObjectById(type->attachedPropertiesId(), object);
                if (ao)
                    return QV4::QObjectWrapper::getQmlProperty(v4->current, context, ao, name, QV4::QObjectWrapper::IgnoreRevision, hasProperty).asReturnedValue();

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
                return create(w->v8, object, r.type, w->mode).asReturnedValue();
            } else if (r.scriptIndex != -1) {
                int index = r.scriptIndex;
                if (index < context->importedScripts.count())
                    return context->importedScripts.at(index).value().asReturnedValue();
            } else if (r.importNamespace) {
                return create(w->v8, object, context->imports, r.importNamespace).asReturnedValue();
            }

            return QV4::Value::undefinedValue().asReturnedValue();

        }

        // Fall through to base implementation

    } else {
        Q_ASSERT(!"Unreachable");
    }

    if (hasProperty)
        *hasProperty = false;
    return Object::get(m, name, hasProperty);
}


void QmlTypeWrapper::put(Managed *m, String *name, const Value &value)
{
    QmlTypeWrapper *w = m->as<QmlTypeWrapper>();
    QV4::ExecutionEngine *v4 = m->engine();
    if (!w)
        v4->current->throwTypeError();

    QV8Engine *v8engine = v4->v8Engine;
    QQmlContextData *context = v8engine->callingContext();

    QQmlType *type = w->type;
    if (type && !type->isSingleton() && w->object) {
        QObject *object = w->object;
        QObject *ao = qmlAttachedPropertiesObjectById(type->attachedPropertiesId(), object);
        if (ao) 
            QV4::QObjectWrapper::setQmlProperty(v4->current, context, ao, name, QV4::QObjectWrapper::IgnoreRevision, value);
    } else if (type && type->isSingleton()) {
        QQmlEngine *e = v8engine->engine();
        QQmlType::SingletonInstanceInfo *siinfo = type->singletonInstanceInfo();
        siinfo->init(e);

        QObject *qobjectSingleton = siinfo->qobjectApi(e);
        if (qobjectSingleton) {
            QV4::QObjectWrapper::setQmlProperty(v4->current, context, qobjectSingleton, name, QV4::QObjectWrapper::IgnoreRevision, value);
        } else if (!siinfo->scriptApi(e).isUndefined()) {
            QV4::Object *apiprivate = QJSValuePrivate::get(siinfo->scriptApi(e))->value.asObject();
            if (!apiprivate) {
                QString error = QLatin1String("Cannot assign to read-only property \"") + name->toQString() + QLatin1Char('\"');
                v4->current->throwError(error);
            } else {
                apiprivate->put(name, value);
            }
        }
    }
}

PropertyAttributes QmlTypeWrapper::query(const Managed *m, String *name)
{
    // ### Implement more efficiently.
    bool hasProperty = false;
    const_cast<Managed*>(m)->get(name, &hasProperty);
    return hasProperty ? Attr_Data : Attr_Invalid;
}

void QmlTypeWrapper::destroy(Managed *that)
{
    static_cast<QmlTypeWrapper *>(that)->~QmlTypeWrapper();
}

QT_END_NAMESPACE
