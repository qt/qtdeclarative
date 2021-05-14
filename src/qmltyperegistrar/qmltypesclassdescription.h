/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
    bool isRootClass = false;
    bool hasCustomParser = false;
    QStringList implementsInterfaces;

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
