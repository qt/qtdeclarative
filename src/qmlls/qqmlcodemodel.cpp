// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmlcodemodel_p.h"
#include "qtextdocument_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/qlibraryinfo.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

#include <memory>
#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QmlLsp {

Q_LOGGING_CATEGORY(codeModelLog, "qt.languageserver.codemodel")

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
There are two main task that are executed in parallel: Indexing, and OpenDocumentUpdate.
Indexing is meant to keep the global view up to date.
OpenDocumentUpdate keeps the snapshots of the open documents up to date.

There is always a tension between being responsive, using all threads available, and avoid to hog
too many resources. One can choose different parallelization strategies, we went with a flexiable
approach.
We have (private) functions that execute part of the work: indexSome() and openUpdateSome(). These
do all locking needed, get some work, do it without locks, and at the end update the state of the
code model. If there is more work, then they return true. Thus while (xxxSome()); works until there
is no work left.

addDirectoriesToIndex(), the internal addDirectory() and addOpenToUpdate() add more work to do.

indexNeedsUpdate() and openNeedUpdate(), check if there is work to do, and if yes ensure that a
worker thread (or more) that work on it exist.
*/

QQmlCodeModel::QQmlCodeModel(QObject *parent, QQmlToolingSettings *settings)
    : QObject { parent },
      m_currentEnv(std::make_shared<DomEnvironment>(
                       QStringList(QLibraryInfo::path(QLibraryInfo::QmlImportsPath)),
                       DomEnvironment::Option::SingleThreaded)),
      m_validEnv(std::make_shared<DomEnvironment>(
                     QStringList(QLibraryInfo::path(QLibraryInfo::QmlImportsPath)),
                     DomEnvironment::Option::SingleThreaded)),
      m_settings(settings)
{
}

QQmlCodeModel::~QQmlCodeModel()
{
    while (true) {
        bool shouldWait;
        {
            QMutexLocker l(&m_mutex);
            m_state = State::Stopping;
            m_openDocumentsToUpdate.clear();
            shouldWait = m_nIndexInProgress != 0 || m_nUpdateInProgress != 0;
        }
        if (!shouldWait)
            break;
        QThread::yieldCurrentThread();
    }
}

OpenDocumentSnapshot QQmlCodeModel::snapshotByUrl(const QByteArray &url)
{
    return openDocumentByUrl(url).snapshot;
}

int QQmlCodeModel::indexEvalProgress() const
{
    Q_ASSERT(!m_mutex.tryLock()); // should be called while locked
    const int dirCost = 10;
    int costToDo = 1;
    for (const ToIndex &el : std::as_const(m_toIndex))
        costToDo += dirCost * el.leftDepth;
    costToDo += m_indexInProgressCost;
    return m_indexDoneCost * 100 / (costToDo + m_indexDoneCost);
}

void QQmlCodeModel::indexStart()
{
    Q_ASSERT(!m_mutex.tryLock()); // should be called while locked
    qCDebug(codeModelLog) << "indexStart";
}

void QQmlCodeModel::indexEnd()
{
    Q_ASSERT(!m_mutex.tryLock()); // should be called while locked
    qCDebug(codeModelLog) << "indexEnd";
    m_lastIndexProgress = 0;
    m_nIndexInProgress = 0;
    m_toIndex.clear();
    m_indexInProgressCost = 0;
    m_indexDoneCost = 0;
}

void QQmlCodeModel::indexSendProgress(int progress)
{
    if (progress <= m_lastIndexProgress)
        return;
    m_lastIndexProgress = progress;
    // ### actually send progress
}

bool QQmlCodeModel::indexCancelled()
{
    QMutexLocker l(&m_mutex);
    if (m_state == State::Stopping)
        return true;
    return false;
}

