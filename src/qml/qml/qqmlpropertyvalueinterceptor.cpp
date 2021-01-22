/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#include "qqmlpropertyvalueinterceptor_p.h"

#include "qqml.h"

QT_BEGIN_NAMESPACE

/*!
    \class QQmlPropertyValueInterceptor
    \brief The QQmlPropertyValueInterceptor class is inherited by property interceptors such as Behavior.
    \internal

    This class intercepts property writes, allowing for custom handling. For example, Behavior uses this
    interception to provide a default animation for all changes to a property's value.
 */

/*!
    Constructs a QQmlPropertyValueInterceptor.
*/
QQmlPropertyValueInterceptor::QQmlPropertyValueInterceptor() : m_next(nullptr)
{
}

QQmlPropertyValueInterceptor::~QQmlPropertyValueInterceptor()
{
}

/*!
    \fn void QQmlPropertyValueInterceptor::setTarget(const QQmlProperty &property)
    Set the target \a property for the value interceptor.  This method will
    be called by the QML engine when assigning a value interceptor.
*/

/*!
    \fn void QQmlPropertyValueInterceptor::write(const QVariant &value)
    This method will be called when a new \a value is assigned to the property being intercepted.
*/

QT_END_NAMESPACE
