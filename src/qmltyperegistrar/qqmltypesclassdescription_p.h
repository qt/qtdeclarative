// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTYPESCLASSDESCRIPTION_P_H
#define QMLTYPESCLASSDESCRIPTION_P_H

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
#include <QtCore/qjsonobject.h>
#include <QtCore/qvector.h>
#include <QtCore/qset.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

struct QmlTypesClassDescription
{
    const QJsonObject *resolvedClass = nullptr;
    QString file;
    QString className;
    QString elementName;
    QString defaultProp;
    QString parentProp;
    QString superClass;
    QString attachedType;
    QString extensionType;
    QString sequenceValueType;
    QString accessSemantics;
    QList<QTypeRevision> revisions;
    QTypeRevision addedInRevision;
    QTypeRevision removedInRevision;
    bool isCreatable = true;
    bool isSingleton = false;
    bool hasCustomParser = false;
    bool omitFromQmlTypes = false;
    bool extensionIsNamespace = false;
    QStringList implementsInterfaces;
    QStringList deferredNames;
    QStringList immediateNames;

    enum CollectMode {
        TopLevel,
        SuperClass,
        RelatedType
    };

    void collect(const QJsonObject *classDef, const QVector<QJsonObject> &types,
                 const QVector<QJsonObject> &foreign, CollectMode mode,
                 QTypeRevision defaultRevision);
    const QJsonObject *collectRelated(
            const QString &related, const QVector<QJsonObject> &types,
            const QVector<QJsonObject> &foreign, QTypeRevision defaultRevision,
            const QStringList &namespaces);
    static const QJsonObject *findType(
            const QVector<QJsonObject> &types, const QVector<QJsonObject> &foreign,
            const QString &name, const QStringList &namespaces);

    void collectLocalAnonymous(const QJsonObject *classDef,const QVector<QJsonObject> &types,
                      const QVector<QJsonObject> &foreign, QTypeRevision defaultRevision);


private:
    void collectSuperClasses(
            const QJsonObject *classDef, const QVector<QJsonObject> &types,
            const QVector<QJsonObject> &foreign, CollectMode mode, QTypeRevision defaultRevision);
    void collectInterfaces(const QJsonObject *classDef);
};

QT_END_NAMESPACE
#endif // QMLTYPESCLASSDESCRIPTION_P_H
