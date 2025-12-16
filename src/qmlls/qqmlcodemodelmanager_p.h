// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQmlCodeModelManager_P_H
#define QQmlCodeModelManager_P_H

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

#include "qqmlcodemodel_p.h"

#include <QObject>
#include <QHash>
#include <QtCore/qfilesystemwatcher.h>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>

#include <memory>

QT_BEGIN_NAMESPACE
class TextSynchronization;
namespace QmlLsp {

struct QQmlWorkspace
{
    QByteArray url;
    std::unique_ptr<QQmlCodeModel> codeModel;
    bool managedByClient = false;
    bool toBeClosed = false;
};

class QQmlCodeModelManager : public QObject
{
    Q_OBJECT
public:
    QQmlCodeModelManager(QObject *parent = nullptr, QQmlToolingSharedSettings *settings = nullptr);
    ~QQmlCodeModelManager();
    void prepareForShutdown();

    OpenDocumentSnapshot snapshotByUrl(const QByteArray &url);
    OpenDocument openDocumentByUrl(const QByteArray &url);

    void addOpenToUpdate(const QByteArray &);
    void removeDirectory(const QByteArray &);

    void newOpenFile(const QByteArray &url, int version, const QString &docText);
    void closeOpenFile(const QByteArray &url);
    QList<QByteArray> rootUrls() const;
    void addRootUrls(const QList<QByteArray> &urls);
    QStringList buildPathsForFileUrl(const QByteArray &url);
    QStringList resourceFilesForFileUrl(const QByteArray &url);
    void setBuildPathsForRootUrl(const QByteArray &url, const QStringList &paths);
    QStringList importPathsForUrl(const QByteArray &);
    void setImportPaths(const QStringList &paths);
    void removeRootUrls(const QList<QByteArray> &urls);
    QQmlToolingSharedSettings *settings() const { return m_settings; }
    void disableCMakeCalls();
    const QFactoryLoader &pluginLoader() const { return m_pluginLoader; }
    QByteArray shortestRootUrlForFile(const QByteArray &fileUrl) const;

    RegisteredSemanticTokens &registeredTokens(const QByteArray &);
    const RegisteredSemanticTokens &registeredTokens(const QByteArray &) const;
    void setDocumentationRootPath(const QString &path);
    HelpManager *helpManagerForUrl(const QByteArray &);

    void tryEnableCMakeCalls();
    void setCMakeJobs(int jobs);
    int defaultCMakeJobs() const { return fallbackCodeModel()->cmakeJobs(); }

    void setVerbose(bool verbose);
    QStringList defaultResourceFiles() const { return fallbackCodeModel()->resourceFiles(); }
    QStringList defaultImportPaths() const { return fallbackCodeModel()->importPaths(); }
    QStringList defaultBuildPaths() const { return fallbackCodeModel()->buildPaths(); }
    QString defaultDocumentationRootPath() const
    {
        return fallbackCodeModel()->documentationRootPath();
    }
public slots:
    void onCMakeProberFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onBuildFinished(const QByteArray &rootUrl);

private:
    enum SetBuildPathOption { AppendPathsFromFallback, DontAppendPathsFromFallback };
    void setBuildPathsOn(const QQmlWorkspace *ws, const QStringList &buildFolder,
                         SetBuildPathOption option);

protected:
    using Workspaces = std::vector<QQmlWorkspace>;
    using WorkspaceIterator = Workspaces::const_iterator;
    enum CMakeStatus { HasCMake, DoesNotHaveCMake, IsProbingCMake };

    QQmlCodeModel *findCodeModelForFile(const QByteArray &url);
    QQmlCodeModel *fallbackCodeModel() const { return m_workspaces.front().codeModel.get(); }
    WorkspaceIterator findWorkspaceForFile(const QByteArray &url);
    WorkspaceIterator workspaceFromBuildFolder(const QString &fileName,
                                               const QStringList &buildFolders);
    WorkspaceIterator fallbackWorkspace() const { return m_workspaces.begin(); }
    WorkspaceIterator beginNonFallbackWorkspace() const { return ++m_workspaces.begin(); }
    WorkspaceIterator endNonFallbackWorkspace() const { return m_workspaces.end(); }

    enum ManagedBy { ManagedByClient, ManagedByServer };
    using WorkspaceMutableIterator = Workspaces::iterator;
    WorkspaceMutableIterator findWorkspace(const QByteArray &url);
    void appendWorkspace(const QByteArray &url, ManagedBy managedBy);

    QQmlToolingSharedSettings *m_settings;
    QFactoryLoader m_pluginLoader;

    Workspaces m_workspaces;
    QQmllsBuildInformation m_buildInformation;
    QProcessScheduler m_processScheduler;

    std::map<QByteArray, QByteArray> m_file2CodeModel;

    QProcess m_cmakeProber;
    CMakeStatus m_cmakeStatus = IsProbingCMake;
    bool m_verbose = false;

Q_SIGNALS:
    void updatedSnapshot(const QByteArray &url, UpdatePolicy policy);
};

} // namespace QmlLsp
QT_END_NAMESPACE

#endif // QQmlCodeModelManager_P_H
