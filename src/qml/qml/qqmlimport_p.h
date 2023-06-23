// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLIMPORT_P_H
#define QQMLIMPORT_P_H

#include <QtCore/qurl.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qset.h>
#include <QtCore/qstringlist.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlerror.h>
#include <QtQml/qqmlfile.h>
#include <private/qqmldirparser_p.h>
#include <private/qqmltype_p.h>
#include <private/qstringhash_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qfieldlist_p.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QQmlTypeNameCache;
class QQmlEngine;
class QDir;
class QQmlImportNamespace;
class QQmlImportDatabase;
class QQmlTypeLoader;
class QQmlTypeLoaderQmldirContent;
class QTypeRevision;

const QLoggingCategory &lcQmlImport();

namespace QQmlImport {
    enum RecursionRestriction { PreventRecursion, AllowRecursion };
}

struct QQmlImportInstance
{
    enum Precedence {
        Lowest = std::numeric_limits<quint8>::max(),
        Implicit = Lowest / 2,
        Highest = 0,
    };

    QString uri; // e.g. QtQuick
    QString url; // the base path of the import
    QTypeRevision version; // the version imported

    bool isLibrary; // true means that this is not a file import

    // not covered by precedence. You can set a component as implicitly imported after the fact.
    bool implicitlyImported = false;
    bool isInlineComponent = false;

    quint8 precedence = 0;

    QQmlDirComponents qmlDirComponents; // a copy of the components listed in the qmldir
    QQmlDirScripts qmlDirScripts; // a copy of the scripts in the qmldir

    bool setQmldirContent(const QString &resolvedUrl, const QQmlTypeLoaderQmldirContent &qmldir,
                          QQmlImportNamespace *nameSpace, QList<QQmlError> *errors);

    static QQmlDirScripts getVersionedScripts(const QQmlDirScripts &qmldirscripts,
                                              QTypeRevision version);

    bool resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef &type,
                     QTypeRevision *version_return, QQmlType* type_return,
                     const QString *base = nullptr, bool *typeRecursionDetected = nullptr,
                     QQmlType::RegistrationType = QQmlType::AnyRegistrationType,
                     QQmlImport::RecursionRestriction recursionRestriction = QQmlImport::PreventRecursion,
                     QList<QQmlError> *errors = nullptr) const;
};

class QQmlImportNamespace
{
public:
    QQmlImportNamespace() : nextNamespace(nullptr) {}
    ~QQmlImportNamespace() { qDeleteAll(imports); }

    QList<QQmlImportInstance *> imports;

    QQmlImportInstance *findImport(const QString &uri) const;

    bool resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef& type,
                     QTypeRevision *version_return, QQmlType* type_return,
                     const QString *base = nullptr, QList<QQmlError> *errors = nullptr,
                     QQmlType::RegistrationType registrationType = QQmlType::AnyRegistrationType,
                     bool *typeRecursionDeteced = nullptr);

    // Prefix when used as a qualified import.  Otherwise empty.
    QHashedString prefix;

    // Used by QQmlImports::m_qualifiedSets
    // set to this in unqualifiedSet to indicate that the lists of imports needs
    // to be sorted when an inline component import was added
    // We can't use flag pointer, as that does not work with QFieldList
    QQmlImportNamespace *nextNamespace = nullptr;
    bool needsSorting() const { return nextNamespace == this; }
    void setNeedsSorting(bool needsSorting)
    {
        Q_ASSERT(nextNamespace == this || nextNamespace == nullptr);
        nextNamespace = needsSorting ? this : nullptr;
    }
};

class Q_QML_PRIVATE_EXPORT QQmlImports final : public QQmlRefCounted<QQmlImports>
{
    Q_DISABLE_COPY_MOVE(QQmlImports)
public:
    enum ImportVersion { FullyVersioned, PartiallyVersioned, Unversioned };

    enum ImportFlag : quint8 {
        ImportNoFlag        = 0x0,
        ImportIncomplete    = 0x1,
    };
    Q_DECLARE_FLAGS(ImportFlags, ImportFlag)

