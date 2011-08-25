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

#include "private/qdeclarativestateoperations_p.h"

#include <qdeclarative.h>
#include <qdeclarativecontext.h>
#include <qdeclarativeexpression.h>
#include <qdeclarativeinfo.h>
#include <qdeclarativeguard_p.h>
#include <qdeclarativenullablevalue_p_p.h>
#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativeproperty_p.h"
#include "private/qdeclarativebinding_p.h"
#include "private/qdeclarativestate_p_p.h"

#include <QtCore/qdebug.h>
#include <QtWidgets/qgraphicsitem.h>
#include <QtCore/qmath.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeStateChangeScriptPrivate : public QDeclarativeStateOperationPrivate
{
public:
    QDeclarativeStateChangeScriptPrivate() {}

    QDeclarativeScriptString script;
    QString name;
};

/*!
    \qmlclass StateChangeScript QDeclarativeStateChangeScript
    \inqmlmodule QtQuick 2
    \ingroup qml-state-elements
    \brief The StateChangeScript element allows you to run a script in a state.

    A StateChangeScript is run upon entering a state. You can optionally use
    ScriptAction to specify the point in the transition at which
    the StateChangeScript should to be run.

    \snippet snippets/declarative/states/statechangescript.qml state and transition

    \sa ScriptAction
*/

QDeclarativeStateChangeScript::QDeclarativeStateChangeScript(QObject *parent)
: QDeclarativeStateOperation(*(new QDeclarativeStateChangeScriptPrivate), parent)
{
}

QDeclarativeStateChangeScript::~QDeclarativeStateChangeScript()
{
}

/*!
    \qmlproperty script QtQuick2::StateChangeScript::script
    This property holds the script to run when the state is current.
*/
QDeclarativeScriptString QDeclarativeStateChangeScript::script() const
{
    Q_D(const QDeclarativeStateChangeScript);
    return d->script;
}

void QDeclarativeStateChangeScript::setScript(const QDeclarativeScriptString &s)
{
    Q_D(QDeclarativeStateChangeScript);
    d->script = s;
}

/*!
    \qmlproperty string QtQuick2::StateChangeScript::name
    This property holds the name of the script. This name can be used by a
    ScriptAction to target a specific script.

    \sa ScriptAction::scriptName
*/
QString QDeclarativeStateChangeScript::name() const
{
    Q_D(const QDeclarativeStateChangeScript);
    return d->name;
}

void QDeclarativeStateChangeScript::setName(const QString &n)
{
    Q_D(QDeclarativeStateChangeScript);
    d->name = n;
}

void QDeclarativeStateChangeScript::execute(Reason)
{
    Q_D(QDeclarativeStateChangeScript);
    const QString &script = d->script.script();
    if (!script.isEmpty()) {
        QDeclarativeExpression expr(d->script.context(), d->script.scopeObject(), script);
        QDeclarativeData *ddata = QDeclarativeData::get(this);
        if (ddata && ddata->outerContext && !ddata->outerContext->url.isEmpty())
            expr.setSourceLocation(ddata->outerContext->url.toString(), ddata->lineNumber);
        expr.evaluate();
        if (expr.hasError())
            qmlInfo(this, expr.error());
    }
}

QDeclarativeStateChangeScript::ActionList QDeclarativeStateChangeScript::actions()
{
    ActionList rv;
    QDeclarativeAction a;
    a.event = this;
    rv << a;
    return rv;
}

QString QDeclarativeStateChangeScript::typeName() const
{
    return QLatin1String("StateChangeScript");
}


#include <moc_qdeclarativestateoperations_p.cpp>

QT_END_NAMESPACE

