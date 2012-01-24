/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativebind_p.h"

#include <private/qdeclarativenullablevalue_p_p.h>
#include <private/qdeclarativeproperty_p.h>
#include <private/qdeclarativebinding_p.h>
#include <private/qdeclarativeguard_p.h>

#include <qdeclarativeengine.h>
#include <qdeclarativecontext.h>
#include <qdeclarativeproperty.h>
#include <qdeclarativeinfo.h>

#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeBindPrivate : public QObjectPrivate
{
public:
    QDeclarativeBindPrivate() : componentComplete(true), obj(0), prevBind(0) {}
    ~QDeclarativeBindPrivate() { if (prevBind) prevBind->destroy(); }

    QDeclarativeNullableValue<bool> when;
    bool componentComplete;
    QDeclarativeGuard<QObject> obj;
    QString propName;
    QDeclarativeNullableValue<QVariant> value;
    QDeclarativeProperty prop;
    QDeclarativeAbstractBinding *prevBind;
};


/*!
    \qmlclass Binding QDeclarativeBind
    \inqmlmodule QtQuick 2
    \ingroup qml-working-with-data
    \brief The Binding element allows arbitrary property bindings to be created.

    \section1 Binding to an inaccessible property

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

    \section1 "Single-branch" conditional binding

    In some circumstances you may want to control the value of a property
    only when a certain condition is true (and relinquish control in all
    other cirumstances). This often isn't possible to accomplish with a direct
    binding, as you need to supply values for all possible branches.

    \qml
    // warning: "Unable to assign [undefined] to double value"
    value: if (mouse.pressed) mouse.mouseX
    \endqml

    The above example will produce a warning whenever we release the mouse, as the value
    of the binding is undefined when the mouse isn't pressed. We can use the Binding
    element to rewrite the above code and avoid the warning.

    \qml
    Binding on value {
        when: mouse.pressed
        value: mouse.mouseX
    }
    \endqml

    The Binding element will also restore any previously set direct bindings on
    the property. In that sense, it functions much like a simplified State.

    \qml
    // this is equivilant to the above Binding
    State {
        name: "pressed"
        when: mouse.pressed
        PropertyChanges {
            target: obj
            value: mouse.mouseX
        }
    }
    \endqml

    If the binding target or binding property is changed, the bound value is
    immediately pushed onto the new target.

    \sa QtDeclarative
*/
QDeclarativeBind::QDeclarativeBind(QObject *parent)
    : QObject(*(new QDeclarativeBindPrivate), parent)
{
}

QDeclarativeBind::~QDeclarativeBind()
{
}

/*!
    \qmlproperty bool QtQuick2::Binding::when

    This property holds when the binding is active.
    This should be set to an expression that evaluates to true when you want the binding to be active.

    \code
    Binding {
        target: contactName; property: 'text'
        value: name; when: list.ListView.isCurrentItem
    }
    \endcode

    When the binding becomes inactive again, any direct bindings that were previously
    set on the property will be restored.
*/
bool QDeclarativeBind::when() const
{
    Q_D(const QDeclarativeBind);
    return d->when;
}

void QDeclarativeBind::setWhen(bool v)
{
    Q_D(QDeclarativeBind);
    if (!d->when.isNull && d->when == v)
        return;

    d->when = v;
    eval();
}

/*!
    \qmlproperty Object QtQuick2::Binding::target

    The object to be updated.
*/
QObject *QDeclarativeBind::object()
{
    Q_D(const QDeclarativeBind);
    return d->obj;
}

void QDeclarativeBind::setObject(QObject *obj)
{
    Q_D(QDeclarativeBind);
    if (d->obj && d->when.isValid() && d->when) {
        /* if we switch the object at runtime, we need to restore the
           previous binding on the old object before continuing */
        d->when = false;
        eval();
        d->when = true;
    }
    d->obj = obj;
    if (d->componentComplete)
        d->prop = QDeclarativeProperty(d->obj, d->propName);
    eval();
}

/*!
    \qmlproperty string QtQuick2::Binding::property

    The property to be updated.
*/
QString QDeclarativeBind::property() const
{
    Q_D(const QDeclarativeBind);
    return d->propName;
}

void QDeclarativeBind::setProperty(const QString &p)
{
    Q_D(QDeclarativeBind);
    if (!d->propName.isEmpty() && d->when.isValid() && d->when) {
        /* if we switch the property name at runtime, we need to restore the
           previous binding on the old object before continuing */
        d->when = false;
        eval();
        d->when = true;
    }
    d->propName = p;
    if (d->componentComplete)
        d->prop = QDeclarativeProperty(d->obj, d->propName);
    eval();
}

/*!
    \qmlproperty any QtQuick2::Binding::value

    The value to be set on the target object and property.  This can be a
    constant (which isn't very useful), or a bound expression.
*/
QVariant QDeclarativeBind::value() const
{
    Q_D(const QDeclarativeBind);
    return d->value.value;
}

void QDeclarativeBind::setValue(const QVariant &v)
{
    Q_D(QDeclarativeBind);
    d->value = v;
    eval();
}

void QDeclarativeBind::setTarget(const QDeclarativeProperty &p)
{
    Q_D(QDeclarativeBind);
    d->prop = p;
}

void QDeclarativeBind::classBegin()
{
    Q_D(QDeclarativeBind);
    d->componentComplete = false;
}

void QDeclarativeBind::componentComplete()
{
    Q_D(QDeclarativeBind);
    d->componentComplete = true;
    if (!d->prop.isValid())
        d->prop = QDeclarativeProperty(d->obj, d->propName);
    eval();
}

void QDeclarativeBind::eval()
{
    Q_D(QDeclarativeBind);
    if (!d->prop.isValid() || d->value.isNull || !d->componentComplete)
        return;

    if (d->when.isValid()) {
        if (!d->when) {
            //restore any previous binding
            if (d->prevBind) {
                QDeclarativeAbstractBinding *tmp = d->prevBind;
                d->prevBind = 0;
                tmp = QDeclarativePropertyPrivate::setBinding(d->prop, tmp);
                if (tmp) //should this ever be true?
                    tmp->destroy();
            }
            return;
        }

        //save any set binding for restoration
        QDeclarativeAbstractBinding *tmp;
        tmp = QDeclarativePropertyPrivate::setBinding(d->prop, 0);
        if (tmp && d->prevBind)
            d->prevBind->destroy();
        else if (!d->prevBind)
            d->prevBind = tmp;
    }

    d->prop.write(d->value.value);
}

QT_END_NAMESPACE
