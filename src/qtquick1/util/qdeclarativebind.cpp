/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtQuick1/private/qdeclarativebind_p.h"

#include "QtDeclarative/private/qdeclarativenullablevalue_p_p.h"
#include "QtDeclarative/private/qdeclarativeguard_p.h"

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeproperty.h>

#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtDeclarative/qjsvalue.h>
#include <QtDeclarative/qjsengine.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE



class QDeclarative1BindPrivate : public QObjectPrivate
{
public:
    QDeclarative1BindPrivate() : when(true), componentComplete(true), obj(0) {}

    bool when : 1;
    bool componentComplete : 1;
    QDeclarativeGuard<QObject> obj;
    QString prop;
    QDeclarativeNullableValue<QVariant> value;
};


/*!
    \qmlclass Binding QDeclarative1Bind
    \inqmlmodule QtQuick 1
    \ingroup qml-working-with-data
    \since QtQuick 1.0
    \brief The Binding element allows arbitrary property bindings to be created.

    Sometimes it is necessary to bind to a property of an object that wasn't
    directly instantiated by QML - generally a property of a class exported
    to QML by C++. In these cases, regular property binding doesn't work. Binding
    allows you to bind any value to any property.

    For example, imagine a C++ application that maps an "app.enteredText"
    property into QML. You could use Binding to update the enteredText property
    like this.
    \code
    TextEdit { id: myTextField; text: "Please type here..." }
    Binding { target: app; property: "enteredText"; value: myTextField.text }
    \endcode
    Whenever the text in the TextEdit is updated, the C++ property will be
    updated also.

    If the binding target or binding property is changed, the bound value is
    immediately pushed onto the new target.

    \sa QtDeclarative
*/
QDeclarative1Bind::QDeclarative1Bind(QObject *parent)
    : QObject(*(new QDeclarative1BindPrivate), parent)
{
}

QDeclarative1Bind::~QDeclarative1Bind()
{
}

/*!
    \qmlproperty bool QtQuick1::Binding::when

    This property holds when the binding is active.
    This should be set to an expression that evaluates to true when you want the binding to be active.

    \code
    Binding {
        target: contactName; property: 'text'
        value: name; when: list.ListView.isCurrentItem
    }
    \endcode
*/
bool QDeclarative1Bind::when() const
{
    Q_D(const QDeclarative1Bind);
    return d->when;
}

void QDeclarative1Bind::setWhen(bool v)
{
    Q_D(QDeclarative1Bind);
    d->when = v;
    eval();
}

/*!
    \qmlproperty Object QtQuick1::Binding::target

    The object to be updated.
*/
QObject *QDeclarative1Bind::object()
{
    Q_D(const QDeclarative1Bind);
    return d->obj;
}

void QDeclarative1Bind::setObject(QObject *obj)
{
    Q_D(QDeclarative1Bind);
    d->obj = obj;
    eval();
}

/*!
    \qmlproperty string QtQuick1::Binding::property

    The property to be updated.
*/
QString QDeclarative1Bind::property() const
{
    Q_D(const QDeclarative1Bind);
    return d->prop;
}

void QDeclarative1Bind::setProperty(const QString &p)
{
    Q_D(QDeclarative1Bind);
    d->prop = p;
    eval();
}

/*!
    \qmlproperty any QtQuick1::Binding::value

    The value to be set on the target object and property.  This can be a
    constant (which isn't very useful), or a bound expression.
*/
QVariant QDeclarative1Bind::value() const
{
    Q_D(const QDeclarative1Bind);
    return d->value.value;
}

void QDeclarative1Bind::setValue(const QVariant &v)
{
    Q_D(QDeclarative1Bind);
    d->value.value = v;
    d->value.isNull = false;
    eval();
}

void QDeclarative1Bind::classBegin()
{
    Q_D(QDeclarative1Bind);
    d->componentComplete = false;
}

void QDeclarative1Bind::componentComplete()
{
    Q_D(QDeclarative1Bind);
    d->componentComplete = true;
    eval();
}

void QDeclarative1Bind::eval()
{
    Q_D(QDeclarative1Bind);
    if (!d->obj || d->value.isNull || !d->when || !d->componentComplete)
        return;

    QDeclarativeProperty prop(d->obj, d->prop);
    prop.write(d->value.value);
}



QT_END_NAMESPACE
