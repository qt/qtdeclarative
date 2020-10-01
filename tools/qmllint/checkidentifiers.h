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

#include <QtQmlCompiler/private/scopetree_p.h>
#include <QtQmlCompiler/private/qmljsimporter_p.h>

class ColorOutput;

struct SignalHandler {
    MetaMethod signal;
    bool isMultiline;
};

struct FieldMember
{
    QString m_name;
    QString m_parentType;
    QQmlJS::SourceLocation m_location;
};

using MemberAccessChains = QHash<ScopeTree::ConstPtr, QVector<QVector<FieldMember>>>;

class CheckIdentifiers
{
public:
    CheckIdentifiers(ColorOutput *colorOut, const QString &code,
                     const QmlJSImporter::ImportedTypes &types, const QString &fileName) :
        m_colorOut(colorOut), m_code(code), m_types(types), m_fileName(fileName)
    {}

    bool operator ()(const QHash<QString, ScopeTree::ConstPtr> &qmlIDs,
                     const QHash<QQmlJS::SourceLocation, SignalHandler> &signalHandlers,
                     const MemberAccessChains &memberAccessChains,
                     const ScopeTree::ConstPtr &root, const QString &rootId) const;

    static void printContext(const QString &code, ColorOutput *output,
                             const QQmlJS::SourceLocation &location);

private:
    bool checkMemberAccess(const QVector<FieldMember> &members,
                           const ScopeTree::ConstPtr &outerScope,
                           const MetaProperty *prop = nullptr) const;

    ColorOutput *m_colorOut = nullptr;
    QString m_code;
    QmlJSImporter::ImportedTypes m_types;
    QString m_fileName;
};

#endif // CHECKIDENTIFIERS_H
