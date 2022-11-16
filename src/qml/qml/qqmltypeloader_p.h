// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTYPELOADER_P_H
#define QQMLTYPELOADER_P_H

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

#include <private/qqmldatablob_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlmetatype_p.h>

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmlerror.h>

#include <QtCore/qcache.h>
#include <QtCore/qmutex.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQmlScriptBlob;
class QQmlQmldirData;
class QQmlTypeData;
class QQmlEngineExtensionInterface;
class QQmlExtensionInterface;
class QQmlProfiler;
class QQmlTypeLoaderThread;
class QQmlEngine;

class Q_QML_PRIVATE_EXPORT QQmlTypeLoader
{
    Q_DECLARE_TR_FUNCTIONS(QQmlTypeLoader)
public:
    using ChecksumCache = QHash<quintptr, QByteArray>;
    enum Mode { PreferSynchronous, Asynchronous, Synchronous };

    class Q_QML_PRIVATE_EXPORT Blob : public QQmlDataBlob
    {
    public:
        Blob(const QUrl &url, QQmlDataBlob::Type type, QQmlTypeLoader *loader);
        ~Blob() override;

        const QQmlImports *imports() const { return m_importCache.data(); }

        void setCachedUnitStatus(QQmlMetaType::CachedUnitLookupError status) { m_cachedUnitStatus = status; }

        struct PendingImport
        {
            QString uri;
            QString qualifier;

            QV4::CompiledData::Import::ImportType type
                = QV4::CompiledData::Import::ImportType::ImportLibrary;
            QV4::CompiledData::Location location;

            QQmlImports::ImportFlags flags;
            quint8 precedence = 0;
            int priority = 0;

            QTypeRevision version;

            PendingImport() = default;
            PendingImport(Blob *blob, const QV4::CompiledData::Import *import,
                          QQmlImports::ImportFlags flags);
        };
        using PendingImportPtr = std::shared_ptr<PendingImport>;

        void importQmldirScripts(const PendingImportPtr &import, const QQmlTypeLoaderQmldirContent &qmldir, const QUrl &qmldirUrl);

    protected:
        bool addImport(const QV4::CompiledData::Import *import, QQmlImports::ImportFlags,
                       QList<QQmlError> *errors);
        bool addImport(PendingImportPtr import, QList<QQmlError> *errors);

        bool fetchQmldir(const QUrl &url, PendingImportPtr import, int priority, QList<QQmlError> *errors);
        bool updateQmldir(const QQmlRefPointer<QQmlQmldirData> &data, const PendingImportPtr &import, QList<QQmlError> *errors);

    private:
        bool addScriptImport(const PendingImportPtr &import);
        bool addFileImport(const PendingImportPtr &import, QList<QQmlError> *errors);
        bool addLibraryImport(const PendingImportPtr &import, QList<QQmlError> *errors);

        virtual bool qmldirDataAvailable(const QQmlRefPointer<QQmlQmldirData> &, QList<QQmlError> *);

        virtual void scriptImported(const QQmlRefPointer<QQmlScriptBlob> &, const QV4::CompiledData::Location &, const QString &, const QString &) {}

        void dependencyComplete(QQmlDataBlob *) override;

        bool loadImportDependencies(
                const PendingImportPtr &currentImport, const QString &qmldirUri,
                QQmlImports::ImportFlags flags, QList<QQmlError> *errors);

    protected:
        bool loadDependentImports(
                const QList<QQmlDirParser::Import> &imports, const QString &qualifier,
                QTypeRevision version, quint16 precedence, QQmlImports::ImportFlags flags,
                QList<QQmlError> *errors);
        virtual QString stringAt(int) const { return QString(); }

        bool isDebugging() const;
        bool readCacheFile() const;
        bool writeCacheFile() const;
        QQmlMetaType::CacheMode aotCacheMode() const;

        QQmlRefPointer<QQmlImports> m_importCache;
        QVector<PendingImportPtr> m_unresolvedImports;
        QVector<QQmlRefPointer<QQmlQmldirData>> m_qmldirs;
        QQmlMetaType::CachedUnitLookupError m_cachedUnitStatus = QQmlMetaType::CachedUnitLookupError::NoError;
    };

    QQmlTypeLoader(QQmlEngine *);
    ~QQmlTypeLoader();

    QQmlImportDatabase *importDatabase() const;
    ChecksumCache *checksumCache() { return &m_checksumCache; }
    const ChecksumCache *checksumCache() const { return &m_checksumCache; }