    QQmlImports(QQmlTypeLoader *loader) : m_typeLoader(loader) {}
    ~QQmlImports()
    {
        while (QQmlImportNamespace *ns = m_qualifiedSets.takeFirst())
            delete ns;
    }

    void setBaseUrl(const QUrl &url, const QString &urlString = QString());
    QUrl baseUrl() const { return m_baseUrl; }

    bool resolveType(
            const QHashedStringRef &type, QQmlType *type_return, QTypeRevision *version_return,
            QQmlImportNamespace **ns_return, QList<QQmlError> *errors = nullptr,
            QQmlType::RegistrationType registrationType = QQmlType::AnyRegistrationType,
            bool *typeRecursionDetected = nullptr) const;

    QTypeRevision addImplicitImport(
            QQmlImportDatabase *importDb, QString *localQmldir, QList<QQmlError> *errors)
    {
        Q_ASSERT(errors);
        qCDebug(lcQmlImport) << "addImplicitImport:" << qPrintable(baseUrl().toString());

        const ImportFlags flags =
                ImportFlags(!isLocal(baseUrl()) ? ImportIncomplete : ImportNoFlag);
        return addFileImport(
                    importDb, QLatin1String("."), QString(), QTypeRevision(),
                    flags, QQmlImportInstance::Implicit, localQmldir, errors);
    }

    bool addInlineComponentImport(
            QQmlImportInstance  *const importInstance, const QString &name, const QUrl importUrl);

    QTypeRevision addFileImport(
            QQmlImportDatabase *importDb, const QString &uri, const QString &prefix,
            QTypeRevision version, ImportFlags flags, quint16 precedence, QString *localQmldir,
            QList<QQmlError> *errors);

    QTypeRevision addLibraryImport(
            QQmlImportDatabase *importDb, const QString &uri, const QString &prefix,
            QTypeRevision version, const QString &qmldirIdentifier, const QString &qmldirUrl,
            ImportFlags flags, quint16 precedence, QList<QQmlError> *errors);

    QTypeRevision updateQmldirContent(
            QQmlImportDatabase *importDb, const QString &uri, const QString &prefix,
            const QString &qmldirIdentifier, const QString &qmldirUrl, QList<QQmlError> *errors);

    void populateCache(QQmlTypeNameCache *cache) const;

    struct ScriptReference
    {
        QString nameSpace;
        QString qualifier;
        QUrl location;
    };

    QList<ScriptReference> resolvedScripts() const;

    struct CompositeSingletonReference
    {
        QString typeName;
        QString prefix;
        QTypeRevision version;
    };

    QList<CompositeSingletonReference> resolvedCompositeSingletons() const;

    static QStringList completeQmldirPaths(
            const QString &uri, const QStringList &basePaths, QTypeRevision version);

    static QString versionString(QTypeRevision version, ImportVersion importVersion);

    static bool isLocal(const QString &url)
    {
        return !QQmlFile::urlToLocalFileOrQrc(url).isEmpty();
    }

    static bool isLocal(const QUrl &url)
    {
        return !QQmlFile::urlToLocalFileOrQrc(url).isEmpty();
    }

    static QUrl urlFromLocalFileOrQrcOrUrl(const QString &);

    static void setDesignerSupportRequired(bool b);

    static QTypeRevision validVersion(QTypeRevision version = QTypeRevision());

private:
    friend class QQmlImportDatabase;

    QQmlImportNamespace *importNamespace(const QString &prefix);

    bool resolveType(
            const QHashedStringRef &type, QTypeRevision *version_return, QQmlType *type_return,
            QList<QQmlError> *errors, QQmlType::RegistrationType registrationType,
            bool *typeRecursionDetected = nullptr) const;

    QQmlImportNamespace *findQualifiedNamespace(const QHashedStringRef &) const;

    static QTypeRevision matchingQmldirVersion(
            const QQmlTypeLoaderQmldirContent &qmldir, const QString &uri,
            QTypeRevision version, QList<QQmlError> *errors);

    QTypeRevision importExtension(
            const QString &uri, QTypeRevision version, QQmlImportDatabase *database,
            const QQmlTypeLoaderQmldirContent *qmldir, QList<QQmlError> *errors);

