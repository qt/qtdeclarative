/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef CHECKIDENTIFIERS_H
#define CHECKIDENTIFIERS_H

#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#include <QtQmlCompiler/private/qqmljsmetatypes_p.h>
#include <QtQmlCompiler/private/qqmljsscopesbyid_p.h>

class QColorOutput;

struct FieldMember
{
    QString m_name;
    QString m_parentType;
    QQmlJS::SourceLocation m_location;
};

using MemberAccessChains = QHash<QQmlJSScope::ConstPtr, QVector<QVector<FieldMember>>>;

class CheckIdentifiers
{
public:
    CheckIdentifiers(QQmlJSLogger *logger, const QString &code,
                     const QQmlJSImporter::ImportedTypes &types, const QString &fileName) :
        m_logger(logger), m_code(code), m_types(types), m_fileName(fileName)
    {}

    void operator()(const QQmlJSScopesById &qmlIDs,
                    const QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> &signalHandlers,
                    const MemberAccessChains &memberAccessChains, const QQmlJSScope::ConstPtr &root,
                    const QString &rootId) const;

private:
    void checkMemberAccess(const QVector<FieldMember> &members,
                           const QQmlJSScope::ConstPtr &outerScope,
                           const QQmlJSMetaProperty *prop = nullptr) const;

    QQmlJSLogger *m_logger = nullptr;
    QString m_code;
    QQmlJSImporter::ImportedTypes m_types;
    QString m_fileName;
};

#endif // CHECKIDENTIFIERS_H
