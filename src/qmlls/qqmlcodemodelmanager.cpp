// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:trusted-sources

#include "qqmlcodemodelmanager_p.h"
#include "qqmllsplugin_p.h"

#include <QtQmlCompiler/private/qqmljsutils_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QmlLsp {

using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

void QQmlCodeModelManager::onCMakeProberFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_cmakeStatus == DoesNotHaveCMake)
        return;

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        disableCMakeCalls();
        return;
    }
    m_cmakeStatus = HasCMake;
    for (const auto &ws : m_workspaces)
        ws.codeModel->tryEnableCMakeCalls(&m_processScheduler);
}

/*!
\internal
Enable and initialize the functionality that uses CMake, if CMake exists.

\note Set the buildpaths before calling this method!
*/
void QQmlCodeModelManager::tryEnableCMakeCalls()
{
    m_cmakeStatus = IsProbingCMake;

    m_cmakeProber.setProgram(u"cmake"_s);
    m_cmakeProber.setArguments({ u"--version"_s });
    QObject::connect(&m_cmakeProber, &QProcess::finished, this,
                     &QQmlCodeModelManager::onCMakeProberFinished);
    QObject::connect(&m_cmakeProber, &QProcess::errorOccurred, this,
                     &QQmlCodeModelManager::disableCMakeCalls);

    m_cmakeProber.start();
}

QQmlCodeModelManager::QQmlCodeModelManager(QObject *parent, QQmlToolingSharedSettings *settings)
    : QObject{ parent }, m_settings(settings), m_pluginLoader(QmlLSPluginInterface_iid, u"/qmlls"_s)
{
    const QByteArray defaultCodeModel;
    appendWorkspace(defaultCodeModel, ManagedByServer);
    connect(&m_processScheduler, &QProcessScheduler::done, this,
            &QQmlCodeModelManager::onBuildFinished);
}

void QQmlCodeModelManager::onBuildFinished(const QByteArray &url)
{
    auto it = findWorkspace(url);
    // note: fallback workspaces have no build folder information
    if (it == fallbackWorkspace() || it == m_workspaces.end())
        return;

    // refresh information obtained from build folder, like .qmlls.build.ini or resource files
    const QStringList buildPaths = it->codeModel->buildPaths();
    m_buildInformation.loadSettingsFrom(buildPaths, ForceUpdate);
    setBuildPathsOn(&*it, buildPaths, AppendPathsFromFallback);
    emit configurationChanged();
}

void QQmlCodeModelManager::prepareForShutdown()
{
    for (auto it = m_workspaces.begin(), end = m_workspaces.end(); it != end; ++it)
        it->codeModel->prepareForShutdown();
}

QQmlCodeModelManager::~QQmlCodeModelManager()
{
    m_cmakeProber.kill();
    m_cmakeProber.waitForFinished();
    prepareForShutdown();
}

QQmlCodeModelManager::WorkspaceIterator
QQmlCodeModelManager::findWorkspaceForFile(const QByteArray &url)
{
    Q_ASSERT(!m_workspaces.empty());
    // if file was already opened before: re-use same CodeModel as last time
    if (auto it = m_file2CodeModel.find(url); it != m_file2CodeModel.end()) {
        const auto result = findWorkspace(it->second);
        Q_ASSERT(result != m_workspaces.end());
        return result;
    }

    long longestRootUrl = 0;
    WorkspaceIterator result = m_workspaces.begin();
    for (auto it = m_workspaces.begin(), end = m_workspaces.end(); it != end; ++it) {
        if (it->toBeClosed)
            continue;
        const QByteArray rootUrl = it->url;
        if (!url.startsWith(rootUrl))
            continue;

        if (rootUrl.size() == url.size())
            return it;

        const long rootUrlLength = rootUrl.length();
        if (rootUrlLength > longestRootUrl) {
            longestRootUrl = rootUrlLength;
            result = it;
        }
    }

    // check .qmlls.build.ini for a potentially better match
    if (const ModuleSetting moduleSetting =
                m_buildInformation.settingFor(QUrl::fromEncoded(url).toLocalFile());
        !moduleSetting.importPaths.isEmpty()) {
        const QByteArray rootUrl = QUrl::fromLocalFile(moduleSetting.sourceFolder).toEncoded();
        if (longestRootUrl < rootUrl.size()) {
            appendWorkspace(rootUrl, ManagedByServer);
            return --m_workspaces.end();
        }
    }
    Q_ASSERT(result != m_workspaces.end());
    return result;
}

QQmlCodeModel *QQmlCodeModelManager::findCodeModelForFile(const QByteArray &url)
{
    return findWorkspaceForFile(url)->codeModel.get();
}

