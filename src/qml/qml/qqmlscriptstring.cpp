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

#include "qqmlscriptstring.h"
#include "qqmlscriptstring_p.h"

QT_BEGIN_NAMESPACE

/*!
\class QQmlScriptString
\since 4.7
\brief The QQmlScriptString class encapsulates a script and its context.

QQmlScriptString is used to create QObject properties that accept a script "assignment" from QML.

Normally, the following QML would result in a binding being established for the \c script
property; i.e. \c script would be assigned the value obtained from running \c {myObj.value = Math.max(myValue, 100)}

\qml
MyType {
    script: myObj.value = Math.max(myValue, 100)
}
\endqml

If instead the property had a type of QQmlScriptString,
the script itself -- \e {myObj.value = Math.max(myValue, 100)} -- would be passed to the \c script property
and the class could choose how to handle it. Typically, the class will evaluate
the script at some later time using a QQmlExpression.

\code
QQmlExpression expr(scriptString);
expr.evaluate();
\endcode

\sa QQmlExpression
*/

/*!
Constructs an empty instance.
*/
QQmlScriptString::QQmlScriptString()
:  d(new QQmlScriptStringPrivate)
{
}

/*!
Copies \a other.
*/
QQmlScriptString::QQmlScriptString(const QQmlScriptString &other)
: d(other.d)
{
}

/*!
\internal
*/
QQmlScriptString::~QQmlScriptString()
{
}

/*!
Assigns \a other to this.
*/
QQmlScriptString &QQmlScriptString::operator=(const QQmlScriptString &other)
{
    d = other.d;
    return *this;
}

/*!
Returns the context for the script.
*/
QQmlContext *QQmlScriptString::context() const
{
    return d->context;
}

/*!
Sets the \a context for the script.
*/
void QQmlScriptString::setContext(QQmlContext *context)
{
    d->context = context;
}

/*!
Returns the scope object for the script.
*/
QObject *QQmlScriptString::scopeObject() const
{
    return d->scope;
}

/*!
Sets the scope \a object for the script.
*/
void QQmlScriptString::setScopeObject(QObject *object)
{
    d->scope = object;
}

/*!
Returns the script text.
*/
QString QQmlScriptString::script() const
{
    return d->script;
}

/*!
Sets the \a script text.
*/
void QQmlScriptString::setScript(const QString &script)
{
    d->script = script;
}

QT_END_NAMESPACE

