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

QT_END_NAMESPACE
