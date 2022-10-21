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

#include "qjsprimitivevalue.h"

#include <QtQml/private/qv4runtime_p.h>

QT_BEGIN_NAMESPACE

/*!
  \since 6.1
  \class QJSPrimitiveUndefined

  \inmodule QtQml

  \brief An empty marker type to signify the JavaScript Undefined type and its single value.
  \inmodule QtQml
 */

/*!
  \since 6.1
  \class QJSPrimitiveNull

  \inmodule QtQml

  \brief An empty marker type to signify the JavaScript null value.
  \inmodule QtQml
 */

/*!
  \since 6.1
  \class QJSPrimitiveValue

  \brief The QJSPrimitiveValue class operates on primitive types in JavaScript semantics.

  \ingroup qtjavascript
  \inmodule QtQml

  QJSPrimitiveValue supports most of the primitive types defined in the
  \l{ECMA-262} standard, in particular Undefined, Boolean, Number, and String.
  Additionally, you can store a JavaScript null in a QJSPrimitiveValue and as a
  special case of Number, you can store an integer value.

  All those values are stored immediately, without interacting with the
  JavaScript heap. Therefore, you can pass QJSPrimitiveValues between different
  JavaScript engines. In contrast to QJSManagedValue, there is also no danger
  in destroying a QJSPrimitiveValue from a different thread than it was created
  in. On the flip side, QJSPrimitiveValue does not hold a reference to any
  JavaScript engine.

  QJSPrimitiveValue implements the JavaScript arithmetic and comparison
  operators on the supported types in JavaScript semantics. Types are coerced
  like the JavaScript engine would coerce them if the operators were written
  in a JavaScript expression.

  The JavaScript Symbol type is not supported as it is of very limited utility
  regarding arithmetic and comparison operators, the main purpose of
  QJSPrimitiveValue. In particular, it causes an exception whenever you try to
  coerce it to a number or a string, and we cannot throw exceptions without a
  JavaScript Engine.
 */

/*!
  \enum QJSPrimitiveValue::Type

  This enum speicifies the types a QJSPrimitiveValue might contain.

  \value Undefined The JavaScript Undefined value.
  \value Null The JavaScript null value. This is in fact not a separate
              JavaScript type but a special value of the Object type. As it is
              very common and storable without JavaScript engine, it is still
              supported.
  \value Boolean A JavaScript Boolean value.
  \value Integer An integer. This is a special case of the JavaScript Number
                 type. JavaScript does not have an actual integer type, but
                 the \l{ECMA-262} standard contains rules on how to transform a
                 Number in order to prepare it for certain operators that only
                 make sense on integers, in particular the bit shift operators.
                 QJSPrimitiveValue's Integer type represents the result of such
                 a transformation.
  \value Double A JavaScript Number value.
  \value String A JavaScript String value.
 */

/*!
  \fn Type QJSPrimitiveValue::type() const

  Returns the type of the QJSPrimitiveValue.
 */

/*!
  \fn QJSPrimitiveValue::QJSPrimitiveValue()

  Creates a QJSPrimitiveValue of type Undefined.
 */

/*!
  \fn QJSPrimitiveValue::QJSPrimitiveValue(QJSPrimitiveUndefined undefined)

  Creates a QJSPrimitiveValue of value \a undefined and type Undefined.
 */

/*!
  \fn QJSPrimitiveValue::QJSPrimitiveValue(QJSPrimitiveNull null)

  Creates a QJSPrimitiveValue of value \a null and type Null.
 */

/*!
  \fn QJSPrimitiveValue::QJSPrimitiveValue(bool value)

  Creates a QJSPrimitiveValue of value \a value and type Boolean.
 */

/*!
  \fn QJSPrimitiveValue::QJSPrimitiveValue(int value)

  Creates a QJSPrimitiveValue of value \a value and type Integer.
 */

/*!
  \fn QJSPrimitiveValue::QJSPrimitiveValue(double value)

  Creates a QJSPrimitiveValue of value \a value and type Double.
 */

/*!
  \fn QJSPrimitiveValue::QJSPrimitiveValue(QString value)

  Creates a QJSPrimitiveValue of value \a value and type String.
 */

/*!
  \fn bool QJSPrimitiveValue::toBoolean() const

  Returns the value coerced a boolean by JavaScript rules.
 */

/*!
  \fn int QJSPrimitiveValue::toInteger() const

  Returns the value coerced to an integral 32bit number by the rules JavaScript
  would apply when preparing it for a bit shift operation.
 */

/*!
  \fn double QJSPrimitiveValue::toDouble() const

  Returns the value coerced to a JavaScript Number by JavaScript rules.
 */

/*!
  \fn QString QJSPrimitiveValue::toString() const

  Returns the value coerced to a JavaScript String by JavaScript rules.
 */

/*!
  \fn QJSPrimitiveValue QJSPrimitiveValue::operator+(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)

  \since 6.1

  Perfoms the JavaScript '+' operation on \a lhs and \a rhs, and returns the
  result.
 */

/*!
  \fn QJSPrimitiveValue QJSPrimitiveValue::operator-(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
  \since 6.1

  Performs the JavaScript '-' operation on \a lhs and \a rhs, and returns the
  result.
 */

/*!
  \fn QJSPrimitiveValue QJSPrimitiveValue::operator*(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
  \since 6.1

  Performs the JavaScript '*' operation on \a lhs and \a rhs, and returns the
  result.
 */

/*!
  \fn QJSPrimitiveValue QJSPrimitiveValue::operator/(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
  \since 6.1

  Performs the JavaScript '/' operation between \a lhs and \a rhs, and returns the
  result.
 */

/*!
  \fn bool QJSPrimitiveValue::strictlyEquals(const QJSPrimitiveValue &other) const

  Performs the JavaScript '===' operation on this QJSPrimitiveValue and
  \a other, and returns the result.
 */

/*!
  \fn bool QJSPrimitiveValue::equals(const QJSPrimitiveValue &other) const

  Performs the JavaScript '==' operation on this QJSPrimitiveValue and
  \a other, and returns the result.
 */

/*!
  \fn bool QJSPrimitiveValue::operator==(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
  \since 6.1

  Performs the JavaScript '===' operation on \a lhs and \a rhs, and returns the
  result.
 */

/*!
  \fn bool QJSPrimitiveValue::operator!=(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
  \since 6.1

  Performs the JavaScript '!==' operation on \a lhs and \a rhs, and returns the
  result.
 */

/*!
  \fn bool QJSPrimitiveValue::operator<(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
  \since 6.1

  Performs the JavaScript '<' operation on \a lhs and \a rhs, and returns the
  result.
 */

/*!
  \fn bool QJSPrimitiveValue::operator>(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
  \since 6.1

  Performs the JavaScript '>' operation on \a lhs and \a rhs, and returns the
  result.
 */

/*!
  \fn bool QJSPrimitiveValue::operator<=(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
  \since 6.1

  Performs the JavaScript '<=' operation on \a lhs and \a rhs, and returns the
  result.
 */

/*!
  \fn bool QJSPrimitiveValue::operator>=(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
  \since 6.1

  Performs the JavaScript '>=' operation on \a lhs and \a rhs, and returns the
  result.
 */

QString QJSPrimitiveValue::toString(double d)
{
    QString result;
    QV4::RuntimeHelpers::numberToString(&result, d);
    return result;
}

QT_END_NAMESPACE

