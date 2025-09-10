// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <QtCore/qjsonarray.h>
#include <QtCore/qplugin.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QQmlPluginImporter
{
    Q_DISABLE_COPY_MOVE(QQmlPluginImporter)

public:
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

    ~QQmlPluginImporter() = default;

    QTypeRevision importDynamicPlugin(
            const QString &filePath, const QString &pluginId, bool optional);
    QTypeRevision importStaticPlugin(QObject *instance, const QString &pluginId);
    QTypeRevision importPlugins();

    static bool removePlugin(const QString &pluginId);
    static QStringList plugins();

private:
    struct StaticPluginData {
        QStaticPlugin plugin;
        QJsonArray uriList;
    };

    static QString truncateToDirectory(const QString &qmldirFilePath);
    bool populatePluginDataVector(QVector<StaticPluginData> &result,
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
