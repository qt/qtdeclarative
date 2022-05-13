// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstatechangescript_p.h"

#include <qqml.h>
#include <qqmlcontext.h>
#include <qqmlexpression.h>
#include <qqmlinfo.h>
#include <private/qqmlcontext_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlbinding_p.h>
#include "qquickstate_p_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qmath.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQuickStateChangeScriptPrivate : public QQuickStateOperationPrivate
{
public:
    QQuickStateChangeScriptPrivate() {}

    QQmlScriptString script;
    QString name;
};

/*!
    \qmltype StateChangeScript
    \instantiates QQuickStateChangeScript
    \inqmlmodule QtQuick
    \ingroup qtquick-states
    \brief Specifies how to run a script in a state.

    A StateChangeScript is run upon entering a state. You can optionally use
    ScriptAction to specify the point in the transition at which
    the StateChangeScript should be run.

    \snippet qml/states/statechangescript.qml state and transition

    \sa ScriptAction
*/

QQuickStateChangeScript::QQuickStateChangeScript(QObject *parent)
: QQuickStateOperation(*(new QQuickStateChangeScriptPrivate), parent)
{
}

/*!
    \qmlproperty script QtQuick::StateChangeScript::script
    This property holds the script to run when the state is current.
*/
QQmlScriptString QQuickStateChangeScript::script() const
{
    Q_D(const QQuickStateChangeScript);
    return d->script;
}

void QQuickStateChangeScript::setScript(const QQmlScriptString &s)
{
    Q_D(QQuickStateChangeScript);
    d->script = s;
}

/*!
    \qmlproperty string QtQuick::StateChangeScript::name
    This property holds the name of the script. This name can be used by a
    ScriptAction to target a specific script.

    \sa ScriptAction::scriptName
*/
QString QQuickStateChangeScript::name() const
{
    Q_D(const QQuickStateChangeScript);
    return d->name;
}

void QQuickStateChangeScript::setName(const QString &n)
{
    Q_D(QQuickStateChangeScript);
    d->name = n;
}

void QQuickStateChangeScript::execute()
{
    Q_D(QQuickStateChangeScript);
    if (!d->script.isEmpty()) {
        QQmlExpression expr(d->script);
        expr.evaluate();
        if (expr.hasError())
            qmlWarning(this, expr.error());
    }
}

QQuickStateChangeScript::ActionList QQuickStateChangeScript::actions()
{
    ActionList rv;
    QQuickStateAction a;
    a.event = this;
    rv << a;
    return rv;
}

QQuickStateActionEvent::EventType QQuickStateChangeScript::type() const
{
    return Script;
}

QT_END_NAMESPACE

#include <moc_qquickstatechangescript_p.cpp>
