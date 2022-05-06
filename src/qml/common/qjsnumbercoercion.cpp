/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
