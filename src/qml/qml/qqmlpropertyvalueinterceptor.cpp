// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpropertyvalueinterceptor_p.h"

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
  \internal
  Called when a BindableProperty metacall gets intercepted. The default implementation does nothing
  and simply returns false.
  A subclass which can properly intercept the metacall should return true after doing its work.
  \a bindable is the pointer to the QUntypedBindable passed through the metacall
  \a target is the QUntypedBindable of the intercepted property
*/
bool QQmlPropertyValueInterceptor::bindable(QUntypedBindable *bindable, QUntypedBindable target)
{
    Q_UNUSED(bindable);
    Q_UNUSED(target)
    return false;
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