    bool getQmldirContent(
            const QString &qmldirIdentifier, const QString &uri, QQmlTypeLoaderQmldirContent *qmldir,
            QList<QQmlError> *errors);

    QString resolvedUri(const QString &dir_arg, QQmlImportDatabase *database);

    QQmlImportInstance *addImportToNamespace(
            QQmlImportNamespace *nameSpace, const QString &uri, const QString &url,
            QTypeRevision version, QV4::CompiledData::Import::ImportType type,
            QList<QQmlError> *errors, quint16 precedence);

    QUrl m_baseUrl;
    QString m_base;

    // storage of data related to imports without a namespace
    // TODO: This needs to be mutable because QQmlImportNamespace likes to sort itself on
    //       resolveType(). Therefore, QQmlImportNamespace::resolveType() is not const.
    //       There should be a better way to do this.
    mutable QQmlImportNamespace m_unqualifiedset;

    // storage of data related to imports with a namespace
    QFieldList<QQmlImportNamespace, &QQmlImportNamespace::nextNamespace> m_qualifiedSets;

    QQmlTypeLoader *m_typeLoader = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlImports::ImportFlags)

class Q_QML_PRIVATE_EXPORT QQmlImportDatabase
{
    Q_DECLARE_TR_FUNCTIONS(QQmlImportDatabase)
public:
    enum PathType { Local, Remote, LocalOrRemote };

    enum LocalQmldirSearchLocation {
        QmldirFileAndCache,
        QmldirCacheOnly,
    };

    enum LocalQmldirResult {
        QmldirFound,
        QmldirNotFound,
        QmldirInterceptedToRemote,
        QmldirRejected
    };

    QQmlImportDatabase(QQmlEngine *);
    ~QQmlImportDatabase() { clearDirCache(); }

    bool removeDynamicPlugin(const QString &pluginId);
    QStringList dynamicPlugins() const;

    QStringList importPathList(PathType type = LocalOrRemote) const;
    void setImportPathList(const QStringList &paths);
    void addImportPath(const QString& dir);

    QStringList pluginPathList() const { return filePluginPath; }
    void setPluginPathList(const QStringList &paths);

    void addPluginPath(const QString& path);

    template<typename Callback>
    LocalQmldirResult locateLocalQmldir(
            const QString &uri, QTypeRevision version, LocalQmldirSearchLocation location,
            const Callback &callback);

    static QTypeRevision lockModule(const QString &uri, const QString &typeNamespace,
                                    QTypeRevision version, QList<QQmlError> *errors);

private:
    friend class QQmlImports;
    friend class QQmlPluginImporter;

    QString absoluteFilePath(const QString &path) const;
    void clearDirCache();

    struct QmldirCache {
        QTypeRevision version;
        QString qmldirFilePath;
        QString qmldirPathUrl;
        QmldirCache *next;
    };
    // Maps from an import to a linked list of qmldir info.
    // Used in QQmlImports::locateQmldir()
    QStringHash<QmldirCache *> qmldirCache;

    // XXX thread
    QStringList filePluginPath;
    QStringList fileImportPath;

    QSet<QString> modulesForWhichPluginsHaveBeenLoaded;
    QSet<QString> initializedPlugins;
    QQmlEngine *engine;
};

