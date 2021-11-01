/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QMLTCCOMPILERUTILS_H
#define QMLTCCOMPILERUTILS_H

#include <QtCore/qstring.h>
#include <private/qqmljsscope_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal

    Wraps \a type into \c const and \c & if that is a "good" thing to do (e.g.
    the type is not a pointer type).
*/
inline QString wrapInConstRef(QString type)
{
    if (!type.endsWith(u'*'))
        type = u"const " + type + u"&";
    return type;
}

/*!
    \internal

    Returns an internalName() of \a s, using the accessSemantics() to augment
    the result
*/
inline QString augmentInternalName(const QQmlJSScope::ConstPtr &s)
{
    Q_ASSERT(s->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence);
    const QString suffix =
            (s->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) ? u" *"_qs : u""_qs;
    return s->internalName() + suffix;
}

QT_END_NAMESPACE

#endif // QMLTCCOMPILERUTILS_H
