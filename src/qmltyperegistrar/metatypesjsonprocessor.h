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
******************************************************************************/

#ifndef METATYPESJSONPROCESSOR_H
#define METATYPESJSONPROCESSOR_H

#include "qmltypesclassdescription.h"

#include <QtCore/qstring.h>
#include <QtCore/qvector.h>
#include <QtCore/qjsonobject.h>

class MetaTypesJsonProcessor
{
public:
    MetaTypesJsonProcessor(bool privateIncludes) : m_privateIncludes(privateIncludes) {}

    bool processTypes(const QStringList &files);
    bool processForeignTypes(const QStringList &foreignTypesFiles);

    void postProcessTypes();
    void postProcessForeignTypes();

    QVector<QJsonObject> types() const { return m_types; }
    QVector<QJsonObject> foreignTypes() const { return m_foreignTypes; }
    QStringList referencedTypes() const { return m_referencedTypes; }
    QStringList includes() const { return m_includes; }

private:
    enum RegistrationMode {
        NoRegistration,
        ObjectRegistration,
        GadgetRegistration,
        NamespaceRegistration
    };

    static RegistrationMode qmlTypeRegistrationMode(const QJsonObject &classDef);
    void addRelatedTypes();

    void sortTypes(QVector<QJsonObject> &types);
    QString resolvedInclude(const QString &include);;
    void processTypes(const QJsonObject &types);
    void processForeignTypes(const QJsonObject &types);

    QStringList m_includes;
    QStringList m_referencedTypes;
    QVector<QJsonObject> m_types;
    QVector<QJsonObject> m_foreignTypes;
    bool m_privateIncludes = false;
};

#endif // METATYPESJSONPROCESSOR_H
