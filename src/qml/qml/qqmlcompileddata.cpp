/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlcompiler_p.h"
#include "qqmlengine.h"
#include "qqmlcomponent.h"
#include "qqmlcomponent_p.h"
#include "qqmlcontext.h"
#include "qqmlcontext_p.h"
#include "qqmlpropertymap.h"
#ifdef QML_THREADED_VME_INTERPRETER
#include "qqmlvme_p.h"
#endif

#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

QQmlCompiledData::QQmlCompiledData(QQmlEngine *engine)
: engine(engine), importCache(0), metaTypeId(-1), listMetaTypeId(-1), isRegisteredWithEngine(false),
  rootPropertyCache(0), totalBindingsCount(0), totalParserStatusCount(0)
{
    Q_ASSERT(engine);
}

void QQmlCompiledData::destroy()
{
    if (engine && hasEngine())
        QQmlEnginePrivate::deleteInEngineThread(engine, this);
    else
        delete this;
}

QQmlCompiledData::~QQmlCompiledData()
{
    if (isRegisteredWithEngine)
        QQmlEnginePrivate::get(engine)->unregisterInternalCompositeType(this);

    clear();

    for (QHash<int, TypeReference*>::Iterator resolvedType = resolvedTypes.begin(), end = resolvedTypes.end();
         resolvedType != end; ++resolvedType) {
        if ((*resolvedType)->component)
            (*resolvedType)->component->release();
        if ((*resolvedType)->typePropertyCache)
            (*resolvedType)->typePropertyCache->release();
    }
    qDeleteAll(resolvedTypes);
    resolvedTypes.clear();

    for (int ii = 0; ii < propertyCaches.count(); ++ii)
        if (propertyCaches.at(ii))
            propertyCaches.at(ii)->release();

    for (int ii = 0; ii < scripts.count(); ++ii)
        scripts.at(ii)->release();

    if (importCache)
        importCache->release();

    if (rootPropertyCache)
        rootPropertyCache->release();
}

void QQmlCompiledData::clear()
{
}

/*!
Returns the property cache, if one alread exists.  The cache is not referenced.
*/
QQmlPropertyCache *QQmlCompiledData::TypeReference::propertyCache() const
{
    if (type)
        return typePropertyCache;
    else
        return component->rootPropertyCache;
}

/*!
Returns the property cache, creating one if it doesn't already exist.  The cache is not referenced.
*/
QQmlPropertyCache *QQmlCompiledData::TypeReference::createPropertyCache(QQmlEngine *engine)
{
    if (typePropertyCache) {
        return typePropertyCache;
    } else if (type) {
        typePropertyCache = QQmlEnginePrivate::get(engine)->cache(type->metaObject());
        typePropertyCache->addref();
        return typePropertyCache;
    } else {
        return component->rootPropertyCache;
    }
}

template <typename T>
bool qtTypeInherits(const QMetaObject *mo) {
    while (mo) {
        if (mo == &T::staticMetaObject)
            return true;
        mo = mo->superClass();
    }
    return false;
}

void QQmlCompiledData::TypeReference::doDynamicTypeCheck()
{
    const QMetaObject *mo = 0;
    if (typePropertyCache)
        mo = typePropertyCache->firstCppMetaObject();
    else if (type)
        mo = type->metaObject();
    else if (component)
        mo = component->rootPropertyCache->firstCppMetaObject();
    isFullyDynamicType = qtTypeInherits<QQmlPropertyMap>(mo);
}

void QQmlCompiledData::initialize(QQmlEngine *engine)
{
    Q_ASSERT(!hasEngine());
    QQmlCleanup::addToEngine(engine);
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
    if (compilationUnit && !compilationUnit->engine)
        compilationUnit->linkToEngine(v4);
}

QT_END_NAMESPACE