QQmlCodeModelManager::WorkspaceMutableIterator
QQmlCodeModelManager::findWorkspace(const QByteArray &url)
{
    return std::find_if(m_workspaces.begin(), m_workspaces.end(),
                        [&url](const QQmlWorkspace &ws) { return ws.url == url; });
}

void QQmlCodeModelManager::setBuildPathsOn(const QQmlWorkspace *ws, const QStringList &buildFolder,
                                           SetBuildPathOption option)
{
    ws->codeModel->setBuildPaths(
            buildFolder
            + (option == DontAppendPathsFromFallback ? QStringList{} : defaultBuildPaths()));

    const QString file = QUrl::fromEncoded(ws->url).toLocalFile();
    ws->codeModel->setImportPaths(
            m_buildInformation.importPathsFor(file)
            + (option == DontAppendPathsFromFallback ? QStringList{} : defaultImportPaths()));

    if (const QStringList resourceFiles = m_buildInformation.resourceFilesFor(file);
            !resourceFiles.isEmpty()) {
        ws->codeModel->setResourceFiles(resourceFiles);
        return;
    }
    // fallback for qt projects < 6.11 without resource files in their .qmlls.build.ini
    const QStringList resourceFiles =
            QQmlJSUtils::resourceFilesFromBuildFolders(ws->codeModel->buildPaths());
    ws->codeModel->setResourceFiles(
            resourceFiles
            + (option == DontAppendPathsFromFallback ? QStringList{} : defaultResourceFiles()));
}

void QQmlCodeModelManager::appendWorkspace(const QByteArray &url, ManagedBy managedBy)
{
    QQmlWorkspace ws;
    ws.url = url;
    ws.codeModel = std::make_unique<QQmlCodeModel>(url, this, m_settings);

    // the non-fallback codemodel inherits the default values from the fallback codemodel
    if (!url.isEmpty()) {
        ws.codeModel->setCMakeJobs(defaultCMakeJobs());
        ws.codeModel->setDocumentationRootPath(defaultDocumentationRootPath());

        setBuildPathsOn(&ws, {}, AppendPathsFromFallback);
    }

    QObject::connect(ws.codeModel.get(), &QQmlCodeModel::updatedSnapshot, this,
                     &QQmlCodeModelManager::updatedSnapshot);
    ws.managedByClient = managedBy == ManagedByClient;

    switch (m_cmakeStatus) {
    case DoesNotHaveCMake:
        ws.codeModel->disableCMakeCalls();
        break;
    case HasCMake:
        ws.codeModel->tryEnableCMakeCalls(&m_processScheduler);
        break;
    case IsProbingCMake:
        // will be enabled once the CMake probing process finishes
        break;
    }
    m_workspaces.emplace_back(std::move(ws));
}

QQmlCodeModelManager::WorkspaceIterator
QQmlCodeModelManager::workspaceFromBuildFolder(const QString &fileName,
                                               const QStringList &buildFolders)
{
    m_buildInformation.loadSettingsFrom(buildFolders);
    const ModuleSetting setting = m_buildInformation.settingFor(fileName);
    QByteArray url = QUrl::fromLocalFile(setting.sourceFolder).toEncoded();
    if (auto it = findWorkspace(url); it != m_workspaces.end())
        return it;
    appendWorkspace(url, ManagedByServer);
    return --m_workspaces.end();
}

void QQmlCodeModelManager::disableCMakeCalls()
{
    m_cmakeStatus = DoesNotHaveCMake;
    for (const auto &ws : m_workspaces)
        ws.codeModel->disableCMakeCalls();
}

OpenDocumentSnapshot QQmlCodeModelManager::snapshotByUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->snapshotByUrl(url);
}

void QQmlCodeModelManager::removeDirectory(const QByteArray &url)
{
    findCodeModelForFile(url)->removeDirectory(url);
}

void QQmlCodeModelManager::newOpenFile(const QByteArray &url, int version, const QString &docText)
{
    const auto ws = findWorkspaceForFile(url);
    m_file2CodeModel[url] = ws->url;
    ws->codeModel->newOpenFile(url, version, docText);
}

OpenDocument QQmlCodeModelManager::openDocumentByUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->openDocumentByUrl(url);
}

RegisteredSemanticTokens &QQmlCodeModelManager::registeredTokens(const QByteArray &url)
{
    return findCodeModelForFile(url)->registeredTokens();
}

void QQmlCodeModelManager::closeOpenFile(const QByteArray &url)
{
    m_file2CodeModel.erase(url);
    const auto it = findWorkspaceForFile(url);
    it->codeModel->closeOpenFile(url);

    // don't close the default workspace
    if (it->url.isEmpty())
        return;

    // close empty WS when managed by server or when client marked ws as toBeClosed.
    if ((it->managedByClient && it->toBeClosed) || !it->managedByClient) {
        if (it->codeModel->isEmpty())
            m_workspaces.erase(it);
    }
}

