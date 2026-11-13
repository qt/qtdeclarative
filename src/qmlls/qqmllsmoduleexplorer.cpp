// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmllsmoduleexplorer_p.h"

#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
\internal
\class QQmlLSModuleExplorer
\brief Searches import paths for QML modules on a background thread.

Import paths passed to explore() are scanned for qmldir files one after the other. Modules are
published as soon as they are found, so modulesIn() always returns the latest partial or complete
result without waiting for the search to finish. Import paths that were already explored or are
queued are not scanned again.

All functions can be called from any thread.
*/

static std::optional<QString> moduleNameFromQmldir(const QString &qmldirPath)
{
    static constexpr QLatin1String moduleKey = "module"_L1;
    QFile qmldir(qmldirPath);
    if (!qmldir.open(QIODevice::ReadOnly | QIODevice::Text))
        return std::nullopt;
    // Qt 6.12.0 ships with ~125 QML modules it seems, so we end up parsing at least
    // that many qmldirs here. Most of them will never be imported and their QQmldirParser never
    // be re-used, so do a manual grep instead of parsing via QQmldirParser.
    while (!qmldir.atEnd()) {
        const QByteArray lineData = qmldir.readLine().trimmed();
        if (lineData.startsWith(moduleKey))
            return QString::fromUtf8(lineData.mid(moduleKey.size())).trimmed();
    }
    return std::nullopt;
}

QQmlLSModuleExplorer::QQmlLSModuleExplorer()
{
}

QQmlLSModuleExplorer::~QQmlLSModuleExplorer()
{
    {
        QMutexLocker guard(&m_mutex);
        m_stopRequested = true;
    }
    m_hasWork.wakeAll();
    if (m_thread)
        m_thread->wait();
}

void QQmlLSModuleExplorer::setEnabled(bool enabled)
{
    QMutexLocker guard(&m_mutex);
    m_enabled = enabled;
}

void QQmlLSModuleExplorer::explore(const QStringList &importPaths)
{
    bool startedExploring = false;
    {
        QMutexLocker guard(&m_mutex);
        if (!m_enabled)
            return;
        bool queuedAny = false;
        for (const QString &importPath : importPaths) {
            if (m_modulesPerImportPath.contains(importPath))
                continue;
            m_modulesPerImportPath.insert(importPath, {});
            m_pendingImportPaths.push_back(importPath);
            queuedAny = true;
        }
        if (!queuedAny)
            return;

        startedExploring = !std::exchange(m_exploring, true);
        if (!m_thread) {
            m_thread.reset(QThread::create([this] { run(); }));
            m_thread->setObjectName(u"QQmlLSModuleExplorer"_s);
            m_thread->start();
        }
    }
    if (startedExploring)
        emit explorationStarted();
    m_hasWork.wakeAll();
}

QStringList QQmlLSModuleExplorer::modulesIn(const QStringList &importPaths) const
{
    QStringList modules;
    {
        QMutexLocker guard(&m_mutex);
        for (const QString &importPath : importPaths)
            modules.append(m_modulesPerImportPath.value(importPath));
    }
    modules.removeDuplicates();
    return modules;
}

bool QQmlLSModuleExplorer::isExploring() const
{
    QMutexLocker guard(&m_mutex);
    return m_exploring;
}

std::optional<QString> QQmlLSModuleExplorer::takeNextImportPath()
{
    QMutexLocker guard(&m_mutex);
    while (m_pendingImportPaths.empty() && !m_stopRequested)
        m_hasWork.wait(&m_mutex);
    if (m_stopRequested)
        return std::nullopt;
    QString importPath = std::move(m_pendingImportPaths.front());
    m_pendingImportPaths.pop_front();
    return importPath;
}

void QQmlLSModuleExplorer::addModule(const QString &importPath, const QString &module)
{
    QMutexLocker guard(&m_mutex);
    m_modulesPerImportPath[importPath].append(module);
}

bool QQmlLSModuleExplorer::finishExploring()
{
    QMutexLocker guard(&m_mutex);
    if (!m_pendingImportPaths.empty())
        return false;
    m_exploring = false;
    return true;
}

void QQmlLSModuleExplorer::run()
{
    while (const std::optional<QString> importPath = takeNextImportPath()) {
        QDirIterator it(*importPath, { u"qmldir"_s }, QDir::Files,
                        QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (it.hasNext()) {
            {
                QMutexLocker guard(&m_mutex);
                if (m_stopRequested)
                    return;
            }
            if (const auto module = moduleNameFromQmldir(it.next()))
                addModule(*importPath, *module);
        }
        if (finishExploring())
            emit explorationFinished();
    }
}

QT_END_NAMESPACE
