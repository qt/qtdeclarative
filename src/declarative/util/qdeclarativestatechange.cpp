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

#include "qdeclarativestatechange_p.h"

#include <qdeclarativeexpression.h>
#include "qdeclarativeinfo.h"
#include "qdeclarativestate_p.h"
#include "qdeclarativestategroup_p.h"
#include "qdeclarativeexpression_p.h"
#include "qv8qobjectwrapper_p.h"

QT_BEGIN_NAMESPACE

int QDeclarativeStateChange::m_activateIdx = -1;

QDeclarativeStateChange::QDeclarativeStateChange(QObject *parent) :
    QObject(parent), m_state(0), m_expression(0),
    m_triggerObject(0), m_triggerIdx(-1), m_triggerDirty(false)
{
}

QDeclarativeStateChange::~QDeclarativeStateChange()
{
    delete m_expression;
}

QString QDeclarativeStateChange::toState() const
{
    return m_toState;
}

void QDeclarativeStateChange::setToState(QString arg)
{
    if (m_toState == arg)
        return;

    m_toState = arg;
    emit toStateChanged(arg);
}

QDeclarativeScriptString QDeclarativeStateChange::when() const
{
    return m_when;
}

void QDeclarativeStateChange::setWhen(QDeclarativeScriptString arg)
{
    if (!m_when.script().isEmpty()) {
        qmlInfo(this) << "'when' clause cannot be changed (can only be set once).";
        return;
    }

    m_when = arg;
}

QDeclarativeScriptString QDeclarativeStateChange::trigger() const
{
    return m_trigger;
}

void QDeclarativeStateChange::setTrigger(QDeclarativeScriptString arg)
{
    if (!m_when.script().isEmpty()) {
        qmlInfo(this) << "'trigger' clause cannot be changed (can only be set once).";
        return;
    }

    m_trigger = arg;
    m_triggerDirty = true;
}

void QDeclarativeStateChange::createTrigger()
{
    if (!m_triggerDirty)
        return;

    QDeclarativeExpression triggerExpression(m_trigger.context(), m_trigger.scopeObject(), m_trigger.script());
    QDeclarativeExpressionPrivate *exp = QDeclarativeExpressionPrivate::get(&triggerExpression);

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(m_trigger.context());

    QPair<QObject*, int> qtMethod;
    {
        v8::HandleScope handle_scope;
        v8::Local<v8::Value> result = exp->v8value();
        if (!result->IsFunction())
            return; //### exception?

        qtMethod = QV8QObjectWrapper::ExtractQtMethod(&ep->v8engine, v8::Handle<v8::Function>::Cast(result));
    }

    if (!qtMethod.first)
        return; //### exception?

    //verify that the method is a signal
    QMetaMethod method = qtMethod.first->metaObject()->method(qtMethod.second);
    if (method.methodType() != QMetaMethod::Signal)
        return; //### exception?

    m_triggerObject = qtMethod.first;
    m_triggerIdx = qtMethod.second;

    m_triggerDirty = false;
}

void QDeclarativeStateChange::setState(QDeclarativeState *state)
{
    m_state = state;

    if (!m_state)
        setActive(false);
}

void QDeclarativeStateChange::setActive(bool active)
{
    //qDebug() << "activating" << active << this;
    if (active) {
        createTrigger();
        if (m_triggerObject) {
            if (m_activateIdx < 0)
                m_activateIdx = QDeclarativeStateChange::staticMetaObject.indexOfSlot("activate()");
            QMetaObject::connect(m_triggerObject, m_triggerIdx, this, m_activateIdx);
        }

        if (!m_when.script().isEmpty()) {
            if (!m_expression) {
                m_expression = new QDeclarativeExpression(m_when.context(), m_when.scopeObject(), m_when.script());
                connect(m_expression, SIGNAL(valueChanged()), this, SLOT(updateState()));
            }
            if (!m_triggerObject)   //when is only 'active' if there is no trigger. Otherwise it is 'passive'
                m_expression->setNotifyOnValueChanged(true);
            m_expression->evaluate();
        }
    } else {
        if (m_triggerObject)
            QMetaObject::disconnect(m_triggerObject, m_triggerIdx, this, m_activateIdx);
        if (m_expression)
            m_expression->setNotifyOnValueChanged(false);
    }
}

void QDeclarativeStateChange::updateState()
{
    QVariant value = !m_when.script().isEmpty() ? m_expression->evaluate() : QVariant(true);

    if (value.toBool())
        m_state->stateGroup()->setState(m_toState);
}

void QDeclarativeStateChange::activate()
{
    updateState();
}

QT_END_NAMESPACE
