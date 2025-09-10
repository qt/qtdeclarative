// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qjsnumbercoercion.h"

QT_BEGIN_NAMESPACE

/*!
  \since 6.1
  \class QJSNumberCoercion
  \internal

  \brief Implements the JavaScript double-to-int coercion.
 */

/*!
  \fn bool QJSNumberCoercion::isInteger(double d)
  \internal
  \deprecated 6.7
 */

/*!
  \fn bool QJSNumberCoercion::isArrayIndex(double d)
  \internal

  Checks whether \a d contains a value that can serve as an index into an array.
  For that, \a d must be a non-negative value representable as an unsigned 32bit int.
 */

/*!
  \fn bool QJSNumberCoercion::isArrayIndex(qint64 i)
  \internal

  Checks whether \a i contains a value that can serve as an index into an array.
  For that, \a d must be a non-negative value representable as an unsigned 32bit int.
*/

/*!
  \fn bool QJSNumberCoercion::isArrayIndex(quint64 i)
  \internal

  Checks whether \a i contains a value that can serve as an index into an array.
  For that, \a d must be a value representable as an unsigned 32bit int.
*/

/*!
  \fn int QJSNumberCoercion::toInteger(double d)
  \internal

  Coerces the given \a d to a 32bit integer by JavaScript rules and returns
  the result.
 */

/*!
  \fn bool equals(double lhs, double rhs)
  \internal

  Compares \a lhs and \a rhs bit by bit without causing a compile warning.
  Returns the \c true if they are equal, or \c false if not.
 */

/*!
  \fn double QJSNumberCoercion::roundTowards0(double d)
  \internal

  Rounds \a d towards 0 by JavaScript \e ToInteger rules and returns the result.
*/

QT_END_NAMESPACE
