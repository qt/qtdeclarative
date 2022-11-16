// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmlengine_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qv4runtimecodegen_p.h>
#include <private/qv4script_p.h>

#include <QtCore/qloggingcategory.h>

Q_DECLARE_LOGGING_CATEGORY(DBG_DISK_CACHE)
Q_LOGGING_CATEGORY(DBG_DISK_CACHE, "qt.qml.diskcache")

QT_BEGIN_NAMESPACE

QQmlScriptBlob::QQmlScriptBlob(const QUrl &url, QQmlTypeLoader *loader)
    : QQmlTypeLoader::Blob(url, JavaScriptFile, loader)
      , m_isModule(url.path().endsWith(QLatin1String(".mjs")))
{
}

QQmlScriptBlob::~QQmlScriptBlob()
{
}

QQmlRefPointer<QQmlScriptData> QQmlScriptBlob::scriptData() const
{
    return m_scriptData;
}

void QQmlScriptBlob::dataReceived(const SourceCodeData &data)
{
    if (readCacheFile()) {
        QQmlRefPointer<QV4::ExecutableCompilationUnit> unit
                = QV4::ExecutableCompilationUnit::create();
        QString error;
        if (unit->loadFromDisk(url(), data.sourceTimeStamp(), &error)) {
            initializeFromCompilationUnit(unit);
            return;
        } else {
            qCDebug(DBG_DISK_CACHE()) << "Error loading" << urlString() << "from disk cache:" << error;
        }
    }

    if (!data.exists()) {
        if (m_cachedUnitStatus == QQmlMetaType::CachedUnitLookupError::VersionMismatch)
            setError(QQmlTypeLoader::tr("File was compiled ahead of time with an incompatible version of Qt and the original file cannot be found. Please recompile"));
        else
            setError(QQmlTypeLoader::tr("No such file or directory"));
        return;
    }

    QString error;
    QString source = data.readAll(&error);
    if (!error.isEmpty()) {
        setError(error);
        return;
    }

    QV4::CompiledData::CompilationUnit unit;

    if (m_isModule) {
        QList<QQmlJS::DiagnosticMessage> diagnostics;
        unit = QV4::Compiler::Codegen::compileModule(isDebugging(), urlString(), source,
                                                     data.sourceTimeStamp(), &diagnostics);
        QList<QQmlError> errors = QQmlEnginePrivate::qmlErrorFromDiagnostics(urlString(), diagnostics);
        if (!errors.isEmpty()) {
            setError(errors);
            return;
        }
    } else {
        QmlIR::Document irUnit(isDebugging());

        irUnit.jsModule.sourceTimeStamp = data.sourceTimeStamp();

        QmlIR::ScriptDirectivesCollector collector(&irUnit);
        irUnit.jsParserEngine.setDirectives(&collector);

        QList<QQmlError> errors;
        irUnit.javaScriptCompilationUnit = QV4::Script::precompile(
                     &irUnit.jsModule, &irUnit.jsParserEngine, &irUnit.jsGenerator, urlString(), finalUrlString(),
                     source, &errors, QV4::Compiler::ContextType::ScriptImportedByQML);

        source.clear();
        if (!errors.isEmpty()) {
            setError(errors);
            return;
        }

        QmlIR::QmlUnitGenerator qmlGenerator;
        qmlGenerator.generate(irUnit);
        unit = std::move(irUnit.javaScriptCompilationUnit);
    }

    auto executableUnit = QV4::ExecutableCompilationUnit::create(std::move(unit));

    if (writeCacheFile()) {
        QString errorString;
        if (executableUnit->saveToDisk(url(), &errorString)) {
            QString error;
            if (!executableUnit->loadFromDisk(url(), data.sourceTimeStamp(), &error)) {
                // ignore error, keep using the in-memory compilation unit.
            }
        } else {
            qCDebug(DBG_DISK_CACHE()) << "Error saving cached version of"
                                      << executableUnit->fileName() << "to disk:" << errorString;
        }
    }

    initializeFromCompilationUnit(executableUnit);
}

void QQmlScriptBlob::initializeFromCachedUnit(const QQmlPrivate::CachedQmlUnit *unit)
{
    initializeFromCompilationUnit(QV4::ExecutableCompilationUnit::create(
            QV4::CompiledData::CompilationUnit(unit->qmlData, unit->aotCompiledFunctions, urlString(), finalUrlString())));
}

