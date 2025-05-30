// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTYPEREGISTRAR_P_H
#define QMLTYPEREGISTRAR_P_H

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

#include <QJsonObject>
#include <QTypeRevision>

#include <cstdlib>

#include "qmetatypesjsonprocessor_p.h"

QT_BEGIN_NAMESPACE

class QmlTypeRegistrar
{
    QString m_module;
    QString m_targetNamespace;
    QTypeRevision m_moduleVersion;
    QList<quint8> m_pastMajorVersions;
    QStringList m_includes;
    bool m_followForeignVersioning = false;
    QVector<QJsonObject> m_types;
    QVector<QJsonObject> m_foreignTypes;
    QStringList m_referencedTypes;

    QJsonValue findType(const QString &name) const;
    QJsonValue findTypeForeign(const QString &name) const;

public:
    void write(QTextStream &os);
    bool generatePluginTypes(const QString &pluginTypesFile);
    void setModuleNameAndNamespace(const QString &module, const QString &targetNamespace);
    void setModuleVersions(QTypeRevision moduleVersion, const QList<quint8> &pastMajorVersions,
                           bool followForeignVersioning);
    void setIncludes(const QList<QString> &includes);
    void setTypes(const QVector<QJsonObject> &types, const QVector<QJsonObject> &foreignTypes);
    void setReferencedTypes(const QStringList &referencedTypes);

    static bool argumentsFromCommandLineAndFile(QStringList &allArguments,
                                                const QStringList &arguments);
    static int runExtract(const QString &baseName, const MetaTypesJsonProcessor &processor);
};

QT_END_NAMESPACE
#endif // QMLTYPEREGISTRAR_P_H
