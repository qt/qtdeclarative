/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
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
QQmlPropertyValueInterceptor::QQmlPropertyValueInterceptor() : m_next(0)
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
