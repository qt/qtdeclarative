// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef METATYPESJSONPROCESSOR_P_H
#define METATYPESJSONPROCESSOR_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qvector.h>
#include <QtCore/qcbormap.h>

QT_BEGIN_NAMESPACE

class MetaTypesJsonProcessor
{
public:
    static QList<QAnyStringView> namespaces(const QCborMap &classDef);

    MetaTypesJsonProcessor(bool privateIncludes) : m_privateIncludes(privateIncludes) {}

    bool processTypes(const QStringList &files);

    bool processForeignTypes(const QString &foreignTypesFile);
    bool processForeignTypes(const QStringList &foreignTypesFiles);

    void postProcessTypes();
    void postProcessForeignTypes();

    QVector<QCborMap> types() const { return m_types; }
    QVector<QCborMap> foreignTypes() const { return m_foreignTypes; }
    QList<QAnyStringView> referencedTypes() const { return m_referencedTypes; }
    QList<QString> includes() const { return m_includes; }

    QString extractRegisteredTypes() const;

private:
    enum RegistrationMode {
        NoRegistration,
        ObjectRegistration,
        GadgetRegistration,
        NamespaceRegistration
    };

    static RegistrationMode qmlTypeRegistrationMode(const QCborMap &classDef);
    void addRelatedTypes();

    void sortTypes(QVector<QCborMap> &types);
    QString resolvedInclude(QAnyStringView include);
    void processTypes(const QCborMap &types);
    void processForeignTypes(const QCborMap &types);

    QList<QString> m_includes;
    QList<QAnyStringView> m_referencedTypes;
    QVector<QCborMap> m_types;
    QVector<QCborMap> m_foreignTypes;
    bool m_privateIncludes = false;
};

QT_END_NAMESPACE

#endif // METATYPESJSONPROCESSOR_P_H