void QQmlCodeModel::indexDirectory(const QString &path, int depthLeft)
{
    if (indexCancelled())
        return;
    QDir dir(path);
    if (depthLeft > 1) {
        const QStringList dirs =
                dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        for (const QString &child : dirs)
            addDirectory(dir.filePath(child), --depthLeft);
    }
    const QStringList qmljs =
            dir.entryList(QStringList({ u"*.qml"_s, u"*.js"_s, u"*.mjs"_s }), QDir::Files);
    int progress = 0;
    {
        QMutexLocker l(&m_mutex);
        m_indexInProgressCost += qmljs.size();
        progress = indexEvalProgress();
    }
    indexSendProgress(progress);
    if (qmljs.isEmpty())
        return;
    DomItem newCurrent = m_currentEnv.makeCopy(DomItem::CopyOption::EnvConnected).item();
    for (const QString &file : qmljs) {
        if (indexCancelled())
            return;
        QString fPath = dir.filePath(file);
        DomCreationOptions options;
        options.setFlag(DomCreationOption::WithScriptExpressions);
        options.setFlag(DomCreationOption::WithSemanticAnalysis);
        FileToLoad fileToLoad =
                FileToLoad::fromFileSystem(newCurrent.ownerAs<DomEnvironment>(), fPath, options);
        if (!fileToLoad.canonicalPath().isEmpty()) {
            newCurrent.loadBuiltins();
            newCurrent.loadFile(fileToLoad, [](Path, DomItem &, DomItem &) {}, {});
            newCurrent.loadPendingDependencies();
            newCurrent.commitToBase(m_validEnv.ownerAs<DomEnvironment>());
        }
        {
            QMutexLocker l(&m_mutex);
            ++m_indexDoneCost;
            --m_indexInProgressCost;
            progress = indexEvalProgress();
        }
        indexSendProgress(progress);
    }
}

void QQmlCodeModel::addDirectoriesToIndex(const QStringList &paths, QLanguageServer *server)
{
    Q_UNUSED(server);
    // ### create progress, &scan in a separate instance
    const int maxDepth = 5;
    for (const auto &path : paths)
        addDirectory(path, maxDepth);
    indexNeedsUpdate();
}

void QQmlCodeModel::addDirectory(const QString &path, int depthLeft)
{
    if (depthLeft < 1)
        return;
    {
        QMutexLocker l(&m_mutex);
        for (auto it = m_toIndex.begin(); it != m_toIndex.end();) {
            if (it->path.startsWith(path)) {
                if (it->path.size() == path.size())
                    return;
                if (it->path.at(path.size()) == u'/') {
                    it = m_toIndex.erase(it);
                    continue;
                }
            } else if (path.startsWith(it->path) && path.at(it->path.size()) == u'/')
                return;
            ++it;
        }
        m_toIndex.append({ path, depthLeft });
    }
}

void QQmlCodeModel::removeDirectory(const QString &path)
{
    {
        QMutexLocker l(&m_mutex);
        auto toRemove = [path](const QString &p) {
            return p.startsWith(path) && (p.size() == path.size() || p.at(path.size()) == u'/');
        };
        auto it = m_toIndex.begin();
        auto end = m_toIndex.end();
        while (it != end) {
            if (toRemove(it->path))
                it = m_toIndex.erase(it);
            else
                ++it;
        }
    }
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
    addOpenToUpdate(url);
    openNeedUpdate();
}

OpenDocument QQmlCodeModel::openDocumentByUrl(const QByteArray &url)
{
    QMutexLocker l(&m_mutex);
    return m_openDocuments.value(url);
}

void QQmlCodeModel::indexNeedsUpdate()
{
    const int maxIndexThreads = 1;
    {
        QMutexLocker l(&m_mutex);
        if (m_toIndex.isEmpty() || m_nIndexInProgress >= maxIndexThreads)
            return;
        if (++m_nIndexInProgress == 1)
            indexStart();
    }
    QThreadPool::globalInstance()->start([this]() {
        while (indexSome()) { }
    });
}

bool QQmlCodeModel::indexSome()
{
    qCDebug(codeModelLog) << "indexSome";
    ToIndex toIndex;
    {
        QMutexLocker l(&m_mutex);
        if (m_toIndex.isEmpty()) {
            if (--m_nIndexInProgress == 0)
                indexEnd();
            return false;
        }
        toIndex = m_toIndex.last();
        m_toIndex.removeLast();
    }
    bool hasMore = false;
    {
        auto guard = qScopeGuard([this, &hasMore]() {
            QMutexLocker l(&m_mutex);
            if (m_toIndex.isEmpty()) {
                if (--m_nIndexInProgress == 0)
                    indexEnd();
                hasMore = false;
            } else {
                hasMore = true;
            }
        });
        indexDirectory(toIndex.path, toIndex.leftDepth);
    }
    return hasMore;
}

