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

#include "scopetree.h"

class ColorOutput;

class CheckIdentifiers
{
public:
    CheckIdentifiers(ColorOutput *colorOut, const QString &code, const QHash<QString,
                     ScopeTree::ConstPtr> &types) :
        m_colorOut(colorOut), m_code(code), m_types(types)
    {}

    bool operator ()(const QHash<QString, const ScopeTree *> &qmlIDs,
                     const ScopeTree *root, const QString &rootId) const;

private:
    bool checkMemberAccess(const QVector<ScopeTree::FieldMember> &members,
                           const ScopeTree *scope) const;
    void printContext(const QQmlJS::SourceLocation &location) const;

    ColorOutput *m_colorOut = nullptr;
    QString m_code;
    QHash<QString, ScopeTree::ConstPtr> m_types;
};

#endif // CHECKIDENTIFIERS_H
