// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <QObject>
#include <QHash>
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
    std::optional<int> scopeVersion;
    QDateTime scopeDependenciesLoadTime;
    bool scopeDependenciesChanged = false;
    QQmlJSScope::Ptr scope = {};
    QDebug dump(QDebug dbg, DumpOptions dump = DumpOption::NoCode);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(OpenDocumentSnapshot::DumpOptions)

class OpenDocument
{
public:
    OpenDocumentSnapshot snapshot;
    std::shared_ptr<Utils::TextDocument> textDocument;
};

struct ToIndex
{
    QString path;
    int leftDepth;
};

class QQmlCodeModel : public QObject
{
    Q_OBJECT
public:
    enum class UrlLookup { Caching, ForceLookup };
    enum class State { Running, Stopping };

    explicit QQmlCodeModel(QObject *parent = nullptr, QQmlToolingSettings *settings = nullptr);
    ~QQmlCodeModel();
    QQmlJS::Dom::DomItem currentEnv();
    QQmlJS::Dom::DomItem validEnv();
    OpenDocumentSnapshot snapshotByUrl(const QByteArray &url);
    OpenDocument openDocumentByUrl(const QByteArray &url);

    void openNeedUpdate();
    void indexNeedsUpdate();
    void addDirectoriesToIndex(const QStringList &paths, QLanguageServer *server);
    void addOpenToUpdate(const QByteArray &);
    void removeDirectory(const QString &path);
    // void updateDocument(const OpenDocument &doc);
    QString url2Path(const QByteArray &url, UrlLookup options = UrlLookup::Caching);
    void newOpenFile(const QByteArray &url, int version, const QString &docText);
    void newDocForOpenFile(const QByteArray &url, int version, const QString &docText);
    void closeOpenFile(const QByteArray &url);
    void setRootUrls(const QList<QByteArray> &urls);
    QList<QByteArray> rootUrls() const;
    void addRootUrls(const QList<QByteArray> &urls);
    QStringList buildPathsForRootUrl(const QByteArray &url);
    QStringList buildPathsForFileUrl(const QByteArray &url);
    void setBuildPathsForRootUrl(QByteArray url, const QStringList &paths);
    void removeRootUrls(const QList<QByteArray> &urls);
    QQmlToolingSettings *settings();
Q_SIGNALS:
    void updatedSnapshot(const QByteArray &url);
private:
    void indexDirectory(const QString &path, int depthLeft);
    int indexEvalProgress() const; // to be called in the mutex
    void indexStart(); // to be called in the mutex
    void indexEnd(); // to be called in the mutex
    void indexSendProgress(int progress);
    bool indexCancelled();
    bool indexSome();
    void addDirectory(const QString &path, int leftDepth);
    bool openUpdateSome();
    void openUpdateStart();
    void openUpdateEnd();
    void openUpdate(const QByteArray &);
    mutable QMutex m_mutex;
    State m_state = State::Running;
    int m_lastIndexProgress = 0;
    int m_nIndexInProgress = 0;
    QList<ToIndex> m_toIndex;
    int m_indexInProgressCost = 0;
    int m_indexDoneCost = 0;
    int m_nUpdateInProgress = 0;
    QQmlJS::Dom::DomItem m_currentEnv;
    QQmlJS::Dom::DomItem m_validEnv;
    QByteArray m_lastOpenDocumentUpdated;
    QSet<QByteArray> m_openDocumentsToUpdate;
    QHash<QByteArray, QStringList> m_buildPathsForRootUrl;
    QList<QByteArray> m_rootUrls;
    QHash<QByteArray, QString> m_url2path;
    QHash<QString, QByteArray> m_path2url;
    QHash<QByteArray, OpenDocument> m_openDocuments;
    QQmlToolingSettings *m_settings;
};

} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QQMLCODEMODEL_P_H
