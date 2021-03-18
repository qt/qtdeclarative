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

#include <private/qqmlengine_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmlsourcecoordinate_p.h>
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
    if (diskCacheEnabled()) {
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

    if (diskCacheEnabled()) {
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

void QQmlScriptBlob::initializeFromCachedUnit(const QV4::CompiledData::Unit *unit)
{
    initializeFromCompilationUnit(QV4::ExecutableCompilationUnit::create(
            QV4::CompiledData::CompilationUnit(unit, urlString(), finalUrlString())));
}

void QQmlScriptBlob::done()
{
    if (isError())
        return;

    // Check all script dependencies for errors
    for (int ii = 0; ii < m_scripts.count(); ++ii) {
        const ScriptReference &script = m_scripts.at(ii);
        Q_ASSERT(script.script->isCompleteOrError());
        if (script.script->isError()) {
            QList<QQmlError> errors = script.script->errors();
            QQmlError error;
            error.setUrl(url());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(script.location.line));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(script.location.column));
            error.setDescription(QQmlTypeLoader::tr("Script %1 unavailable").arg(script.script->urlString()));
            errors.prepend(error);
            setError(errors);
            return;
        }
    }

    if (!m_isModule) {
        m_scriptData->typeNameCache.adopt(new QQmlTypeNameCache(m_importCache));

        QSet<QString> ns;

        for (int scriptIndex = 0; scriptIndex < m_scripts.count(); ++scriptIndex) {
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

        m_importCache.populateCache(m_scriptData->typeNameCache.data());
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

    m_importCache.setBaseUrl(finalUrl(), finalUrlString());

    QQmlRefPointer<QV4::ExecutableCompilationUnit> script = m_scriptData->m_precompiledScript;

    if (!m_isModule) {
        QList<QQmlError> errors;
        for (quint32 i = 0, count = script->importCount(); i < count; ++i) {
            const QV4::CompiledData::Import *import = script->importAt(i);
            if (!addImport(import, &errors)) {
                Q_ASSERT(errors.size());
                QQmlError error(errors.takeFirst());
                error.setUrl(m_importCache.baseUrl());
                error.setLine(import->location.line);
                error.setColumn(import->location.column);
                errors.prepend(error); // put it back on the list after filling out information.
                setError(errors);
                return;
            }
        }
    }

    auto *v4 = QQmlEnginePrivate::getV4Engine(typeLoader()->engine());

    v4->injectModule(unit);

    for (const QString &request: unit->moduleRequests()) {
        if (v4->moduleForUrl(QUrl(request), unit.data()))
            continue;

        const QUrl absoluteRequest = unit->finalUrl().resolved(QUrl(request));
        QQmlRefPointer<QQmlScriptBlob> blob = typeLoader()->getScript(absoluteRequest);
        addDependency(blob.data());
        scriptImported(blob, /* ### */QV4::CompiledData::Location(), /*qualifier*/QString(), /*namespace*/QString());
    }
}

QT_END_NAMESPACE
