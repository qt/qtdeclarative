/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <private/qqmlscriptdata_p.h>
#include <private/qqmlcontext_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4object_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qv4module_p.h>

QT_BEGIN_NAMESPACE

QQmlScriptData::QQmlScriptData()
    : typeNameCache(nullptr)
      , m_loaded(false)
{
}

QQmlContextData *QQmlScriptData::qmlContextDataForContext(QQmlContextData *parentQmlContextData)
{
    Q_ASSERT(parentQmlContextData && parentQmlContextData->engine);

    if (m_precompiledScript->isESModule())
        return nullptr;

    auto qmlContextData = new QQmlContextData();

    qmlContextData->isInternal = true;
    qmlContextData->isJSContext = true;
    if (m_precompiledScript->isSharedLibrary())
        qmlContextData->isPragmaLibraryContext = true;
    else
        qmlContextData->isPragmaLibraryContext = parentQmlContextData->isPragmaLibraryContext;
    qmlContextData->baseUrl = url;
    qmlContextData->baseUrlString = urlString;

    // For backward compatibility, if there are no imports, we need to use the
    // imports from the parent context.  See QTBUG-17518.
    if (!typeNameCache->isEmpty()) {
        qmlContextData->imports = typeNameCache;
    } else if (!m_precompiledScript->isSharedLibrary()) {
        qmlContextData->imports = parentQmlContextData->imports;
        qmlContextData->importedScripts = parentQmlContextData->importedScripts;
    }

    if (!m_precompiledScript->isSharedLibrary()) {
        qmlContextData->setParent(parentQmlContextData);
    } else {
        qmlContextData->engine = parentQmlContextData->engine; // Fix for QTBUG-21620
    }

    QV4::ExecutionEngine *v4 = parentQmlContextData->engine->handle();
    QV4::Scope scope(v4);
    QV4::ScopedObject scriptsArray(scope);
    if (qmlContextData->importedScripts.isNullOrUndefined()) {
        scriptsArray = v4->newArrayObject(scripts.count());
        qmlContextData->importedScripts.set(v4, scriptsArray);
    } else {
        scriptsArray = qmlContextData->importedScripts.valueRef();
    }
    QV4::ScopedValue v(scope);
    for (int ii = 0; ii < scripts.count(); ++ii)
        scriptsArray->put(ii, (v = scripts.at(ii)->scriptData()->scriptValueForContext(qmlContextData)));

    return qmlContextData;
}

QV4::ReturnedValue QQmlScriptData::scriptValueForContext(QQmlContextData *parentQmlContextData)
{
    if (m_loaded)
        return m_value.value();

    Q_ASSERT(parentQmlContextData && parentQmlContextData->engine);
    QV4::ExecutionEngine *v4 = parentQmlContextData->engine->handle();
    QV4::Scope scope(v4);

    if (!hasEngine()) {
        addToEngine(parentQmlContextData->engine);
        addref();
    }

    QQmlContextDataRef qmlContextData = qmlContextDataForContext(parentQmlContextData);
    QV4::Scoped<QV4::QmlContext> qmlExecutionContext(scope);
    if (qmlContextData)
        qmlExecutionContext =
                QV4::QmlContext::create(v4->rootContext(), qmlContextData, /* scopeObject: */ nullptr);

    QV4::Scoped<QV4::Module> module(scope, m_precompiledScript->instantiate(v4));
    if (module) {
        if (qmlContextData) {
            module->d()->scope->outer.set(v4, qmlExecutionContext->d());
            qmlExecutionContext->d()->qml()->module.set(v4, module->d());
        }

        module->evaluate();
    }

    if (v4->hasException) {
        QQmlError error = v4->catchExceptionAsQmlError();
        if (error.isValid())
            QQmlEnginePrivate::get(v4)->warning(error);
    }

    QV4::ScopedValue value(scope);
    if (qmlContextData)
        value = qmlExecutionContext->d()->qml();
    else if (module)
        value = module->d();

    if (m_precompiledScript->isSharedLibrary() || m_precompiledScript->isESModule()) {
        m_loaded = true;
        m_value.set(v4, value);
    }

    return value->asReturnedValue();
}

void QQmlScriptData::clear()
{
    typeNameCache = nullptr;
    scripts.clear();

    // An addref() was made when the QQmlCleanup was added to the engine.
    release();
}

QT_END_NAMESPACE
