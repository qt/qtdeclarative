// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:trusted-sources

#include "qqmlcodemodel_p.h"
#include "qqmllsplugin_p.h"
#include "qtextdocument_p.h"
#include "qqmllsutils_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qprocess.h>
#include <QtCore/qdiriterator.h>

#if QT_CONFIG(settings)
#  include <QtCore/qsettings.h>
#endif

#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlCompiler/private/qqmljsutils_p.h>

#include <memory>
#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QmlLsp {

Q_STATIC_LOGGING_CATEGORY(codeModelLog, "qt.languageserver.codemodel")

using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

/*!
\internal
\class QQmlCodeModel

The code model offers a view of the current state of the current files, and traks open files.
All methods are threadsafe, and generally return immutable or threadsafe objects that can be
worked on from any thread (unless otherwise noted).
The idea is the let all other operations be as lock free as possible, concentrating all tricky
synchronization here.

\section2 Global views
\list
\li currentEnv() offers a view that contains the latest version of all the loaded files
\li validEnv() is just like current env but stores only the valid (meaning correctly parsed,
  not necessarily without errors) version of a file, it is normally a better choice to load the
  dependencies/symbol information from
\endlist

\section2 OpenFiles
\list
\li snapshotByUrl() returns an OpenDocumentSnapshot of an open document. From it you can get the
  document, its latest valid version, scope, all connected to a specific version of the document
  and immutable. The signal updatedSnapshot() is called every time a snapshot changes (also for
  every partial change: document change, validDocument change, scope change).
\li openDocumentByUrl() is a lower level and more intrusive access to OpenDocument objects. These
  contains the current snapshot, and shared pointer to a Utils::TextDocument. This is *always* the
  current version of the document, and has line by line support.
  Working on it is more delicate and intrusive, because you have to explicitly acquire its mutex()
  before *any* read or write/modification to it.
  It has a version nuber which is supposed to always change and increase.
  It is mainly used for highlighting/indenting, and is immediately updated when the user edits a
  document. Its use should be avoided if possible, preferring the snapshots.
\endlist

\section2 Parallelism/Theading
Most operations are not parallel and usually take place in the main thread (but are still thread
safe).
There is one task that is executed in parallel: OpenDocumentUpdate.
OpenDocumentUpdate keeps the snapshots of the open documents up to date.

There is always a tension between being responsive, using all threads available, and avoid to hog
too many resources. One can choose different parallelization strategies, we went with a flexiable
approach.
We have (private) functions that execute part of the work: openUpdateSome(). These
do all locking needed, get some work, do it without locks, and at the end update the state of the
code model. If there is more work, then they return true. Thus while (xxxSome()); works until there
is no work left.

The internal addOpenToUpdate() add more work to do.

openNeedUpdate() checks if there is work to do, and if yes ensure that a
worker thread (or more) that work on it exist.
*/

QQmlCodeModel::QQmlCodeModel(const QByteArray &rootUrl, QObject *parent,
                             QQmlToolingSharedSettings *settings)
    : QObject{ parent },
      m_rootUrl(rootUrl),
      m_currentEnv(std::make_shared<DomEnvironment>(
              QLibraryInfo::paths(QLibraryInfo::QmlImportsPath),
              DomEnvironment::Option::SingleThreaded, DomCreationOption::Extended)),
      m_validEnv(std::make_shared<DomEnvironment>(
              m_currentEnv.ownerAs<DomEnvironment>()->loadPaths(),
              DomEnvironment::Option::SingleThreaded, DomCreationOption::Extended)),
      m_settings(settings)
{
}

/*!
\internal
Enable and initialize the functionality that uses CMake, if CMake exists. Runs the build once.
*/
void QQmlCodeModel::tryEnableCMakeCalls(QProcessScheduler *scheduler)
{
    Q_ASSERT(scheduler);
    if (cmakeStatus() == DoesNotHaveCMake)
        return;

    if (m_settings) {
        const QString cmakeCalls = u"no-cmake-calls"_s;
        m_settings->search(url2Path(m_rootUrl), { QString(), verbose() });
        if (m_settings->isSet(cmakeCalls) && m_settings->value(cmakeCalls).toBool()) {
            disableCMakeCalls();
            return;
        }
    }

    QObject::connect(&m_cppFileWatcher, &QFileSystemWatcher::fileChanged, scheduler,
                     [this, scheduler] { callCMakeBuild(scheduler); });
    setCMakeStatus(HasCMake);
    callCMakeBuild(scheduler);
}

