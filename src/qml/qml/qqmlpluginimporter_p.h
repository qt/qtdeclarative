/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQMLPLUGINIMPORTER_P_H
#define QQMLPLUGINIMPORTER_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmltypeloaderqmldircontent_p.h>

#include <QtCore/qversionnumber.h>
#include <QtCore/qplugin.h>

QT_BEGIN_NAMESPACE

class QQmlPluginImporter
{
    Q_DISABLE_COPY_MOVE(QQmlPluginImporter)

public:
    using StaticPluginPair = QPair<QStaticPlugin, QJsonArray>;

    QQmlPluginImporter(const QString &uri, QTypeRevision version, QQmlImportDatabase *database,
                   const QQmlTypeLoaderQmldirContent *qmldir, QQmlTypeLoader *typeLoader,
                   QList<QQmlError> *errors)
        : uri(uri)
        , qmldirPath(truncateToDirectory(qmldir->qmldirLocation()))
        , qmldir(qmldir)
        , database(database)
        , typeLoader(typeLoader)
        , errors(errors)
        , version(version)
    {}

    QTypeRevision importDynamicPlugin(
            const QString &filePath, const QString &pluginId, bool optional);
    QTypeRevision importStaticPlugin(QObject *instance, const QString &pluginId);
    QTypeRevision importPlugins();

    static bool removePlugin(const QString &pluginId);
    static QStringList plugins();

private:
    static QString truncateToDirectory(const QString &qmldirFilePath);
    bool populatePluginPairVector(QVector<StaticPluginPair> &result,
                                  const QStringList &versionUris);

    QString resolvePlugin(const QString &qmldirPluginPath, const QString &baseName);
    void finalizePlugin(QObject *instance, const QString &path);

    const QString uri;
    const QString qmldirPath;

    const QQmlTypeLoaderQmldirContent *qmldir = nullptr;
    QQmlImportDatabase *database = nullptr;
    QQmlTypeLoader *typeLoader = nullptr;
    QList<QQmlError> *errors = nullptr;

    const QTypeRevision version;
};

QT_END_NAMESPACE

#endif // QQMLPLUGINIMPORTER_P_H
