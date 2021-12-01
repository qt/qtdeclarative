/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef QQMLJSSCOPESBYID_P_H
#define QQMLJSSCOPESBYID_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.


#include "qqmljsscope_p.h"

#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QQmlJSScopesById
{
public:
    QString id(const QQmlJSScope::ConstPtr &scope) const
    {
        for (auto it = m_scopesById.begin(), end = m_scopesById.end(); it != end; ++it) {
            if (*it == scope)
                return it.key();
        }
        return QString();
    }

    QQmlJSScope::ConstPtr scope(const QString &id, const QQmlJSScope::ConstPtr &referrer) const
    {
        Q_ASSERT(!id.isEmpty());
        const QQmlJSScope::ConstPtr root = componentRoot(referrer);
        const auto range =  m_scopesById.equal_range(id);
        for (auto it = range.first; it != range.second; ++it) {
            if (componentRoot(*it) == root)
                return *it;
        }
        return QQmlJSScope::ConstPtr();
    }

    void insert(const QString &id, const QQmlJSScope::ConstPtr &scope)
    {
        Q_ASSERT(!id.isEmpty());
        m_scopesById.insert(id, scope);
    }

    void clear() { m_scopesById.clear(); }

private:
    static QQmlJSScope::ConstPtr componentRoot(const QQmlJSScope::ConstPtr &inner)
    {
        QQmlJSScope::ConstPtr scope = inner;
        while (scope && !scope->isComponentRootElement() && !scope->isInlineComponent()) {
            if (QQmlJSScope::ConstPtr parent = scope->parentScope())
                scope = parent;
            else
                break;
        }
        return scope;
    }

    QMultiHash<QString, QQmlJSScope::ConstPtr> m_scopesById;
};

QT_END_NAMESPACE

#endif // QQMLJSSCOPESBYID_P_H
