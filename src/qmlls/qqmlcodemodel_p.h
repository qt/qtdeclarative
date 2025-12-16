// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLCODEMODEL_P_H
#define QQMLCODEMODEL_P_H

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

#include "qlanguageserver_p.h"
#include "qtextdocument_p.h"
#include "qprocessscheduler_p.h"
#include "qqmllshelputils_p.h"
#include "qqmlsemantictokens_p.h"

#include <QObject>
#include <QHash>
#include <QtCore/qfilesystemwatcher.h>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
class TextSynchronization;
namespace QmlLsp {

class OpenDocumentSnapshot
{
public:
    enum class DumpOption {
        NoCode = 0,
        LatestCode = 0x1,
        ValidCode = 0x2,
        AllCode = LatestCode | ValidCode
    };
    Q_DECLARE_FLAGS(DumpOptions, DumpOption)
    QStringList searchPath;
    QByteArray url;
    std::optional<int> docVersion;
    QQmlJS::Dom::DomItem doc;
    std::optional<int> validDocVersion;
    QQmlJS::Dom::DomItem validDoc;
    QDateTime scopeDependenciesLoadTime;
    bool scopeDependenciesChanged = false;
    QQmlJSScope::ConstPtr scope;
    QDebug dump(QDebug dbg, DumpOptions dump = DumpOption::NoCode);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(OpenDocumentSnapshot::DumpOptions)

class OpenDocument
{
public:
    OpenDocumentSnapshot snapshot;
    std::shared_ptr<Utils::TextDocument> textDocument;
};

struct RegisteredSemanticTokens
{
    QByteArray resultId = "0";
    QmlHighlighting::HighlightsContainer highlights;
};

struct ModuleSetting
{
    QString sourceFolder;
    QStringList importPaths;
    QStringList resourceFiles;
};

enum UpdatePolicy { NormalUpdate, ForceUpdate };

using ModuleSettings = QList<ModuleSetting>;
class QQmllsBuildInformation
{
public:
    QQmllsBuildInformation();
    void loadSettingsFrom(const QStringList &buildPaths, UpdatePolicy policy = NormalUpdate);
    QStringList importPathsFor(const QString &filePath);
    QStringList resourceFilesFor(const QString &filePath);
    ModuleSetting settingFor(const QString &filePath);

private:
    QString m_docDir;
    ModuleSettings m_moduleSettings;
    QSet<QString> m_seenSettings;
};

class QQmlCodeModel : public QObject
{
    Q_OBJECT
public:
    enum class UrlLookup { Caching, ForceLookup };
    enum class State { Running, Stopping };

    static constexpr QLatin1String s_maxCMakeJobs = "max"_L1;
    static constexpr int s_defaultCMakeJobs = 1;

    explicit QQmlCodeModel(const QByteArray &rootUrl = {}, QObject *parent = nullptr,
                           QQmlToolingSharedSettings *settings = nullptr);
    ~QQmlCodeModel();
    void prepareForShutdown();

    QQmlJS::Dom::DomItem currentEnv() const { return m_currentEnv; };
    QQmlJS::Dom::DomItem validEnv() const { return m_validEnv; };
    OpenDocumentSnapshot snapshotByUrl(const QByteArray &url);
    OpenDocument openDocumentByUrl(const QByteArray &url);
    bool isEmpty() const;

    void addOpenToUpdate(const QByteArray &, UpdatePolicy policy);
    void removeDirectory(const QByteArray &);
    // void updateDocument(const OpenDocument &doc);
    void newOpenFile(const QByteArray &url, int version, const QString &docText);
    void closeOpenFile(const QByteArray &url);
    QByteArray rootUrl() const;
    QStringList buildPaths();
    QStringList buildPathsForFileUrl(const QByteArray &url);
    void setBuildPaths(const QStringList &paths);
    QStringList buildPathsForOpenedFiles();
    QStringList importPathsForUrl(const QByteArray &);
    QStringList importPaths() const;
    void setImportPaths(const QStringList &paths);
    QStringList resourceFiles() const;
    void setResourceFiles(const QStringList &resourceFiles);
    QQmlToolingSharedSettings *settings() const { return m_settings; }
    QStringList findFilePathsFromFileNames(const QStringList &fileNames,
                                           const QSet<QString> &alreadyWatchedFiles);
    static QStringList fileNamesToWatch(const QQmlJS::Dom::DomItem &qmlFile);
    void disableCMakeCalls();
    void tryEnableCMakeCalls(QProcessScheduler *scheduler);

