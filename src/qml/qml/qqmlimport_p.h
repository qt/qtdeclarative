/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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
class QQmlImportedNamespace;
class QQmlImportsPrivate;
class QQmlImportDatabase;
class QQmlTypeLoader;

// Exported for QtQuick1
class Q_QML_EXPORT QQmlImports
{
public:
    QQmlImports(QQmlTypeLoader *);
    QQmlImports(const QQmlImports &);
    ~QQmlImports();
    QQmlImports &operator=(const QQmlImports &);

    void setBaseUrl(const QUrl &url, const QString &urlString = QString());
    QUrl baseUrl() const;

    bool resolveType(const QString& type,
                     QQmlType** type_return, QString* url_return,
                     int *version_major, int *version_minor,
                     QQmlImportedNamespace** ns_return,
                     QList<QQmlError> *errors = 0) const;
    bool resolveType(QQmlImportedNamespace*, 
                     const QString& type,
                     QQmlType** type_return, QString* url_return,
                     int *version_major, int *version_minor) const;

    bool addImport(QQmlImportDatabase *, 
                   const QString& uri, const QString& prefix, int vmaj, int vmin, 
                   QQmlScript::Import::Type importType,
                   const QQmlDirComponents &qmldircomponentsnetwork, 
                   QList<QQmlError> *errors);

    void populateCache(QQmlTypeNameCache *cache, QQmlEngine *) const;

    struct ScriptReference
    {
        QString nameSpace;
        QString qualifier;
        QUrl location;
    };

    QList<ScriptReference> resolvedScripts() const;

private:
    friend class QQmlImportDatabase;
    QQmlImportsPrivate *d;
};

class QQmlImportDatabase
{
    Q_DECLARE_TR_FUNCTIONS(QQmlImportDatabase)
public:
    QQmlImportDatabase(QQmlEngine *);
    ~QQmlImportDatabase();

    bool importPlugin(const QString &filePath, const QString &uri, QList<QQmlError> *errors);

    QStringList importPathList() const;
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


    // XXX thread
    QStringList filePluginPath;
    QStringList fileImportPath;

    QSet<QString> initializedPlugins;
    QQmlEngine *engine;
};

QT_END_NAMESPACE

#endif // QQMLIMPORT_P_H

