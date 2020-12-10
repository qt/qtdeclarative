/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QJSNUMBERCOERCION_H
#define QJSNUMBERCOERCION_H

#include <QtCore/qglobal.h>
#include <cstring>

QT_BEGIN_NAMESPACE

class QJSNumberCoercion
{
public:
    static constexpr int toInteger(double d) {
        int i = static_cast<int>(d);
        if (equals(i, d))
            return i;
        return QJSNumberCoercion(d).toInteger();
    }

    static constexpr bool equals(double lhs, double rhs)
    {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_FLOAT_COMPARE
        return lhs == rhs;
        QT_WARNING_POP
    }

private:
    constexpr QJSNumberCoercion(double dbl)
    {
        // the dbl == 0 path is guaranteed constexpr. The other one may or may not be, depending
        // on whether and how the compiler inlines the memcpy.
        // In order to declare the ctor constexpr we need one guaranteed constexpr path.
        if (!equals(dbl, 0))
            memcpy(&d, &dbl, sizeof(double));
    }

    constexpr int sign() const
    {
        return (d >> 63) ? -1 : 1;
    }

    constexpr bool isDenormal() const
    {
        return static_cast<int>((d << 1) >> 53) == 0;
    }

    constexpr int exponent() const
    {
        return static_cast<int>((d << 1) >> 53) - 1023;
    }

    constexpr quint64 significant() const
    {
        quint64 m = (d << 12) >> 12;
        if (!isDenormal())
            m |= (static_cast<quint64>(1) << 52);
        return m;
    }

    constexpr int toInteger()
    {
        int e = exponent() - 52;
        if (e < 0) {
            if (e <= -53)
                return 0;
            return sign() * static_cast<int>(significant() >> -e);
        } else {
            if (e > 31)
                return 0;
            return sign() * (static_cast<int>(significant()) << e);
        }
    }

    quint64 d = 0;
};

QT_END_NAMESPACE

#endif // QJSNUMBERCOERCION_H
