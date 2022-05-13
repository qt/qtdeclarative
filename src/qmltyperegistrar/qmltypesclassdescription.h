// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTYPESCLASSDESCRIPTION_H
#define QMLTYPESCLASSDESCRIPTION_H

#include <QtCore/qstring.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qvector.h>
#include <QtCore/qset.h>
#include <QtCore/qversionnumber.h>

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
    void collectRelated(const QString &related, const QVector<QJsonObject> &types,
                        const QVector<QJsonObject> &foreign, QTypeRevision defaultRevision);

    static const QJsonObject *findType(const QVector<QJsonObject> &types, const QString &name);

    void collectLocalAnonymous(const QJsonObject *classDef,const QVector<QJsonObject> &types,
                      const QVector<QJsonObject> &foreign, QTypeRevision defaultRevision);


private:
    void collectSuperClasses(
            const QJsonObject *classDef, const QVector<QJsonObject> &types,
            const QVector<QJsonObject> &foreign, CollectMode mode, QTypeRevision defaultRevision);
    void collectInterfaces(const QJsonObject *classDef);
};

#endif // QMLTYPESCLASSDESCRIPTION_H
