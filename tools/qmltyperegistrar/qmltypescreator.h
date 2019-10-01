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

#ifndef QMLTYPESCREATOR_H
#define QMLTYPESCREATOR_H

#include "qmlstreamwriter.h"
#include "qmltypesclassdescription.h"

#include <QtCore/qstring.h>
#include <QtCore/qset.h>

class QmlTypesCreator
{
public:
    QmlTypesCreator() : m_qml(&m_output) {}

    void generate(const QString &outFileName, const QString &dependenciesFileName);

    void setOwnTypes(QVector<QJsonObject> ownTypes) { m_ownTypes = std::move(ownTypes); }
    void setForeignTypes(QVector<QJsonObject> foreignTypes) { m_foreignTypes = std::move(foreignTypes); }
    void setModule(QString module) { m_module = std::move(module); }
    void setMajorVersion(int majorVersion) { m_majorVersion = majorVersion; }

private:
    void writeClassProperties(const QmlTypesClassDescription &collector);
    void writeType(const QJsonObject &property, const QString &key, bool isReadonly,
                   bool parsePointer);
    void writeProperties(const QJsonArray &properties, QSet<QString> &notifySignals);
    void writeMethods(const QJsonArray &methods, const QString &type,
                      const QSet<QString> &notifySignals = QSet<QString>());
    void writeEnums(const QJsonArray &enums);
    void writeComponents();

    QByteArray m_output;
    QmlStreamWriter m_qml;
    QVector<QJsonObject> m_ownTypes;
    QVector<QJsonObject> m_foreignTypes;
    QString m_module;
    int m_majorVersion = 0;
};

#endif // QMLTYPESCREATOR_H
