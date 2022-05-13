// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTYPESCREATOR_H
#define QMLTYPESCREATOR_H

#include "qmltypesclassdescription.h"
#include "qqmljsstreamwriter_p.h"

#include <QtCore/qstring.h>
#include <QtCore/qset.h>

class QmlTypesCreator
{
public:
    QmlTypesCreator() : m_qml(&m_output) {}

    void generate(const QString &outFileName);

    void setOwnTypes(QVector<QJsonObject> ownTypes) { m_ownTypes = std::move(ownTypes); }
    void setForeignTypes(QVector<QJsonObject> foreignTypes) { m_foreignTypes = std::move(foreignTypes); }
    void setReferencedTypes(QStringList referencedTypes) { m_referencedTypes = std::move(referencedTypes); }
    void setModule(QString module) { m_module = std::move(module); }
    void setVersion(QTypeRevision version) { m_version = version; }

private:
    void writeClassProperties(const QmlTypesClassDescription &collector);
    void writeType(const QJsonObject &property, const QString &key);
    void writeProperties(const QJsonArray &properties);
    void writeMethods(const QJsonArray &methods, const QString &type);
    void writeEnums(const QJsonArray &enums);
    void writeComponents();

    QByteArray m_output;
    QQmlJSStreamWriter m_qml;
    QVector<QJsonObject> m_ownTypes;
    QVector<QJsonObject> m_foreignTypes;
    QStringList m_referencedTypes;
    QString m_module;
    QTypeRevision m_version = QTypeRevision::zero();
};

#endif // QMLTYPESCREATOR_H
