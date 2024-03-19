// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTYPESCREATOR_P_H
#define QMLTYPESCREATOR_P_H

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

#include "qqmltypesclassdescription_p.h"
#include "qqmljsstreamwriter_p.h"

#include <QtCore/qstring.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

class QmlTypesCreator
{
public:
    QmlTypesCreator() : m_qml(&m_output) {}

    bool generate(const QString &outFileName);

    void setOwnTypes(QVector<QCborMap> ownTypes) { m_ownTypes = std::move(ownTypes); }
    void setForeignTypes(QVector<QCborMap> foreignTypes) { m_foreignTypes = std::move(foreignTypes); }
    void setReferencedTypes(QList<QAnyStringView> referencedTypes) { m_referencedTypes = std::move(referencedTypes); }
    void setModule(QByteArray module) { m_module = std::move(module); }
    void setVersion(QTypeRevision version) { m_version = version; }

private:
    void writeClassProperties(const QmlTypesClassDescription &collector);
    void writeType(const QCborMap &property, QLatin1StringView key);
    void writeProperties(const QCborArray &properties);
    void writeMethods(const QCborArray &methods, QLatin1StringView type);

    enum class EnumClassesMode { Scoped, Unscoped };
    void writeEnums(const QCborArray &enums, EnumClassesMode enumClassesMode);

    void writeComponents();
    void writeRootMethods(const QCborMap &classDef);

    QByteArray m_output;
    QQmlJSStreamWriter m_qml;
    QVector<QCborMap> m_ownTypes;
    QVector<QCborMap> m_foreignTypes;
    QList<QAnyStringView> m_referencedTypes;
    QByteArray m_module;
    QTypeRevision m_version = QTypeRevision::zero();
};

QT_END_NAMESPACE

#endif // QMLTYPESCREATOR_P_H
