/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QMLJS_MATH_H
#define QMLJS_MATH_H

#include <qglobal.h>

#ifndef QMLJS_LLVM_RUNTIME
#  include <QtCore/qnumeric.h>
#endif // QMLJS_LLVM_RUNTIME
#include <cmath>

#if defined(Q_CC_GNU)
#define QMLJS_READONLY __attribute((const))
#else
#define QMLJS_READONLY
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

#if !defined(QMLJS_LLVM_RUNTIME) && defined(Q_CC_GNU) && defined(Q_PROCESSOR_X86)

static inline QMLJS_READONLY Value add_int32(int a, int b)
{
    quint8 overflow = 0;
    int aa = a;

    asm ("addl %2, %1\n"
         "seto %0"
    : "=q" (overflow), "=r" (aa)
         : "r" (b), "1" (aa)
         : "cc"
    );
    if (!overflow)
        return Primitive::fromInt32(aa);
    qint64 result = static_cast<qint64>(a) + b;
    return Primitive::fromDouble(result);
}

static inline QMLJS_READONLY Value sub_int32(int a, int b)
{
    quint8 overflow = 0;
    int aa = a;

    asm ("subl %2, %1\n"
         "seto %0"
    : "=q" (overflow), "=r" (aa)
         : "r" (b), "1" (aa)
         : "cc"
    );
    if (!overflow)
        return Primitive::fromInt32(aa);
    qint64 result = static_cast<qint64>(a) - b;
    return Primitive::fromDouble(result);
}

static inline QMLJS_READONLY Value mul_int32(int a, int b)
{
    quint8 overflow = 0;
    int aa = a;

    asm ("imul %2, %1\n"
         "setc %0"
         : "=q" (overflow), "=r" (aa)
         : "r" (b), "1" (aa)
         : "cc"
    );
    if (!overflow)
        return Primitive::fromInt32(aa);
    qint64 result = static_cast<qint64>(a) * b;
    return Primitive::fromDouble(result);
}

#else

static inline QMLJS_READONLY Value add_int32(int a, int b)
{
    qint64 result = static_cast<qint64>(a) + b;
    if (result > INT_MAX || result < INT_MIN)
        return Primitive::fromDouble(result);
    return Primitive::fromInt32(static_cast<int>(result));
}

static inline QMLJS_READONLY Value sub_int32(int a, int b)
{
    qint64 result = static_cast<qint64>(a) - b;
    if (result > INT_MAX || result < INT_MIN)
        return Primitive::fromDouble(result);
    return Primitive::fromInt32(static_cast<int>(result));
}

static inline QMLJS_READONLY Value mul_int32(int a, int b)
{
    qint64 result = static_cast<qint64>(a) * b;
    if (result > INT_MAX || result < INT_MIN)
        return Primitive::fromDouble(result);
    return Primitive::fromInt32(static_cast<int>(result));
}

#endif // defined(QMLJS_INLINE_MATH)

}

QT_END_NAMESPACE

#ifdef QMLJS_READONLY
#undef QMLJS_READONLY
#endif

#endif // QMLJS_MATH_H
