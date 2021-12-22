/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qqmllanguageserver.h"
#include "qqmlcodemodel.h"
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qthreadpool.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include "textdocument.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QmlLsp {

Q_LOGGING_CATEGORY(codeModelLog, "qt.languageserver.codemodel")

using namespace QQmlJS::Dom;

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
\li snapshotByUri() returns an OpenDocumentSnapshot of an open document. From it you can get the
  document, its latest valid version, scope, all connected to a specific version of the document
  and immutable. The signal updatedSnapshot() is called every time a snapshot changes (also for
  every partial change: document change, validDocument change, scope change).
\li openDocumentByUri() is a lower level and more intrusive access to OpenDocument objects. These
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

QQmlCodeModel::QQmlCodeModel(QObject *parent)
    : QObject { parent },
      m_currentEnv(std::make_shared<DomEnvironment>(QStringList(),
                                                    DomEnvironment::Option::SingleThreaded)),
      m_validEnv(std::make_shared<DomEnvironment>(QStringList(),
                                                  DomEnvironment::Option::SingleThreaded))
{
}

OpenDocumentSnapshot QQmlCodeModel::snapshotByUri(const QByteArray &uri)
{
    return openDocumentByUri(uri).snapshot;
}

int QQmlCodeModel::indexEvalProgress() const
{
    Q_ASSERT(!m_mutex.tryLock()); // should be called while locked
    const int dirCost = 10;
    int costToDo = 1;
    for (const ToIndex &el : qAsConst(m_toIndex))
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
    m_nUpdateInProgress = 0;
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
    const QStringList qmljs = dir.entryList(QStringList({ "*.qml", "*.js", "*.mjs" }), QDir::Files);
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
    int iFile = 0;
    for (const QString &file : qmljs) {
        if (indexCancelled())
            return;
        QString fPath = dir.filePath(file);
        QFileInfo fInfo(fPath);
        QString cPath = fInfo.canonicalFilePath();
        if (!cPath.isEmpty()) {
            bool isNew = false;
            newCurrent.loadFile(cPath, fPath,
                                [&isNew](Path, DomItem &oldValue, DomItem &newValue) {
                                    if (oldValue != newValue)
                                        isNew = true;
                                },
                                {});
            newCurrent.loadPendingDependencies();
            if (isNew) {
                newCurrent.commitToBase();
                m_currentEnv.makeCopy(DomItem::CopyOption::EnvConnected).item();
            }
        }
        ++iFile;
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
        auto it = m_toIndex.begin();
        auto end = m_toIndex.end();
        while (it != end) {
            if (it->path.startsWith(path)) {
                if (it->path.size() == path.size())
                    return;
                if (it->path.at(path.size()) == u'/') {
                    it = m_toIndex.erase(it);
                    continue;
                }
            } else if (path.startsWith(it->path) && path.at(it->path.size()) == u'/')
                return;
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

QString QQmlCodeModel::uri2Path(const QByteArray &uri, UriLookup options)
{
    QString res;
    {
        QMutexLocker l(&m_mutex);
        res = m_uri2path.value(uri);
    }
    if (!res.isEmpty() && options == UriLookup::Caching)
        return res;
    QUrl url(QString::fromUtf8(uri));
    QFileInfo f(url.toLocalFile());
    QString cPath = f.canonicalFilePath();
    if (cPath.isEmpty())
        cPath = f.filePath();
    {
        QMutexLocker l(&m_mutex);
        if (!res.isEmpty() && res != cPath)
            m_path2uri.remove(res);
        m_uri2path.insert(uri, cPath);
        m_path2uri.insert(cPath, uri);
    }
    return cPath;
}

void QQmlCodeModel::newOpenFile(const QByteArray &uri, int version, const QString &docText)
{
    {
        QMutexLocker l(&m_mutex);
        auto &openDoc = m_openDocuments[uri];
        if (!openDoc.textDocument)
            openDoc.textDocument = std::make_shared<Utils::TextDocument>();
        QMutexLocker l2(openDoc.textDocument->mutex());
        openDoc.textDocument->setVersion(version);
        openDoc.textDocument->setPlainText(docText);
    }
    addOpenToUpdate(uri);
    openNeedUpdate();
}

OpenDocument QQmlCodeModel::openDocumentByUri(const QByteArray &uri)
{
    QMutexLocker l(&m_mutex);
    return m_openDocuments.value(uri);
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

DomItem QQmlCodeModel::validDocForUpdate(DomItem &item)
{
    if (item.field(Fields::isValid).value().toBool(false)) {
        if (auto envPtr = m_validEnv.ownerAs<DomEnvironment>()) {
            switch (item.fileObject().internalKind()) {
            case DomType::QmlFile:
                envPtr->addQmlFile(item.fileObject().ownerAs<QmlFile>());
                break;
            case DomType::JsFile:
                envPtr->addJsFile(item.fileObject().ownerAs<JsFile>());
                break;
            default:
                qCWarning(lspServerLog)
                        << "Unexpected file type " << item.fileObject().internalKindStr();
                return DomItem();
            }
            return m_validEnv.path(item.canonicalPath());
        }
    }
    return DomItem();
}

void QQmlCodeModel::newDocForOpenFile(const QByteArray &uri, int version, const QString &docText)
{
    qCDebug(codeModelLog) << "updating doc" << uri << "to version" << version << "("
                          << docText.length() << "chars)";
    DomItem newCurrent = m_currentEnv.makeCopy(DomItem::CopyOption::EnvConnected).item();
    QString fPath = uri2Path(uri, UriLookup::ForceLookup);
    Path p;
    newCurrent.loadFile(
            fPath, fPath, docText, QDateTime::currentDateTimeUtc(),
            [&p](Path, DomItem &, DomItem &newValue) { p = newValue.fileObject().canonicalPath(); },
            {});
    newCurrent.loadPendingDependencies();
    if (p) {
        newCurrent.commitToBase();
        DomItem item = m_currentEnv.path(p);
        DomItem vDoc = validDocForUpdate(item);
        {
            QMutexLocker l(&m_mutex);
            OpenDocument &doc = m_openDocuments[uri];
            if (!doc.textDocument) {
                qCWarning(lspServerLog)
                        << "ignoring update to closed document" << QString::fromUtf8(uri);
                return;
            } else {
                QMutexLocker l(doc.textDocument->mutex());
                if (doc.textDocument->version() && *doc.textDocument->version() > version) {
                    qCWarning(lspServerLog)
                            << "docUpdate: version" << version << "of document"
                            << QString::fromUtf8(uri) << "is not the latest anymore";
                    return;
                }
            }
            if (!doc.snapshot.docVersion || *doc.snapshot.docVersion < version) {
                doc.snapshot.docVersion = version;
                doc.snapshot.doc = item;
            } else {
                qCWarning(lspServerLog) << "skippig update of current doc to obsolete version"
                                        << version << "of document" << QString::fromUtf8(uri);
            }
            if (vDoc) {
                if (!doc.snapshot.validDocVersion || *doc.snapshot.validDocVersion < version) {
                    doc.snapshot.validDocVersion = version;
                    doc.snapshot.validDoc = vDoc;
                } else {
                    qCWarning(lspServerLog) << "skippig update of valid doc to obsolete version"
                                            << version << "of document" << QString::fromUtf8(uri);
                }
            } else {
                qCWarning(lspServerLog)
                        << "avoid update of validDoc to " << version << "of document"
                        << QString::fromUtf8(uri) << "as it is invalid";
            }
        }
    }
    if (codeModelLog().isDebugEnabled()) {
        qCDebug(codeModelLog) << "finished update doc of " << uri << "to version" << version;
        snapshotByUri(uri).dump(qDebug() << "postSnapshot",
                                OpenDocumentSnapshot::DumpOption::AllCode);
    }
    // we should update the scope in the future thus call addOpen(uri)
    emit updatedSnapshot(uri);
}

void QQmlCodeModel::closeOpenFile(const QByteArray &uri)
{
    QMutexLocker l(&m_mutex);
    m_openDocuments.remove(uri);
}

void QQmlCodeModel::openUpdate(const QByteArray &uri)
{
    bool updateDoc = false;
    bool updateScope = false;
    std::optional<int> rNow = 0;
    QString docText;
    DomItem validDoc;
    std::shared_ptr<Utils::TextDocument> document;
    {
        QMutexLocker l(&m_mutex);
        OpenDocument &doc = m_openDocuments[uri];
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
        newDocForOpenFile(uri, *rNow, docText);
    }
    if (updateScope) {
        // to do
    }
}

void QQmlCodeModel::addOpenToUpdate(const QByteArray &uri)
{
    QMutexLocker l(&m_mutex);
    m_openDocumentsToUpdate.insert(uri);
}

QDebug OpenDocumentSnapshot::dump(QDebug dbg, DumpOptions options)
{
    dbg.noquote().nospace() << "{";
    dbg << "  uri:" << QString::fromUtf8(uri) << "\n";
    dbg << "  docVersion:" << (docVersion ? QString::number(*docVersion) : u"*none*"_qs) << "\n";
    if (options & DumpOption::LatestCode) {
        dbg << "  doc: ------------\n"
            << doc.field(Fields::code).value().toString() << "\n==========\n";
    } else {
        dbg << u"  doc:"
            << (doc ? u"%1chars"_qs.arg(doc.field(Fields::code).value().toString().length())
                    : u"*none*"_qs)
            << "\n";
    }
    dbg << "  validDocVersion:"
        << (validDocVersion ? QString::number(*validDocVersion) : u"*none*"_qs) << "\n";
    if (options & DumpOption::ValidCode) {
        dbg << "  validDoc: ------------\n"
            << validDoc.field(Fields::code).value().toString() << "\n==========\n";
    } else {
        dbg << u"  validDoc:"
            << (validDoc ? u"%1chars"_qs.arg(
                        validDoc.field(Fields::code).value().toString().length())
                         : u"*none*"_qs)
            << "\n";
    }
    dbg << "  scopeVersion:" << (scopeVersion ? QString::number(*scopeVersion) : u"*none*"_qs)
        << "\n";
    dbg << "  scopeDependenciesLoadTime:" << scopeDependenciesLoadTime << "\n";
    dbg << "  scopeDependenciesChanged" << scopeDependenciesChanged << "\n";
    dbg << "}";
    return dbg;
}

} // namespace QmlLsp

QT_END_NAMESPACE
