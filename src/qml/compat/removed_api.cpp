// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    auto mt = QMetaType(lookup->qmlEnumValueLookup.metaType);
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
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue thisObject(scope, QV4::QObjectWrapper::wrap(scope.engine, object));
    QV4::ScopedFunctionObject function(scope, lookup->getter(engine->handle(), thisObject));
    if (!function) {
        scope.engine->throwTypeError(
                QStringLiteral("Property '%1' of object [object Object] is not a function")
                        .arg(compilationUnit->runtimeStrings[lookup->nameIndex]->toQString()));
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
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue thisObject(scope);
    QV4::ScopedFunctionObject function(
            scope, lookup->contextGetter(scope.engine, thisObject));
    if (!function) {
        scope.engine->throwTypeError(
                QStringLiteral("Property '%1' of object [null] is not a function").arg(
                        compilationUnit->runtimeStrings[lookup->nameIndex]->toQString()));
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

bool QQmlPrivate::AOTCompiledContext::loadGlobalLookup(uint index, void *target, QMetaType type) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue v(scope, lookup->globalGetter(engine->handle()));
    if (!QV4::ExecutionEngine::metaTypeFromJS(
                v, type, target)) {
        engine->handle()->throwTypeError();
        return false;
    }
    return true;
}

void QQmlPrivate::AOTCompiledContext::initLoadGlobalLookup(uint index) const
{
    Q_UNUSED(index);
    Q_ASSERT(engine->hasError());
    engine->handle()->amendException();
}

QVariant QQmlPrivate::AOTCompiledContext::constructValueType(
        QMetaType resultMetaType, const QMetaObject *resultMetaObject,
        int ctorIndex, void *ctorArg) const
{
    void *args[] = {ctorArg};
    return QQmlValueTypeProvider::constructValueType(
            resultMetaType, resultMetaObject, ctorIndex, args);
}

bool QQmlPrivate::AOTCompiledContext::callGlobalLookup(
        uint index, void **args, const QMetaType *types, int argc) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedFunctionObject function(scope, lookup->globalGetter(scope.engine));
    if (!function) {
        scope.engine->throwTypeError(
                QStringLiteral("Property '%1' of object [null] is not a function")
                        .arg(compilationUnit->runtimeStrings[lookup->nameIndex]->toQString()));
        return false;
    }

    function->call(nullptr, args, types, argc);
    return true;
}

void QQmlPrivate::AOTCompiledContext::initCallGlobalLookup(uint index) const
{
    Q_UNUSED(index);
    Q_ASSERT(engine->hasError());
    engine->handle()->amendException();
}

void QQmlPrivate::AOTCompiledContext::initLoadScopeObjectPropertyLookup(
        uint index, QMetaType type) const
{
    Q_UNUSED(type);
    return initLoadScopeObjectPropertyLookup(index);
}

void QQmlPrivate::AOTCompiledContext::initGetObjectLookup(
        uint index, QObject *object, QMetaType type) const
{
    return type == QMetaType::fromType<QVariant>()
            ? initGetObjectLookupAsVariant(index, object)
            : initGetObjectLookup(index, object);
}

void QQmlPrivate::AOTCompiledContext::initSetObjectLookup(
        uint index, QObject *object, QMetaType type) const
{
    return type == QMetaType::fromType<QVariant>()
            ? initSetObjectLookupAsVariant(index, object)
            : initSetObjectLookup(index, object);
}

void QQmlPrivate::AOTCompiledContext::initGetValueLookup(
        uint index, const QMetaObject *metaObject, QMetaType type) const {
    Q_UNUSED(type);
    initGetValueLookup(index, metaObject);
}

void QQmlPrivate::AOTCompiledContext::initSetValueLookup(
        uint index, const QMetaObject *metaObject, QMetaType type) const
{
    Q_UNUSED(type);
    initSetValueLookup(index, metaObject);
}

#endif