/*!
\internal
Disable the functionality that uses CMake, and remove the already watched paths if there are some.
*/
void QQmlCodeModel::disableCMakeCalls()
{
    QMutexLocker guard(&m_mutex);
    m_cmakeStatus = DoesNotHaveCMake;
    if (const QStringList toRemove = m_cppFileWatcher.files(); !toRemove.isEmpty())
        m_cppFileWatcher.removePaths(toRemove);
    QObject::disconnect(&m_cppFileWatcher, &QFileSystemWatcher::fileChanged, nullptr, nullptr);
}

void QQmlCodeModel::prepareForShutdown()
{
    while (true) {
        bool shouldWait;
        {
            QMutexLocker l(&m_mutex);
            m_openDocumentsToUpdate.clear();
            m_state = State::Stopping;
            shouldWait = m_nUpdateInProgress != 0;
        }
        if (!shouldWait)
            break;
        QThread::yieldCurrentThread();
    }
}

QQmlCodeModel::~QQmlCodeModel()
{
    QObject::disconnect(&m_cppFileWatcher, &QFileSystemWatcher::fileChanged, nullptr, nullptr);
    prepareForShutdown();
}

OpenDocumentSnapshot QQmlCodeModel::snapshotByUrl(const QByteArray &url)
{
    return openDocumentByUrl(url).snapshot;
}

void QQmlCodeModel::removeDirectory(const QByteArray &url)
{
    const QString path = url2Path(url);
    if (auto validEnvPtr = m_validEnv.ownerAs<DomEnvironment>())
        validEnvPtr->removePath(path);
    if (auto currentEnvPtr = m_currentEnv.ownerAs<DomEnvironment>())
        currentEnvPtr->removePath(path);
}

QString QQmlCodeModel::url2Path(const QByteArray &url, UrlLookup options)
{
    QString res;
    {
        QMutexLocker l(&m_mutex);
        res = m_url2path.value(url);
    }
    if (!res.isEmpty() && options == UrlLookup::Caching)
        return res;
    QUrl qurl(QString::fromUtf8(url));
    QFileInfo f(qurl.toLocalFile());
    QString cPath = f.canonicalFilePath();
    if (cPath.isEmpty())
        cPath = f.filePath();
    {
        QMutexLocker l(&m_mutex);
        if (!res.isEmpty() && res != cPath)
            m_path2url.remove(res);
        m_url2path.insert(url, cPath);
        m_path2url.insert(cPath, url);
    }
    return cPath;
}

void QQmlCodeModel::newOpenFile(const QByteArray &url, int version, const QString &docText)
{
    {
        QMutexLocker l(&m_mutex);
        auto &openDoc = m_openDocuments[url];
        if (!openDoc.textDocument)
            openDoc.textDocument = std::make_shared<Utils::TextDocument>();
        QMutexLocker l2(openDoc.textDocument->mutex());
        openDoc.textDocument->setVersion(version);
        openDoc.textDocument->setPlainText(docText);
    }
    addOpenToUpdate(url, NormalUpdate);
    openNeedUpdate();
}

OpenDocument QQmlCodeModel::openDocumentByUrl(const QByteArray &url)
{
    QMutexLocker l(&m_mutex);
    return m_openDocuments.value(url);
}

bool QQmlCodeModel::isEmpty() const
{
    QMutexLocker l(&m_mutex);
    return m_openDocuments.isEmpty();
}

RegisteredSemanticTokens &QQmlCodeModel::registeredTokens()
{
    QMutexLocker l(&m_mutex);
    return m_tokens;
}

const RegisteredSemanticTokens &QQmlCodeModel::registeredTokens() const
{
    QMutexLocker l(&m_mutex);
    return m_tokens;
}