    RegisteredSemanticTokens &registeredTokens();
    const RegisteredSemanticTokens &registeredTokens() const;
    QString documentationRootPath() const
    {
        QMutexLocker l(&m_mutex);
        return m_documentationRootPath;
    }
    void setDocumentationRootPath(const QString &path);

    QSet<QString> ignoreForWatching() const
    {
        QMutexLocker guard(&m_mutex);
        return m_ignoreForWatching;
    }
    HelpManager *helpManager() { return &m_helpManager; }

    void setVerbose(bool verbose)
    {
        QMutexLocker guard(&m_mutex);
        m_verbose = verbose;
    }
    bool verbose() const
    {
        QMutexLocker guard(&m_mutex);
        return m_verbose;
    }
    void setCMakeJobs(int jobs) { m_cmakeJobs = jobs; }
    int cmakeJobs() const { return m_cmakeJobs; }

Q_SIGNALS:
    void updatedSnapshot(const QByteArray &url, UpdatePolicy policy);
    void documentationRootPathChanged(const QString &path);
    void openUpdateThreadFinished();

private:
    void addDirectory(const QString &path, int leftDepth);

    // only to be called from the openUpdateThread
    void newDocForOpenFile(const QByteArray &url, int version, const QString &docText,
                           UpdatePolicy policy);
    bool openUpdateSome();
    void openUpdate(const QByteArray &, UpdatePolicy policy);

    void openUpdateStart();
    void openUpdateEnd();
    void openNeedUpdate();
    QString url2Path(const QByteArray &url, UrlLookup options = UrlLookup::Caching);

    void callCMakeBuild(QProcessScheduler *scheduler);
    void onCMakeProcessFinished(const QByteArray &id);

    void addFileWatches(const QQmlJS::Dom::DomItem &qmlFile);

    enum CMakeStatus { RequiresInitialization, HasCMake, DoesNotHaveCMake };
    CMakeStatus cmakeStatus() const
    {
        QMutexLocker guard(&m_mutex);
        return m_cmakeStatus;
    }
    void setCMakeStatus(CMakeStatus status)
    {
        QMutexLocker guard(&m_mutex);
        m_cmakeStatus = status;
    }

    mutable QMutex m_mutex;
    const QByteArray m_rootUrl; // note: access without m_mutex, is const

    QQmlJS::Dom::DomItem m_currentEnv; // note: access without m_mutex, has thread-safe API
    QQmlJS::Dom::DomItem m_validEnv; // note: access without m_mutex, has thread-safe API
    QQmlToolingSharedSettings *m_settings; // note: access without m_mutex. has thread-safe API
    HelpManager m_helpManager; // note: access without m_mutex, has thread-safe API

    QThread* m_openUpdateThread = nullptr; // needed for asserts
    QHash<QByteArray, UpdatePolicy> m_openDocumentsToUpdate;
    QStringList m_buildPaths;
    QStringList m_importPaths;
    QStringList m_resourceFiles;
    QHash<QByteArray, QString> m_url2path;
    QHash<QString, QByteArray> m_path2url;
    QHash<QByteArray, OpenDocument> m_openDocuments;
    QFileSystemWatcher m_cppFileWatcher;
    RegisteredSemanticTokens m_tokens;
    QString m_documentationRootPath;
    QSet<QString> m_ignoreForWatching;
    int m_nUpdateInProgress = 0;
    CMakeStatus m_cmakeStatus = RequiresInitialization;
    int m_cmakeJobs = 1;
    State m_state = QQmlCodeModel::State::Running;
    bool m_verbose = false;
};

} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QQMLCODEMODEL_P_H
