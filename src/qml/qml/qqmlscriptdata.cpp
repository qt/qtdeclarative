// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    : m_loaded(false)
{
}

QQmlRefPointer<QQmlContextData> QQmlScriptData::qmlContextDataForContext(
        const QQmlRefPointer<QQmlContextData> &parentQmlContextData)
{
    Q_ASSERT(parentQmlContextData && parentQmlContextData->engine());

    if (!m_precompiledScript || m_precompiledScript->isESModule())
        return nullptr;

    QQmlRefPointer<QQmlContextData> qmlContextData = m_precompiledScript->isSharedLibrary()
            ? QQmlContextData::createRefCounted(QQmlRefPointer<QQmlContextData>())
            : QQmlContextData::createRefCounted(parentQmlContextData);

    qmlContextData->setInternal(true);
    qmlContextData->setJSContext(true);
    if (m_precompiledScript->isSharedLibrary())
        qmlContextData->setPragmaLibraryContext(true);
    else
        qmlContextData->setPragmaLibraryContext(parentQmlContextData->isPragmaLibraryContext());
    qmlContextData->setBaseUrl(url);
    qmlContextData->setBaseUrlString(urlString);

    // For backward compatibility, if there are no imports, we need to use the
    // imports from the parent context.  See QTBUG-17518.
    if (!typeNameCache->isEmpty()) {
        qmlContextData->setImports(typeNameCache);
    } else if (!m_precompiledScript->isSharedLibrary()) {
        qmlContextData->setImports(parentQmlContextData->imports());
        qmlContextData->setImportedScripts(parentQmlContextData->importedScripts());
    }

    if (m_precompiledScript->isSharedLibrary())
        qmlContextData->setEngine(parentQmlContextData->engine()); // Fix for QTBUG-21620

    QV4::ExecutionEngine *v4 = parentQmlContextData->engine()->handle();
    QV4::Scope scope(v4);
    QV4::ScopedObject scriptsArray(scope);
    if (qmlContextData->importedScripts().isNullOrUndefined()) {
        scriptsArray = v4->newArrayObject(scripts.size());
        qmlContextData->setImportedScripts(
                    QV4::PersistentValue(v4, scriptsArray.asReturnedValue()));
    } else {
        scriptsArray = qmlContextData->importedScripts().valueRef();
    }
    QV4::ScopedValue v(scope);
    for (int ii = 0; ii < scripts.size(); ++ii) {
        v = scripts.at(ii)->scriptData()->scriptValueForContext(qmlContextData);
        scriptsArray->put(ii, v);
    }

    return qmlContextData;
}

QV4::ReturnedValue QQmlScriptData::scriptValueForContext(
        const QQmlRefPointer<QQmlContextData> &parentQmlContextData)
{
    if (m_loaded)
        return m_value.value();

    Q_ASSERT(parentQmlContextData && parentQmlContextData->engine());
    QV4::ExecutionEngine *v4 = parentQmlContextData->engine()->handle();
    QV4::Scope scope(v4);

    QV4::Scoped<QV4::QmlContext> qmlExecutionContext(scope);
    if (auto qmlContextData = qmlContextDataForContext(parentQmlContextData)) {
        qmlExecutionContext = QV4::QmlContext::create(v4->rootContext(), std::move(qmlContextData),
                                                      /* scopeObject: */ nullptr);
    }

    QV4::Scoped<QV4::Module> module(scope, m_precompiledScript->instantiate(v4));
    if (module) {
        if (qmlExecutionContext) {
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
    if (qmlExecutionContext)
        value = qmlExecutionContext->d()->qml();
    else if (module)
        value = module->d();

    if (m_precompiledScript->isSharedLibrary() || m_precompiledScript->isESModule()) {
        m_loaded = true;
        m_value.set(v4, value);
    }

    return value->asReturnedValue();
}

QT_END_NAMESPACE