void QQmlCodeModel::openNeedUpdate()
{
    qCDebug(codeModelLog) << "openNeedUpdate";
    const int maxThreads = 1;
    {
        QMutexLocker l(&m_mutex);
        if (m_openDocumentsToUpdate.isEmpty() || m_nUpdateInProgress >= maxThreads
            || m_state == State::Stopping) {
            return;
        }
        if (++m_nUpdateInProgress == 1)
            openUpdateStart();
    }
    QThreadPool::globalInstance()->start([this]() {
        QScopedValueRollback thread(m_openUpdateThread, QThread::currentThread());
        while (openUpdateSome()) { }
        emit openUpdateThreadFinished();
    });
}

bool QQmlCodeModel::openUpdateSome()
{
    qCDebug(codeModelLog) << "openUpdateSome start";
    QByteArray toUpdate;
    UpdatePolicy policy;
    {
        QMutexLocker l(&m_mutex);
        Q_ASSERT(QThread::currentThread() == m_openUpdateThread);

        if (m_openDocumentsToUpdate.isEmpty()) {
            if (--m_nUpdateInProgress == 0)
                openUpdateEnd();
            return false;
        }
        const auto it = m_openDocumentsToUpdate.begin();
        toUpdate = it.key();
        policy = it.value();
        m_openDocumentsToUpdate.erase(it);
    }
    bool hasMore = false;
    {
        auto guard = qScopeGuard([this, &hasMore]() {
            QMutexLocker l(&m_mutex);
            if (m_openDocumentsToUpdate.isEmpty()) {
                if (--m_nUpdateInProgress == 0)
                    openUpdateEnd();
                hasMore = false;
            } else {
                hasMore = true;
            }
        });
        openUpdate(toUpdate, policy);
    }
    return hasMore;
}

void QQmlCodeModel::openUpdateStart()
{
    qCDebug(codeModelLog) << "openUpdateStart";
}

void QQmlCodeModel::openUpdateEnd()
{
    qCDebug(codeModelLog) << "openUpdateEnd";
}

