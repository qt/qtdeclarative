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
#ifndef QQMLCODEMODEL_H
#define QQMLCODEMODEL_H

#include <QObject>
#include <QHash>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include "qlanguageserver_p.h"
#include "textdocument.h"

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
    QByteArray uri;
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
    enum class UriLookup { Caching, ForceLookup };

    explicit QQmlCodeModel(QObject *parent = nullptr);
    QQmlJS::Dom::DomItem currentEnv();
    QQmlJS::Dom::DomItem validEnv();
    OpenDocumentSnapshot snapshotByUri(const QByteArray &uri);
    OpenDocument openDocumentByUri(const QByteArray &uri);

    void openNeedUpdate();
    void indexNeedsUpdate();
    void addDirectoriesToIndex(const QStringList &paths, QLanguageServer *server);
    void addOpenToUpdate(const QByteArray &);
    void removeDirectory(const QString &path);
    // void updateDocument(const OpenDocument &doc);
    QString uri2Path(const QByteArray &uri, UriLookup options = UriLookup::Caching);
    void newOpenFile(const QByteArray &uri, int version, const QString &docText);
    void newDocForOpenFile(const QByteArray &uri, int version, const QString &docText);
    void closeOpenFile(const QByteArray &uri);
signals:
    void updatedSnapshot(const QByteArray &uri);
private:
    QQmlJS::Dom::DomItem validDocForUpdate(QQmlJS::Dom::DomItem &item);
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
    QHash<QByteArray, QString> m_uri2path;
    QHash<QString, QByteArray> m_path2uri;
    QHash<QByteArray, OpenDocument> m_openDocuments;
};

} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QQMLCODEMODEL_H