    static QUrl normalize(const QUrl &unNormalizedUrl);

    QQmlRefPointer<QQmlTypeData> getType(const QUrl &unNormalizedUrl, Mode mode = PreferSynchronous);
    QQmlRefPointer<QQmlTypeData> getType(const QByteArray &, const QUrl &url, Mode mode = PreferSynchronous);

    QQmlRefPointer<QQmlScriptBlob> getScript(const QUrl &unNormalizedUrl);
    QQmlRefPointer<QQmlQmldirData> getQmldir(const QUrl &);

    QString absoluteFilePath(const QString &path);
    bool fileExists(const QString &path, const QString &file);
    bool directoryExists(const QString &path);

    const QQmlTypeLoaderQmldirContent qmldirContent(const QString &filePath);
    void setQmldirContent(const QString &filePath, const QString &content);

    void clearCache();
    void trimCache();

    bool isTypeLoaded(const QUrl &url) const;
    bool isScriptLoaded(const QUrl &url) const;

    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }

    void load(QQmlDataBlob *, Mode = PreferSynchronous);
    void loadWithStaticData(QQmlDataBlob *, const QByteArray &, Mode = PreferSynchronous);
    void loadWithCachedUnit(QQmlDataBlob *blob, const QQmlPrivate::CachedQmlUnit *unit, Mode mode = PreferSynchronous);

    QQmlEngine *engine() const;
    void initializeEngine(QQmlEngineExtensionInterface *, const char *);
    void initializeEngine(QQmlExtensionInterface *, const char *);
    void invalidate();

#if !QT_CONFIG(qml_debug)
    quintptr profiler() const { return 0; }
    void setProfiler(quintptr) {}
#else
    QQmlProfiler *profiler() const { return m_profiler.data(); }
    void setProfiler(QQmlProfiler *profiler);
#endif // QT_CONFIG(qml_debug)


private:
    friend class QQmlDataBlob;
    friend class QQmlTypeLoaderThread;
#if QT_CONFIG(qml_network)
    friend class QQmlTypeLoaderNetworkReplyProxy;
#endif // qml_network

    void shutdownThread();

    void loadThread(const QQmlDataBlob::Ptr &);
    void loadWithStaticDataThread(const QQmlDataBlob::Ptr &, const QByteArray &);
    void loadWithCachedUnitThread(const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit);
#if QT_CONFIG(qml_network)
    void networkReplyFinished(QNetworkReply *);
    void networkReplyProgress(QNetworkReply *, qint64, qint64);

    typedef QHash<QNetworkReply *, QQmlDataBlob::Ptr> NetworkReplies;
#endif

    void setData(const QQmlDataBlob::Ptr &, const QByteArray &);
    void setData(const QQmlDataBlob::Ptr &, const QString &fileName);
    void setData(const QQmlDataBlob::Ptr &, const QQmlDataBlob::SourceCodeData &);
    void setCachedUnit(const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit);

    typedef QHash<QUrl, QQmlTypeData *> TypeCache;
    typedef QHash<QUrl, QQmlScriptBlob *> ScriptCache;
    typedef QHash<QUrl, QQmlQmldirData *> QmldirCache;
    typedef QCache<QString, QCache<QString, bool> > ImportDirCache;
    typedef QStringHash<QQmlTypeLoaderQmldirContent *> ImportQmlDirCache;

    QQmlEngine *m_engine;
    QQmlTypeLoaderThread *m_thread;
    QMutex &m_mutex;

#if QT_CONFIG(qml_debug)
    QScopedPointer<QQmlProfiler> m_profiler;
#endif

#if QT_CONFIG(qml_network)
    NetworkReplies m_networkReplies;
#endif
    TypeCache m_typeCache;
    int m_typeCacheTrimThreshold;
    ScriptCache m_scriptCache;
    QmldirCache m_qmldirCache;
    ImportDirCache m_importDirCache;
    ImportQmlDirCache m_importQmlDirCache;
    ChecksumCache m_checksumCache;

    template<typename Loader>
    void doLoad(const Loader &loader, QQmlDataBlob *blob, Mode mode);
    void updateTypeCacheTrimThreshold();

    friend struct PlainLoader;
    friend struct CachedLoader;
    friend struct StaticLoader;
};

QT_END_NAMESPACE

#endif // QQMLTYPELOADER_P_H