void QQmlCodeModel::openNeedUpdate()
{
    qCDebug(codeModelLog) << "openNeedUpdate";
    const int maxIndexThreads = 1;
    {
        QMutexLocker l(&m_mutex);
        if (m_openDocumentsToUpdate.isEmpty() || m_nUpdateInProgress >= maxIndexThreads)
            return;
        if (++m_nUpdateInProgress == 1)
            openUpdateStart();
    }
    QThreadPool::globalInstance()->start([this]() {
        while (openUpdateSome()) { }
    });
}

bool QQmlCodeModel::openUpdateSome()
{
    qCDebug(codeModelLog) << "openUpdateSome start";
    QByteArray toUpdate;
    {
        QMutexLocker l(&m_mutex);
        if (m_openDocumentsToUpdate.isEmpty()) {
            if (--m_nUpdateInProgress == 0)
                openUpdateEnd();
            return false;
        }
        auto it = m_openDocumentsToUpdate.find(m_lastOpenDocumentUpdated);
        auto end = m_openDocumentsToUpdate.end();
        if (it == end)
            it = m_openDocumentsToUpdate.begin();
        else if (++it == end)
            it = m_openDocumentsToUpdate.begin();
        toUpdate = *it;
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
        openUpdate(toUpdate);
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

void QQmlCodeModel::newDocForOpenFile(const QByteArray &url, int version, const QString &docText)
{
    qCDebug(codeModelLog) << "updating doc" << url << "to version" << version << "("
                          << docText.size() << "chars)";
    DomItem newCurrent = m_currentEnv.makeCopy(DomItem::CopyOption::EnvConnected).item();
    QStringList loadPaths = buildPathsForFileUrl(url);
    loadPaths.append(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));
    if (std::shared_ptr<DomEnvironment> newCurrentPtr = newCurrent.ownerAs<DomEnvironment>()) {
        newCurrentPtr->setLoadPaths(loadPaths);
    }
    QString fPath = url2Path(url, UrlLookup::ForceLookup);
    Path p;
    DomCreationOptions options;
    options.setFlag(DomCreationOption::WithScriptExpressions);
    options.setFlag(DomCreationOption::WithSemanticAnalysis);
    newCurrent.loadFile(
            FileToLoad::fromMemory(newCurrent.ownerAs<DomEnvironment>(), fPath, docText, options),
            [&p](Path, DomItem &, DomItem &newValue) { p = newValue.fileObject().canonicalPath(); },
            {});
    newCurrent.loadPendingDependencies();
    if (p) {
        newCurrent.commitToBase(m_validEnv.ownerAs<DomEnvironment>());
        DomItem item = m_currentEnv.path(p);
        {
            QMutexLocker l(&m_mutex);
            OpenDocument &doc = m_openDocuments[url];
            if (!doc.textDocument) {
                qCWarning(lspServerLog)
                        << "ignoring update to closed document" << QString::fromUtf8(url);
                return;
            } else {
                QMutexLocker l(doc.textDocument->mutex());
                if (doc.textDocument->version() && *doc.textDocument->version() > version) {
                    qCWarning(lspServerLog)
                            << "docUpdate: version" << version << "of document"
                            << QString::fromUtf8(url) << "is not the latest anymore";
                    return;
                }
            }
            if (!doc.snapshot.docVersion || *doc.snapshot.docVersion < version) {
                doc.snapshot.docVersion = version;
                doc.snapshot.doc = item;
            } else {
                qCWarning(lspServerLog) << "skipping update of current doc to obsolete version"
                                        << version << "of document" << QString::fromUtf8(url);
            }
            if (item.field(Fields::isValid).value().toBool(false)) {
                if (!doc.snapshot.validDocVersion || *doc.snapshot.validDocVersion < version) {
                    DomItem vDoc = m_validEnv.path(p);
                    doc.snapshot.validDocVersion = version;
                    doc.snapshot.validDoc = vDoc;
                } else {
                    qCWarning(lspServerLog) << "skippig update of valid doc to obsolete version"
                                            << version << "of document" << QString::fromUtf8(url);
                }
            } else {
                qCWarning(lspServerLog)
                        << "avoid update of validDoc to " << version << "of document"
                        << QString::fromUtf8(url) << "as it is invalid";
            }
        }
    }
    if (codeModelLog().isDebugEnabled()) {
        qCDebug(codeModelLog) << "finished update doc of " << url << "to version" << version;
        snapshotByUrl(url).dump(qDebug() << "postSnapshot",
                                OpenDocumentSnapshot::DumpOption::AllCode);
    }
    // we should update the scope in the future thus call addOpen(url)
    emit updatedSnapshot(url);
}

void QQmlCodeModel::closeOpenFile(const QByteArray &url)
{
    QMutexLocker l(&m_mutex);
    m_openDocuments.remove(url);
}

void QQmlCodeModel::setRootUrls(const QList<QByteArray> &urls)
{
    QMutexLocker l(&m_mutex);
    m_rootUrls = urls;
}

void QQmlCodeModel::addRootUrls(const QList<QByteArray> &urls)
{
    QMutexLocker l(&m_mutex);
    for (const QByteArray &url : urls) {
        if (!m_rootUrls.contains(url))
            m_rootUrls.append(url);
    }
}

void QQmlCodeModel::removeRootUrls(const QList<QByteArray> &urls)
{
    QMutexLocker l(&m_mutex);
    for (const QByteArray &url : urls)
        m_rootUrls.removeOne(url);
}

QList<QByteArray> QQmlCodeModel::rootUrls() const
{
    QMutexLocker l(&m_mutex);
    return m_rootUrls;
}

QStringList QQmlCodeModel::buildPathsForRootUrl(const QByteArray &url)
{
    QMutexLocker l(&m_mutex);
    return m_buildPathsForRootUrl.value(url);
}

static bool isNotSeparator(char c)
{
    return c != '/';
}

QStringList QQmlCodeModel::buildPathsForFileUrl(const QByteArray &url)
{
    QList<QByteArray> roots;
    {
        QMutexLocker l(&m_mutex);
        roots = m_buildPathsForRootUrl.keys();
    }
    // we want to longest match to be first, as it should override shorter matches
    std::sort(roots.begin(), roots.end(), [](const QByteArray &el1, const QByteArray &el2) {
        if (el1.size() > el2.size())
            return true;
        if (el1.size() < el2.size())
            return false;
        return el1 < el2;
    });
    QStringList buildPaths;
    QStringList defaultValues;
    if (!roots.isEmpty() && roots.last().isEmpty())
        roots.removeLast();
    QByteArray urlSlash(url);
    if (!urlSlash.isEmpty() && isNotSeparator(urlSlash.at(urlSlash.size() - 1)))
        urlSlash.append('/');
    // look if the file has a know prefix path
    for (const QByteArray &root : roots) {
        if (urlSlash.startsWith(root)) {
            buildPaths += buildPathsForRootUrl(root);
            break;
        }
    }
    QString path = url2Path(url);

    // fallback to the empty root, if is has an entry.
    // This is the buildPath that is passed to qmlls via --build-dir.
    if (buildPaths.isEmpty()) {
        buildPaths += buildPathsForRootUrl(QByteArray());
    }

    // look in the QMLLS_BUILD_DIRS environment variable
    if (buildPaths.isEmpty()) {
        QStringList envPaths = qEnvironmentVariable("QMLLS_BUILD_DIRS")
                                       .split(QDir::listSeparator(), Qt::SkipEmptyParts);
        buildPaths += envPaths;
    }

    // look in the settings.
    // This is the one that is passed via the .qmlls.ini file.
    if (buildPaths.isEmpty() && m_settings) {
        m_settings->search(path);
        QString buildDir = QStringLiteral(u"buildDir");
        if (m_settings->isSet(buildDir))
            buildPaths += m_settings->value(buildDir).toString().split(QDir::listSeparator(),
                                                                       Qt::SkipEmptyParts);
    }

    // heuristic to find build directory
    if (buildPaths.isEmpty()) {
        QDir d(path);
        d.setNameFilters(QStringList({ u"build*"_s }));
        const int maxDirDepth = 8;
        int iDir = maxDirDepth;
        QString dirName = d.dirName();
        QDateTime lastModified;
        while (d.cdUp() && --iDir > 0) {
            for (const QFileInfo &fInfo : d.entryInfoList(QDir::Dirs)) {
                if (fInfo.completeBaseName() == u"build"
                    || fInfo.completeBaseName().startsWith(u"build-%1"_s.arg(dirName))) {
                    if (iDir > 1)
                        iDir = 1;
                    if (!lastModified.isValid() || lastModified < fInfo.lastModified()) {
                        buildPaths.clear();
                        buildPaths.append(fInfo.absoluteFilePath());
                    }
                }
            }
        }
    }
    // add dependent build directories
    QStringList res;
    std::reverse(buildPaths.begin(), buildPaths.end());
    const int maxDeps = 4;
    while (!buildPaths.isEmpty()) {
        QString bPath = buildPaths.last();
        buildPaths.removeLast();
        res += bPath;
        if (QFile::exists(bPath + u"/_deps") && bPath.split(u"/_deps/"_s).size() < maxDeps) {
            QDir d(bPath + u"/_deps");
            for (const QFileInfo &fInfo : d.entryInfoList(QDir::Dirs))
                buildPaths.append(fInfo.absoluteFilePath());
        }
    }
    return res;
}

void QQmlCodeModel::setBuildPathsForRootUrl(QByteArray url, const QStringList &paths)
{
    QMutexLocker l(&m_mutex);
    if (!url.isEmpty() && isNotSeparator(url.at(url.size() - 1)))
        url.append('/');
    if (paths.isEmpty())
        m_buildPathsForRootUrl.remove(url);
    else
        m_buildPathsForRootUrl.insert(url, paths);
}

void QQmlCodeModel::openUpdate(const QByteArray &url)
{
    bool updateDoc = false;
    bool updateScope = false;
    std::optional<int> rNow = 0;
    QString docText;
    DomItem validDoc;
    std::shared_ptr<Utils::TextDocument> document;
    {
        QMutexLocker l(&m_mutex);
        OpenDocument &doc = m_openDocuments[url];
        document = doc.textDocument;
        if (!document)
            return;
        {
            QMutexLocker l2(document->mutex());
            rNow = document->version();
        }
        if (rNow && (!doc.snapshot.docVersion || *doc.snapshot.docVersion != *rNow))
            updateDoc = true;
        else if (doc.snapshot.validDocVersion
                 && (!doc.snapshot.scopeVersion
                     || *doc.snapshot.scopeVersion != *doc.snapshot.validDocVersion))
            updateScope = true;
        else
            return;
        if (updateDoc) {
            QMutexLocker l2(doc.textDocument->mutex());
            rNow = doc.textDocument->version();
            docText = doc.textDocument->toPlainText();
        } else {
            validDoc = doc.snapshot.validDoc;
            rNow = doc.snapshot.validDocVersion;
        }
    }
    if (updateDoc) {
        newDocForOpenFile(url, *rNow, docText);
    }
    if (updateScope) {
        // to do
    }
}

void QQmlCodeModel::addOpenToUpdate(const QByteArray &url)
{
    QMutexLocker l(&m_mutex);
    m_openDocumentsToUpdate.insert(url);
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
    dbg << "  scopeVersion:" << (scopeVersion ? QString::number(*scopeVersion) : u"*none*"_s)
        << "\n";
    dbg << "  scopeDependenciesLoadTime:" << scopeDependenciesLoadTime << "\n";
    dbg << "  scopeDependenciesChanged" << scopeDependenciesChanged << "\n";
    dbg << "}";
    return dbg;
}

} // namespace QmlLsp

QT_END_NAMESPACE
