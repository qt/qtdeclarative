// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpropertyvaluesource.h"

QT_BEGIN_NAMESPACE

/*!
    \class QQmlPropertyValueSource
    \brief The QQmlPropertyValueSource class is an interface for property value sources such as animations and bindings.
    \inmodule QtQml

    See \l{Property Value Sources} for information on writing custom property
    value sources.
 */

/*!
    Constructs a QQmlPropertyValueSource.
*/
QQmlPropertyValueSource::QQmlPropertyValueSource()
{
}

/*!
    Destroys the value source.
*/
QQmlPropertyValueSource::~QQmlPropertyValueSource()
{
}

/*!
    \fn void QQmlPropertyValueSource::setTarget(const QQmlProperty &property)
    Set the target \a property for the value source.  This method will
    be called by the QML engine when assigning a value source.
*/

QT_END_NAMESPACE
