/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLIMPORT_P_H
#define QQMLIMPORT_P_H

#include <QtCore/qurl.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qset.h>
#include <QtCore/qstringlist.h>
#include <private/qqmldirparser_p.h>
#include <private/qqmlscript_p.h>
#include <private/qqmlmetatype_p.h>

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
class QQmlImportsPrivate;
class QQmlImportDatabase;
class QQmlTypeLoader;

class Q_QML_PRIVATE_EXPORT QQmlImports
{
public:
    enum ImportVersion { FullyVersioned, PartiallyVersioned, Unversioned };

    QQmlImports(QQmlTypeLoader *);
    QQmlImports(const QQmlImports &);
    ~QQmlImports();
    QQmlImports &operator=(const QQmlImports &);

    void setBaseUrl(const QUrl &url, const QString &urlString = QString());
    QUrl baseUrl() const;

    bool resolveType(const QHashedStringRef &type,
                     QQmlType** type_return,
                     int *version_major, int *version_minor,
                     QQmlImportNamespace** ns_return,
                     QList<QQmlError> *errors = 0) const;
    bool resolveType(QQmlImportNamespace*,
                     const QHashedStringRef& type,
                     QQmlType** type_return, int *version_major, int *version_minor) const;

    bool addImplicitImport(QQmlImportDatabase *importDb, QList<QQmlError> *errors);

    bool addFileImport(QQmlImportDatabase *,
                       const QString& uri, const QString& prefix, int vmaj, int vmin, bool incomplete,
                       QList<QQmlError> *errors);

    bool addLibraryImport(QQmlImportDatabase *importDb,
                          const QString &uri, const QString &prefix, int vmaj, int vmin,
                          const QString &qmldirIdentifier, const QString &qmldirUrl, bool incomplete, QList<QQmlError> *errors);

    bool updateQmldirContent(QQmlImportDatabase *importDb,
                             const QString &uri, const QString &prefix,
                             const QString &qmldirIdentifier, const QString &qmldirUrl, QList<QQmlError> *errors);

    bool locateQmldir(QQmlImportDatabase *,
                      const QString &uri, int vmaj, int vmin,
                      QString *qmldirFilePath, QString *url);

    void populateCache(QQmlTypeNameCache *cache) const;

    struct ScriptReference
    {
        QString nameSpace;
        QString qualifier;
        QUrl location;
    };

    QList<ScriptReference> resolvedScripts() const;

    static QString completeQmldirPath(const QString &uri, const QString &base, int vmaj, int vmin,
                                      QQmlImports::ImportVersion version);

    static bool isLocal(const QString &url);
    static bool isLocal(const QUrl &url);

private:
    friend class QQmlImportDatabase;
    QQmlImportsPrivate *d;
};

class QQmlImportDatabase
{
    Q_DECLARE_TR_FUNCTIONS(QQmlImportDatabase)
public:
    enum PathType { Local, Remote, LocalOrRemote };

    QQmlImportDatabase(QQmlEngine *);
    ~QQmlImportDatabase();

    bool importPlugin(const QString &filePath, const QString &uri, const QString &importNamespace, QList<QQmlError> *errors);

    QStringList importPathList(PathType type = LocalOrRemote) const;
    void setImportPathList(const QStringList &paths);
    void addImportPath(const QString& dir);

    QStringList pluginPathList() const;
    void setPluginPathList(const QStringList &paths);
    void addPluginPath(const QString& path);

private:
    friend class QQmlImportsPrivate;
    QString resolvePlugin(QQmlTypeLoader *typeLoader,
                          const QString &qmldirPath, const QString &qmldirPluginPath,
                          const QString &baseName, const QStringList &suffixes,
                          const QString &prefix = QString());
    QString resolvePlugin(QQmlTypeLoader *typeLoader,
                          const QString &qmldirPath, const QString &qmldirPluginPath,
                          const QString &baseName);

    struct QmldirCache {
        int versionMajor;
        int versionMinor;
        QString qmldirFilePath;
        QString qmldirPathUrl;
        QmldirCache *next;
    };
    // Maps from an import to a linked list of qmldir info.
    // Used in QQmlImportsPrivate::locateQmldir()
    QStringHash<QmldirCache *> qmldirCache;

    // XXX thread
    QStringList filePluginPath;
    QStringList fileImportPath;

    QSet<QString> qmlDirFilesForWhichPluginsHaveBeenLoaded;
    QSet<QString> initializedPlugins;
    QQmlEngine *engine;
};

void qmlClearEnginePlugins();// For internal use by qmlClearRegisteredProperties

QT_END_NAMESPACE

#endif // QQMLIMPORT_P_H

