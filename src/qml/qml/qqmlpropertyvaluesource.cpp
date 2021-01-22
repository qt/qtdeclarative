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

#include "qqmlpropertyvaluesource.h"

#include "qqml.h"

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