QList<QByteArray> QQmlCodeModelManager::rootUrls() const
{
    QList<QByteArray> result;
    result.reserve(m_workspaces.size());
    for (const QQmlWorkspace &ws : m_workspaces) {
        result << ws.url;
    }
    return result;
}

void QQmlCodeModelManager::addRootUrls(const QList<QByteArray> &urls)
{
    for (const QByteArray &url : urls) {
        if (const auto it = findWorkspace(url); it != m_workspaces.end()) {
            it->toBeClosed = false;
            continue;
        }

        appendWorkspace(url, ManagedByClient);
    }
}

void QQmlCodeModelManager::removeRootUrls(const QList<QByteArray> &urls)
{
    for (const QByteArray &url : urls) {
        if (auto it = findWorkspace(url); it != m_workspaces.end() && it->managedByClient)
            it->toBeClosed = true;
    }
}

QStringList QQmlCodeModelManager::importPathsForUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->importPathsForUrl(url);
}

QStringList QQmlCodeModelManager::buildPathsForFileUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->buildPathsForFileUrl(url);
}

QStringList QQmlCodeModelManager::resourceFilesForFileUrl(const QByteArray &url)
{
    if (const QStringList result =
                m_buildInformation.resourceFilesFor(QUrl::fromEncoded(url).toLocalFile());
        !result.isEmpty()) {
        return result;
    }

    // fallback, for standalone qmlls on projects targeting qt < 6.11
    return QQmlJSUtils::resourceFilesFromBuildFolders(buildPathsForFileUrl(url));
}

QByteArray QQmlCodeModelManager::shortestRootUrlForFile(const QByteArray &fileUrl) const
{
    // fallback value for candidate is the empty url of the default workspace
    QByteArray candidate;

    // ignore the default workspace which is at the front of m_workspaces
    Q_ASSERT(m_workspaces.size() > 0);
    Q_ASSERT(m_workspaces.front().url.isEmpty());
    auto it = std::find_if(
            ++m_workspaces.cbegin(), m_workspaces.cend(),
            [&fileUrl](const QQmlWorkspace &ws) { return fileUrl.startsWith(ws.url); });

    if (it != m_workspaces.cend())
        candidate = it->url;

    for (; it != m_workspaces.cend(); ++it) {
        if (it->url.length() < candidate.length() && fileUrl.startsWith(it->url))
            candidate = it->url;
    }
    return candidate;
}

void QQmlCodeModelManager::setDocumentationRootPath(const QString &path)
{
    // Note: this function can only be called after the fallback workspace was created but before
    // all other potential workspaces are created, for example when setting the import paths set via
    // commandline option or environment variable.
    Q_ASSERT(m_workspaces.size() == 1);
    for (const auto &ws : m_workspaces)
        ws.codeModel->setDocumentationRootPath(path);
}

void QQmlCodeModelManager::setVerbose(bool verbose)
{
    m_verbose = verbose;
    for (const auto &ws : m_workspaces)
        ws.codeModel->setVerbose(verbose);
}

void QQmlCodeModelManager::setCMakeJobs(int jobs)
{
    for (const auto &ws : m_workspaces)
        ws.codeModel->setCMakeJobs(jobs);
}

void QQmlCodeModelManager::setBuildPathsForRootUrl(const QByteArray &url, const QStringList &paths)
{
    m_buildInformation.loadSettingsFrom(paths, ForceUpdate);

    // build paths passed by -b have an empty url and apply to all workspaces
    if (url.isEmpty()) {
        setBuildPathsOn(&*fallbackWorkspace(), paths, DontAppendPathsFromFallback);
        // make non-fallback workspaces get the fallback build path
        for (auto it = beginNonFallbackWorkspace(), end = endNonFallbackWorkspace(); it != end;
             ++it) {
            setBuildPathsOn(&*it, {}, AppendPathsFromFallback);
        }
        emit configurationChanged();
        return;
    }

    auto ws = findWorkspaceForFile(url);
    setBuildPathsOn(&*ws, paths, AppendPathsFromFallback);
    emit configurationChanged();
}

void QQmlCodeModelManager::addOpenToUpdate(const QByteArray &url)
{
    findCodeModelForFile(url)->addOpenToUpdate(url, NormalUpdate);
}

void QQmlCodeModelManager::setImportPaths(const QStringList &paths)
{
    // Note: this function can only be called after the fallback workspace was created but before
    // all other potential workspaces are created, for example when setting the import paths set via
    // commandline option or environment variable.
    Q_ASSERT(m_workspaces.size() == 1);
    for (const auto &ws : m_workspaces)
        ws.codeModel->setImportPaths(paths);
}

HelpManager *QQmlCodeModelManager::helpManagerForUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->helpManager();
}

} // namespace QmlLsp

QT_END_NAMESPACE