QStringList QQmlCodeModel::buildPathsForOpenedFiles()
{
    QStringList result;

    std::vector<QByteArray> urls = { m_rootUrl };
    {
        QMutexLocker guard(&m_mutex);
        for (auto it = m_openDocuments.begin(), end = m_openDocuments.end(); it != end; ++it)
            urls.push_back(it.key());
    }

    for (const auto &url : urls)
        result.append(buildPathsForFileUrl(url));

    // remove duplicates
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

static int cmakeJobsFromSettings(QQmlToolingSharedSettings *settings, const QString &rootPath,
                                 int defaultValue)
{
    if (!settings)
        return defaultValue;
    const auto result = settings->search(rootPath);
    if (!result.isValid())
        return defaultValue;

    bool ok = false;
    const QString valueString = settings->value("CMakeJobs"_L1).toString();
    if (valueString == QQmlCodeModel::s_maxCMakeJobs)
        return QThread::idealThreadCount();

    const int cmakeJobs = settings->value("CMakeJobs"_L1).toInt(&ok);
    if (!ok || cmakeJobs < 1)
        return defaultValue;
    return cmakeJobs;
}

void QQmlCodeModel::callCMakeBuild(QProcessScheduler *scheduler)
{
    const QStringList buildPaths = buildPathsForOpenedFiles();
    const int cmakeJobs = cmakeJobsFromSettings(m_settings, url2Path(m_rootUrl), m_cmakeJobs);

    QList<QProcessScheduler::Command> commands;
    for (const auto &path : buildPaths) {
        if (!QFileInfo::exists(path + u"/.cmake"_s))
            continue;

        auto [program, arguments] = QQmlLSUtils::cmakeBuildCommand(path, cmakeJobs);
        commands.append({ std::move(program), std::move(arguments) });
    }
    if (commands.isEmpty())
        return;
    qCInfo(codeModelLog) << "Starting background build on paths" << buildPaths.join(", "_L1);
    scheduler->schedule(commands, m_rootUrl.isEmpty() ? "Default workspace"_ba : m_rootUrl);
}

void QQmlCodeModel::reloadAllOpenFiles()
{
    QMutexLocker guard(&m_mutex);
    for (auto it = m_openDocuments.begin(), end = m_openDocuments.end(); it != end; ++it)
        m_openDocumentsToUpdate[it.key()] = ForceUpdate;
    guard.unlock();
    openNeedUpdate();
}

/*!
\internal
Iterate the entire source directory to find all C++ files that have their names in fileNames, and
return all the found file paths.

This is an overapproximation and might find unrelated files with the same name.
*/
QStringList QQmlCodeModel::findFilePathsFromFileNames(const QStringList &_fileNamesToSearch,
                                                      const QSet<QString> &ignoredFilePaths)
{
    Q_ASSERT(!m_rootUrl.isEmpty());
    QStringList fileNamesToSearch{ _fileNamesToSearch };

    {
        QMutexLocker guard(&m_mutex);
        // ignore files that were not found last time
        fileNamesToSearch.erase(std::remove_if(fileNamesToSearch.begin(), fileNamesToSearch.end(),
                                               [this](const QString &fileName) {
                                                   return m_ignoreForWatching.contains(fileName);
                                               }),
                                fileNamesToSearch.end());
    }

    const QString rootDir = QUrl(QString::fromUtf8(m_rootUrl)).toLocalFile();
    qCDebug(codeModelLog) << "Searching for files to watch in workspace folder" << rootDir;

    const QStringList result =
            QQmlLSUtils::findFilePathsFromFileNames(rootDir, fileNamesToSearch, ignoredFilePaths);

    QMutexLocker guard(&m_mutex);
    for (const auto &fileName : fileNamesToSearch) {
        if (std::none_of(result.begin(), result.end(),
                         [&fileName](const QString &path) { return path.endsWith(fileName); })) {
            m_ignoreForWatching.insert(fileName);
        }
    }
    return result;
}

/*!
\internal
Find all C++ file names (not path, for file paths call \l findFilePathsFromFileNames on the result
of this method) that this qmlFile relies on.
*/
QStringList QQmlCodeModel::fileNamesToWatch(const DomItem &qmlFile)
{
    const QmlFile *file = qmlFile.as<QmlFile>();
    if (!file)
        return {};

    auto resolver = file->typeResolver();
    if (!resolver)
        return {};

    auto types = resolver->importedTypes();

    QStringList result;
    for (const auto &type : types) {
        if (!type.scope)
            continue;
        // note: the factory only loads composite types
        const bool isComposite = type.scope.factory() || type.scope->isComposite();
        if (isComposite)
            continue;

        const QString filePath = QFileInfo(type.scope->filePath()).fileName();
        if (!filePath.isEmpty())
            result << filePath;
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

/*!
\internal
Add watches for all C++ files that this qmlFile relies on, so a rebuild can be triggered when they
are modified. Is a no op if this is the fallback codemodel with empty root url.
*/
void QQmlCodeModel::addFileWatches(const DomItem &qmlFile)
{
    if (m_rootUrl.isEmpty())
        return;
    const auto filesToWatch = fileNamesToWatch(qmlFile);

    // remove already watched files to avoid a warning later on
    const QStringList alreadyWatchedFiles = m_cppFileWatcher.files();
    const QSet<QString> alreadyWatchedFilesSet(alreadyWatchedFiles.begin(),
                                               alreadyWatchedFiles.end());
    QStringList filepathsToWatch = findFilePathsFromFileNames(filesToWatch, alreadyWatchedFilesSet);

    if (filepathsToWatch.isEmpty())
        return;

    QMutexLocker guard(&m_mutex);
    const auto unwatchedPaths = m_cppFileWatcher.addPaths(filepathsToWatch);
    if (!unwatchedPaths.isEmpty()) {
        qCDebug(codeModelLog) << "Cannot watch paths" << unwatchedPaths << "from requested"
                              << filepathsToWatch;
    }
}

enum VersionCheckResult {
    ClosedDocument,
    VersionLowerThanDocument,
    VersionLowerThanSnapshot,
    VersionOk,
};

enum VersionCheckResultForValidDocument {
    VersionLowerThanValidSnapshot,
    VersionOkForValidDocument,
};

VersionCheckResult checkVersion(const OpenDocument& doc, int version)
{
    if (!doc.textDocument)
        return ClosedDocument;

    {
        QMutexLocker guard2(doc.textDocument->mutex());
        if (doc.textDocument->version() && *doc.textDocument->version() > version)
            return VersionLowerThanDocument;
    }

    if (doc.snapshot.docVersion && *doc.snapshot.docVersion >= version)
        return VersionLowerThanSnapshot;

    return VersionOk;
}

static VersionCheckResultForValidDocument checkVersionForValidDocument(const OpenDocument &doc,
                                                                       int version)
{
    if (doc.snapshot.validDocVersion && *doc.snapshot.validDocVersion >= version)
        return VersionLowerThanValidSnapshot;

    return VersionOkForValidDocument;
}

static void updateItemInSnapshot(const DomItem &item, const DomItem &validItem,
                                 const QByteArray &url, OpenDocument *doc, int version,
                                 UpdatePolicy policy)
{
    switch (policy == ForceUpdate ? VersionOk : checkVersion(*doc, version)) {
    case ClosedDocument:
        qCWarning(lspServerLog) << "Ignoring update to closed document" << QString::fromUtf8(url);
        return;
    case VersionLowerThanDocument:
        qCWarning(lspServerLog) << "Version" << version << "of document" << QString::fromUtf8(url)
                                << "is not the latest anymore";
        return;
    case VersionLowerThanSnapshot:
        qCWarning(lspServerLog) << "Skipping update of current doc to obsolete version" << version
                                << "of document" << QString::fromUtf8(url);
        return;
    case VersionOk:
        doc->snapshot.docVersion = version;
        doc->snapshot.doc = item;
        break;
    }

    if (!item.field(Fields::isValid).value().toBool(false)) {
        qCWarning(lspServerLog) << "avoid update of validDoc to " << version << "of document"
                                << QString::fromUtf8(url) << "as it is invalid";
        return;
    }

    switch (policy == ForceUpdate ? VersionOkForValidDocument
                                  : checkVersionForValidDocument(*doc, version)) {
    case VersionLowerThanValidSnapshot:
        qCWarning(lspServerLog) << "Skipping update of valid doc to obsolete version" << version
                                << "of document" << QString::fromUtf8(url);
        return;
    case VersionOkForValidDocument:
        doc->snapshot.validDocVersion = version;
        doc->snapshot.validDoc = validItem;
        break;
    }
}

void QQmlCodeModel::newDocForOpenFile(const QByteArray &url, int version, const QString &docText,
                                      QmlLsp::UpdatePolicy policy)
{
    Q_ASSERT(QThread::currentThread() == m_openUpdateThread);
    qCDebug(codeModelLog) << "updating doc" << url << "to version" << version << "("
                          << docText.size() << "chars)";

    const QString fPath = url2Path(url, UrlLookup::ForceLookup);
    DomItem newCurrent = m_currentEnv.makeCopy(DomItem::CopyOption::EnvConnected).item();

    // if the documentation root path is not set through the commandline,
    // try to set it from the settings file (.qmlls.ini file)
    if (documentationRootPath().isEmpty() && m_settings) {
        // note: settings already searched current file in importPathsForFile() call above
        const QString docDir = QStringLiteral(u"docDir");
        if (m_settings->isSet(docDir))
            setDocumentationRootPath(m_settings->value(docDir).toString());
    }

    Path p;
    auto newCurrentPtr = newCurrent.ownerAs<DomEnvironment>();
    newCurrentPtr->setLoadPaths(importPathsForUrl(url));
    newCurrentPtr->setResourceFiles(m_resourceFiles);
    newCurrentPtr->loadFile(FileToLoad::fromMemory(newCurrentPtr, fPath, docText),
                            [&p, this](Path, const DomItem &, const DomItem &newValue) {
                                const DomItem file = newValue.fileObject();
                                p = file.canonicalPath();
                                // Force population of the file by accessing isValid field. We
                                // don't want to populate the file after adding the file to the
                                // snapshot in updateItemInSnapshot.
                                file.field(Fields::isValid);
                                if (cmakeStatus() == HasCMake)
                                    addFileWatches(file);
                            });
    newCurrentPtr->loadPendingDependencies();
    if (p) {
        newCurrent.commitToBase(m_validEnv.ownerAs<DomEnvironment>());
        QMutexLocker l(&m_mutex);
        updateItemInSnapshot(m_currentEnv.path(p), m_validEnv.path(p), url, &m_openDocuments[url],
                             version, policy);
    }
    if (codeModelLog().isDebugEnabled()) {
        qCDebug(codeModelLog) << "Finished update doc of " << url << "to version" << version;
        snapshotByUrl(url).dump(qDebug() << "postSnapshot",
                                OpenDocumentSnapshot::DumpOption::AllCode);
    }
    // we should update the scope in the future thus call addOpen(url)
    emit updatedSnapshot(url, policy);
}

void QQmlCodeModel::closeOpenFile(const QByteArray &url)
{
    QMutexLocker l(&m_mutex);
    m_openDocuments.remove(url);
}

QByteArray QQmlCodeModel::rootUrl() const
{
    QMutexLocker l(&m_mutex);
    return m_rootUrl;
}

QStringList QQmlCodeModel::buildPaths()
{
    QMutexLocker l(&m_mutex);
    return m_buildPaths;
}

QStringList QQmlCodeModel::importPathsForUrl(const QByteArray &url)
{
    QStringList result = importPaths();

    // fallback for projects targeting Qt < 6.10, that don't have .qmlls.build.ini files
    if (result.isEmpty() || result == QLibraryInfo::paths(QLibraryInfo::QmlImportsPath))
        result << buildPathsForFileUrl(url);

    const QString importPathsKey = u"importPaths"_s;
    const QString fileName = url2Path(url);
    if (m_settings && m_settings->search(fileName, { QString(), verbose() }).isValid()
        && m_settings->isSet(importPathsKey)) {
        result.append(m_settings->valueAsAbsolutePathList(importPathsKey, fileName));
    }
    return result;
}

void QQmlCodeModel::setImportPaths(const QStringList &importPaths)
{
    QMutexLocker guard(&m_mutex);
    m_importPaths = importPaths;

    if (const auto &env = m_currentEnv.ownerAs<DomEnvironment>())
        env->setLoadPaths(importPaths);
    if (const auto &env = m_validEnv.ownerAs<DomEnvironment>())
        env->setLoadPaths(importPaths);
}

QStringList QQmlCodeModel::resourceFiles() const
{
    QMutexLocker guard(&m_mutex);
    return m_resourceFiles;
}

void QQmlCodeModel::setResourceFiles(const QStringList &resourceFiles)
{
    QMutexLocker guard(&m_mutex);
    m_resourceFiles = resourceFiles;

    if (const auto &env = m_currentEnv.ownerAs<DomEnvironment>())
        env->setResourceFiles(resourceFiles);
    if (const auto &env = m_validEnv.ownerAs<DomEnvironment>())
        env->setResourceFiles(resourceFiles);
}

QStringList QQmlCodeModel::importPaths() const
{
    QMutexLocker guard(&m_mutex);
    return m_importPaths;
}

static QStringList withDependentBuildDirectories(QStringList &&buildPaths)
{
    // add dependent build directories
    QStringList res;
    std::reverse(buildPaths.begin(), buildPaths.end());
    const int maxDeps = 4;
    while (!buildPaths.isEmpty()) {
        res += buildPaths.takeLast();
        const QString &bPath = res.constLast();
        const QString bPathExtended = bPath + u"/_deps";
        if (QFile::exists(bPathExtended) && bPath.count(u"/_deps/"_s) < maxDeps) {
            for (const auto &fileInfo :
                 QDirListing{ bPathExtended, QDirListing::IteratorFlag::DirsOnly }) {
                buildPaths.append(fileInfo.absoluteFilePath());
            }
        }
    }
    return res;
}

QStringList QQmlCodeModel::buildPathsForFileUrl(const QByteArray &url)
{
    if (QStringList result = buildPaths(); !result.isEmpty())
        return withDependentBuildDirectories(std::move(result));

    // fallback: look in the user settings (.qmlls.ini files in the source directory)
    if (!m_settings || !m_settings->search(url2Path(url), { QString(), verbose() }).isValid())
        return {};

    constexpr QLatin1String buildDir = "buildDir"_L1;
    if (!m_settings->isSet(buildDir))
        return {};

    const QString fileName = url2Path(url);
    return withDependentBuildDirectories(m_settings->valueAsAbsolutePathList(buildDir, fileName));
}

void QQmlCodeModel::setDocumentationRootPath(const QString &path)
{
    {
        QMutexLocker l(&m_mutex);
        if (m_documentationRootPath == path)
            return;
        m_documentationRootPath = path;
    }
    m_helpManager.setDocumentationRootPath(path);
}

void QQmlCodeModel::setBuildPaths(const QStringList &paths)
{
    QMutexLocker l(&m_mutex);
    m_buildPaths = paths;
}

void QQmlCodeModel::openUpdate(const QByteArray &url, UpdatePolicy policy)
{
    std::optional<int> rNow = 0;
    QString docText;

    {
        QMutexLocker l(&m_mutex);
        Q_ASSERT(QThread::currentThread() == m_openUpdateThread);
        OpenDocument &doc = m_openDocuments[url];
        std::shared_ptr<Utils::TextDocument> document = doc.textDocument;
        if (!document)
            return;
        {
            QMutexLocker l2(document->mutex());
            rNow = document->version();
        }
        if (!rNow
            || (policy != ForceUpdate && doc.snapshot.docVersion
                && *doc.snapshot.docVersion == *rNow)) {
            return;
        }

        {
            QMutexLocker l2(doc.textDocument->mutex());
            rNow = doc.textDocument->version();
            docText = doc.textDocument->toPlainText();
        }
    }
    newDocForOpenFile(url, *rNow, docText, policy);
}

void QQmlCodeModel::addOpenToUpdate(const QByteArray &url, UpdatePolicy policy)
{
    {
        QMutexLocker l(&m_mutex);
        m_openDocumentsToUpdate[url] = policy;
    }
    openNeedUpdate();
}

QDebug OpenDocumentSnapshot::dump(QDebug dbg, DumpOptions options)
{
    dbg.noquote().nospace() << "{";
    dbg << "  url:" << QString::fromUtf8(url) << "\n";
    dbg << "  docVersion:" << (docVersion ? QString::number(*docVersion) : u"*none*"_s) << "\n";
    if (options & DumpOption::LatestCode) {
        dbg << "  doc: ------------\n"
            << doc.field(Fields::code).value().toString() << "\n==========\n";
    } else {
        dbg << u"  doc:"
            << (doc ? u"%1chars"_s.arg(doc.field(Fields::code).value().toString().size())
                    : u"*none*"_s)
            << "\n";
    }
    dbg << "  validDocVersion:"
        << (validDocVersion ? QString::number(*validDocVersion) : u"*none*"_s) << "\n";
    if (options & DumpOption::ValidCode) {
        dbg << "  validDoc: ------------\n"
            << validDoc.field(Fields::code).value().toString() << "\n==========\n";
    } else {
        dbg << u"  validDoc:"
            << (validDoc ? u"%1chars"_s.arg(validDoc.field(Fields::code).value().toString().size())
                         : u"*none*"_s)
            << "\n";
    }
    dbg << "}";
    return dbg;
}

static ModuleSetting *moduleSettingFor(const QString &sourceFolder, ModuleSettings *moduleSettings,
                                       UpdatePolicy policy)
{
    if (policy != ForceUpdate)
        return &moduleSettings->emplaceBack(ModuleSetting {sourceFolder, {}, {}});

    auto it = std::find_if(
            moduleSettings->begin(), moduleSettings->end(),
            [&sourceFolder](const ModuleSetting s) { return s.sourceFolder == sourceFolder; });
    if (it == moduleSettings->end())
        return &moduleSettings->emplaceBack(ModuleSetting {sourceFolder, {}, {}});
    return &*it;
}

static QStringList asStringList(const QVariant &variant)
{
    return variant.toString().split(QDir::listSeparator(), Qt::SkipEmptyParts);
}

// hotfix for projects targeting Qt 6.11, where the .ini array starts at 0 instead of 1
static void handleArrayStartingAt0(QSettings *settings, ModuleSettings *moduleSettings,
                                   UpdatePolicy policy)
{
    static constexpr QLatin1String sourcePathKey = "workspaces/0/sourcePath"_L1;
    if (!settings->contains(sourcePathKey))
        return;

    ModuleSetting *moduleSetting =
            moduleSettingFor(settings->value(sourcePathKey).toString(), moduleSettings, policy);
    moduleSetting->importPaths = asStringList(settings->value("workspaces/0/importPaths"_L1));
    moduleSetting->resourceFiles = asStringList(settings->value("workspaces/0/resourceFiles"_L1));
}

static void loadWorkspacesV2(QSettings *settings, ModuleSettings *moduleSettings,
                             UpdatePolicy policy)
{
    handleArrayStartingAt0(settings, moduleSettings, policy);

    const int entries = settings->beginReadArray("workspaces"_L1);
    for (int i = 0; i < entries; ++i) {
        settings->setArrayIndex(i);

        if (!settings->contains("sourcePath"_L1))
            continue;

        ModuleSetting *moduleSetting =
                moduleSettingFor(settings->value("sourcePath").toString(), moduleSettings, policy);
        moduleSetting->importPaths = asStringList(settings->value("importPaths"_L1));
        moduleSetting->resourceFiles = asStringList(settings->value("resourceFiles"_L1));
    }
    settings->endArray();
}

void QQmllsBuildInformation::loadSettingsFrom(const QStringList &buildPaths, UpdatePolicy policy)
{
#if QT_CONFIG(settings)
    for (const QString &path : buildPaths) {
        if (policy != ForceUpdate && m_seenSettings.contains(path))
            continue;
        m_seenSettings.insert(path);

        const QString iniPath = QString(path).append("/.qt/.qmlls.build.ini"_L1);
        if (!QFile::exists(iniPath))
            continue;

        QSettings settings(iniPath, QSettings::IniFormat);
        m_docDir = settings.value("docDir"_L1).toString();
        const qsizetype version = settings.value("version"_L1, "1"_L1).toString().toInt();
        switch (version) {
        case 2: {
            loadWorkspacesV2(&settings, &m_moduleSettings, policy);
            break;
        }
        case 1:
        default: {
            for (const QString &group : settings.childGroups()) {
                settings.beginGroup(group);

                ModuleSetting *moduleSetting = moduleSettingFor(
                        QString(group).replace("<SLASH>"_L1, "/"_L1), &m_moduleSettings, policy);
                moduleSetting->importPaths = asStringList(settings.value("importPaths"_L1));
                moduleSetting->resourceFiles = asStringList(settings.value("resourceFiles"_L1));
                settings.endGroup();
            }
            break;
        }
        }
    }
#else
    Q_UNUSED(buildPaths);
#endif
}

QStringList QQmllsBuildInformation::importPathsFor(const QString &filePath)
{
    return settingFor(filePath).importPaths;
}

QStringList QQmllsBuildInformation::resourceFilesFor(const QString &filePath)
{
    return settingFor(filePath).resourceFiles;
}

ModuleSetting QQmllsBuildInformation::settingFor(const QString &filePath)
{
    ModuleSetting result;
    qsizetype longestMatch = 0;
    for (const ModuleSetting &setting : m_moduleSettings) {
        const qsizetype matchLength = setting.sourceFolder.size();
        if (filePath.startsWith(setting.sourceFolder) && matchLength > longestMatch) {
            result = setting;
            longestMatch = matchLength;
        }
    }
    QQmlToolingSettings::resolveRelativeImportPaths(filePath, &result.importPaths);
    return result;
}

void QQmllsBuildInformation::addModuleSetting(const ModuleSetting &moduleSetting)
{
    m_moduleSettings.append(moduleSetting);
}

void QQmllsBuildInformation::writeQmllsBuildIniContent(const QString &file) const
{
#if QT_CONFIG(settings)
    QSettings settings(file, QSettings::IniFormat);
    settings.setValue("docDir"_L1, m_docDir);
    settings.setValue("version"_L1, "2"_L1);
    settings.beginWriteArray("workspaces"_L1, m_moduleSettings.size());
    for (int i = 0; i < m_moduleSettings.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("sourcePath"_L1, m_moduleSettings[i].sourceFolder);
        settings.setValue("importPaths"_L1,
                          m_moduleSettings[i].importPaths.join(QDir::listSeparator()));
        settings.setValue("resourceFiles"_L1,
                          m_moduleSettings[i].resourceFiles.join(QDir::listSeparator()));
    }
    settings.endArray();

#else
    Q_UNUSED(file);
#endif
}

QQmllsBuildInformation::QQmllsBuildInformation() { }

} // namespace QmlLsp

QT_END_NAMESPACE
