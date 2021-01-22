/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QMLTYPESCLASSDESCRIPTION_H
#define QMLTYPESCLASSDESCRIPTION_H

#include <QtCore/qstring.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qvector.h>
#include <QtCore/qset.h>

struct QmlTypesClassDescription
{
    const QJsonObject *resolvedClass = nullptr;
    QString file;
    QString elementName;
    QString defaultProp;
    QString superClass;
    QString attachedType;
    QList<int> revisions;
    int addedInRevision = -1;
    int removedInRevision = -1;
    bool isCreatable = true;
    bool isSingleton = false;
    bool isRootClass = false;
    bool isBuiltin = false;

    enum CollectMode {
        TopLevel,
        SuperClass,
        AttachedType
    };

    void collect(const QJsonObject *classDef, const QVector<QJsonObject> &types,
                 const QVector<QJsonObject> &foreign, CollectMode mode);
    void collectAttached(const QString &attached, const QVector<QJsonObject> &types,
                         const QVector<QJsonObject> &foreign);

    static const QJsonObject *findType(const QVector<QJsonObject> &types, const QString &name);
};

#endif // QMLTYPESCLASSDESCRIPTION_H