void QQmlScriptBlob::done()
{
    if (isError())
        return;

    // Check all script dependencies for errors
    for (int ii = 0; ii < m_scripts.size(); ++ii) {
        const ScriptReference &script = m_scripts.at(ii);
        Q_ASSERT(script.script->isCompleteOrError());
        if (script.script->isError()) {
            QList<QQmlError> errors = script.script->errors();
            QQmlError error;
            error.setUrl(url());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(script.location.line()));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(script.location.column()));
            error.setDescription(QQmlTypeLoader::tr("Script %1 unavailable").arg(script.script->urlString()));
            errors.prepend(error);
            setError(errors);
            return;
        }
    }

    if (!m_isModule) {
        m_scriptData->typeNameCache.adopt(new QQmlTypeNameCache(m_importCache));

        QSet<QString> ns;

        for (int scriptIndex = 0; scriptIndex < m_scripts.size(); ++scriptIndex) {
            const ScriptReference &script = m_scripts.at(scriptIndex);

            m_scriptData->scripts.append(script.script);

            if (!script.nameSpace.isNull()) {
                if (!ns.contains(script.nameSpace)) {
                    ns.insert(script.nameSpace);
                    m_scriptData->typeNameCache->add(script.nameSpace);
                }
            }
            m_scriptData->typeNameCache->add(script.qualifier, scriptIndex, script.nameSpace);
        }

        m_importCache->populateCache(m_scriptData->typeNameCache.data());
    }
    m_scripts.clear();
}

QString QQmlScriptBlob::stringAt(int index) const
{
    return m_scriptData->m_precompiledScript->stringAt(index);
}

void QQmlScriptBlob::scriptImported(const QQmlRefPointer<QQmlScriptBlob> &blob, const QV4::CompiledData::Location &location, const QString &qualifier, const QString &nameSpace)
{
    ScriptReference ref;
    ref.script = blob;
    ref.location = location;
    ref.qualifier = qualifier;
    ref.nameSpace = nameSpace;

    m_scripts << ref;
}

void QQmlScriptBlob::initializeFromCompilationUnit(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit)
{
    Q_ASSERT(!m_scriptData);
    m_scriptData.adopt(new QQmlScriptData());
    m_scriptData->url = finalUrl();
    m_scriptData->urlString = finalUrlString();
    m_scriptData->m_precompiledScript = unit;

    m_importCache->setBaseUrl(finalUrl(), finalUrlString());

    QQmlRefPointer<QV4::ExecutableCompilationUnit> script = m_scriptData->m_precompiledScript;

    if (!m_isModule) {
        QList<QQmlError> errors;
        for (quint32 i = 0, count = script->importCount(); i < count; ++i) {
            const QV4::CompiledData::Import *import = script->importAt(i);
            if (!addImport(import, {}, &errors)) {
                Q_ASSERT(errors.size());
                QQmlError error(errors.takeFirst());
                error.setUrl(m_importCache->baseUrl());
                error.setLine(import->location.line());
                error.setColumn(import->location.column());
                errors.prepend(error); // put it back on the list after filling out information.
                setError(errors);
                return;
            }
        }
    }

    auto *v4 = QQmlEnginePrivate::getV4Engine(typeLoader()->engine());

    v4->injectCompiledModule(unit);

    for (const QString &request: unit->moduleRequests()) {
        const auto module = v4->moduleForUrl(QUrl(request), unit.data());
        if (module.compiled || module.native)
            continue;

        const QUrl absoluteRequest = unit->finalUrl().resolved(QUrl(request));
        QQmlRefPointer<QQmlScriptBlob> blob = typeLoader()->getScript(absoluteRequest);
        addDependency(blob.data());
        scriptImported(blob, /* ### */QV4::CompiledData::Location(), /*qualifier*/QString(), /*namespace*/QString());
    }
}

/*!
    \internal

    This initializes a dummy script blob from a "native" ECMAScript module.
    Native modules are just JavaScript values, possibly objects with members.

    \sa QJSEngine::registerModule()
 */
void QQmlScriptBlob::initializeFromNative(const QV4::Value &value)
{
    Q_ASSERT(!m_scriptData);
    m_scriptData.adopt(new QQmlScriptData());
    m_scriptData->url = finalUrl();
    m_scriptData->urlString = finalUrlString();
    m_scriptData->m_loaded = true;
    m_scriptData->m_value.set(QQmlEnginePrivate::getV4Engine(typeLoader()->engine()), value);
    m_importCache->setBaseUrl(finalUrl(), finalUrlString());
}

QT_END_NAMESPACE
