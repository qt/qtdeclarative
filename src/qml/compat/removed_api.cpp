// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_QML_BUILD_REMOVED_API

#include "qtqmlglobal.h"

QT_USE_NAMESPACE

#if QT_QML_REMOVED_SINCE(6, 5)

#include <QtQml/qjsengine.h>

QJSValue QJSEngine::create(int typeId, const void *ptr)
{
    QMetaType type(typeId);
    return create(type, ptr);
}

bool QJSEngine::convertV2(const QJSValue &value, int type, void *ptr)
{
    return convertV2(value, QMetaType(type), ptr);
}

#endif

#if QT_QML_REMOVED_SINCE(6, 6)
#include <QtQml/qqmlprivate.h>
#include <QtQml/private/qv4executablecompilationunit_p.h>
#include <QtQml/private/qv4lookup_p.h>

bool QQmlPrivate::AOTCompiledContext::getEnumLookup(uint index, int *target) const
{
    using namespace QQmlPrivate;
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    auto mt = QMetaType(l->qmlEnumValueLookup.metaType);
    QVariant buffer(mt);
    getEnumLookup(index, buffer.data());
    *target = buffer.toInt();
    return true;
}
#endif

#if QT_QML_REMOVED_SINCE(6, 9)
#include <QtQml/qqmlprivate.h>
#include <QtQml/private/qv4executablecompilationunit_p.h>
#include <QtQml/private/qv4lookup_p.h>
#include <QtQml/private/qv4qobjectwrapper_p.h>

bool QQmlPrivate::AOTCompiledContext::callObjectPropertyLookup(
        uint index, QObject *object, void **args, const QMetaType *types, int argc) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue thisObject(scope, QV4::QObjectWrapper::wrap(scope.engine, object));
    QV4::ScopedFunctionObject function(scope, l->getter(l, engine->handle(), thisObject));
    if (!function) {
        scope.engine->throwTypeError(
                QStringLiteral("Property '%1' of object [object Object] is not a function")
                        .arg(compilationUnit->runtimeStrings[l->nameIndex]->toQString()));
        return false;
    }

    function->call(object, args, types, argc);
    return !scope.hasException();
}

void QQmlPrivate::AOTCompiledContext::initCallObjectPropertyLookup(uint index) const
{
    Q_UNUSED(index);
    Q_ASSERT(engine->hasError());
    engine->handle()->amendException();
}

bool QQmlPrivate::AOTCompiledContext::callQmlContextPropertyLookup(
        uint index, void **args, const QMetaType *types, int argc) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue thisObject(scope);
    QV4::ScopedFunctionObject function(
            scope, l->qmlContextPropertyGetter(l, scope.engine, thisObject));
    if (!function) {
        scope.engine->throwTypeError(
                QStringLiteral("Property '%1' of object [null] is not a function").arg(
                        compilationUnit->runtimeStrings[l->nameIndex]->toQString()));
        return false;
    }

    function->call(qmlScopeObject, args, types, argc);
    return !scope.hasException();
}

void QQmlPrivate::AOTCompiledContext::initCallQmlContextPropertyLookup(uint index) const
{
    Q_UNUSED(index);
    Q_ASSERT(engine->hasError());
    engine->handle()->amendException();
}

#endif