template<typename Callback>
QQmlImportDatabase::LocalQmldirResult QQmlImportDatabase::locateLocalQmldir(
        const QString &uri, QTypeRevision version,
        QQmlImportDatabase::LocalQmldirSearchLocation location, const Callback &callback)
{
    // Check cache first

    LocalQmldirResult result = QmldirNotFound;
    QmldirCache *cacheTail = nullptr;

    QmldirCache **cachePtr = qmldirCache.value(uri);
    QmldirCache *cacheHead = cachePtr ? *cachePtr : nullptr;
    if (cacheHead) {
        cacheTail = cacheHead;
        do {
            if (cacheTail->version == version) {
                if (cacheTail->qmldirFilePath.isEmpty()) {
                    return cacheTail->qmldirPathUrl.isEmpty()
                            ? QmldirNotFound
                            : QmldirInterceptedToRemote;
                }
                if (callback(cacheTail->qmldirFilePath, cacheTail->qmldirPathUrl))
                    return QmldirFound;
                result = QmldirRejected;
            }
        } while (cacheTail->next && (cacheTail = cacheTail->next));
    }


    // Do not try to construct the cache if it already had any entries for the URI.
    // Otherwise we might duplicate cache entries.
    if (location == QmldirCacheOnly || result != QmldirNotFound)
        return result;

    const bool hasInterceptors = !engine->urlInterceptors().isEmpty();

    // Interceptor might redirect remote files to local ones.
    QStringList localImportPaths = importPathList(hasInterceptors ? LocalOrRemote : Local);

    // Search local import paths for a matching version
    const QStringList qmlDirPaths = QQmlImports::completeQmldirPaths(
                uri, localImportPaths, version);

    QString qmldirAbsoluteFilePath;
    for (QString qmldirPath : qmlDirPaths) {
        if (hasInterceptors) {
            const QUrl intercepted = engine->interceptUrl(
                        QQmlImports::urlFromLocalFileOrQrcOrUrl(qmldirPath),
                        QQmlAbstractUrlInterceptor::QmldirFile);
            qmldirPath = QQmlFile::urlToLocalFileOrQrc(intercepted);
            if (result != QmldirInterceptedToRemote
                    && qmldirPath.isEmpty()
                    && !QQmlFile::isLocalFile(intercepted)) {
                result = QmldirInterceptedToRemote;
            }
        }

        qmldirAbsoluteFilePath = absoluteFilePath(qmldirPath);
        if (!qmldirAbsoluteFilePath.isEmpty()) {
            QString url;
            const QString absolutePath = qmldirAbsoluteFilePath.left(
                        qmldirAbsoluteFilePath.lastIndexOf(u'/') + 1);
            if (absolutePath.at(0) == u':') {
                url = QStringLiteral("qrc") + absolutePath;
            } else {
                url = QUrl::fromLocalFile(absolutePath).toString();
                // This handles the UNC path case as when the path is retrieved from the QUrl it
                // will convert the host name from upper case to lower case. So the absoluteFilePath
                // is changed at this point to make sure it will match later on in that case.
                if (qmldirAbsoluteFilePath.startsWith(QStringLiteral("//"))) {
                    qmldirAbsoluteFilePath = QUrl::fromLocalFile(qmldirAbsoluteFilePath)
                            .toString(QUrl::RemoveScheme);
                }
            }

            QmldirCache *cache = new QmldirCache;
            cache->version = version;
            cache->qmldirFilePath = qmldirAbsoluteFilePath;
            cache->qmldirPathUrl = url;
            cache->next = nullptr;
            if (cacheTail)
                cacheTail->next = cache;
            else
                qmldirCache.insert(uri, cache);
            cacheTail = cache;

            if (result != QmldirFound)
                result = callback(qmldirAbsoluteFilePath, url) ? QmldirFound : QmldirRejected;

            // Do not return here. Rather, construct the complete cache for this URI.
        }
    }

    // Nothing found? Add an empty cache entry to signal that for further requests.
    if (result == QmldirNotFound || result == QmldirInterceptedToRemote) {
        QmldirCache *cache = new QmldirCache;
        cache->version = version;
        cache->next = cacheHead;
        if (result == QmldirInterceptedToRemote) {
            // The actual value doesn't matter as long as it's not empty.
            // We only use it to discern QmldirInterceptedToRemote from QmldirNotFound above.
            cache->qmldirPathUrl = QStringLiteral("intercepted");
        }
        qmldirCache.insert(uri, cache);

        if (result == QmldirNotFound) {
            qCDebug(lcQmlImport)
                    << "locateLocalQmldir:" << qPrintable(uri) << "module's qmldir file not found";
        }
    } else {
        qCDebug(lcQmlImport)
                << "locateLocalQmldir:" << qPrintable(uri) << "module's qmldir found at"
                << qmldirAbsoluteFilePath;
    }

    return result;
}

void qmlClearEnginePlugins();// For internal use by qmlClearRegisteredProperties

QT_END_NAMESPACE

#endif // QQMLIMPORT_P_H

