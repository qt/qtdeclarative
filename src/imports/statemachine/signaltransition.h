/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SIGNALTRANSITION_H
#define SIGNALTRANSITION_H

#include <QtCore/QSignalTransition>
#include <QtCore/QVariant>
#include <QtQml/QJSValue>

#include <QtQml/qqmlscriptstring.h>
#include <QtQml/qqmlparserstatus.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlboundsignalexpressionpointer_p.h>
#include <private/qqmlcompiler_p.h>

QT_BEGIN_NAMESPACE

class SignalTransition : public QSignalTransition, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QJSValue signal READ signal WRITE setSignal NOTIFY qmlSignalChanged)
    Q_PROPERTY(QQmlScriptString guard READ guard WRITE setGuard NOTIFY guardChanged)

public:
    explicit SignalTransition(QState *parent = Q_NULLPTR);

    QQmlScriptString guard() const;
    void setGuard(const QQmlScriptString &guard);

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;

    const QJSValue &signal();
    void setSignal(const QJSValue &signal);

    Q_INVOKABLE void invoke();

Q_SIGNALS:
    void guardChanged();
    void invokeYourself();
    /*!
     * \internal
     */
    void qmlSignalChanged();

private:
    void classBegin() Q_DECL_OVERRIDE { m_complete = false; }
    void componentComplete() Q_DECL_OVERRIDE { m_complete = true; connectTriggered(); }
    void connectTriggered();

    friend class SignalTransitionParser;
    QJSValue m_signal;
    QQmlScriptString m_guard;
    bool m_complete;
    QQmlRefPointer<QQmlCompiledData> m_cdata;
    QList<const QV4::CompiledData::Binding *> m_bindings;
    QQmlBoundSignalExpressionPointer m_signalExpression;
};

class SignalTransitionParser : public QQmlCustomParser
{
public:
    void verifyBindings(const QV4::CompiledData::Unit *qmlUnit, const QList<const QV4::CompiledData::Binding *> &props) Q_DECL_OVERRIDE;
    void applyBindings(QObject *object, QQmlCompiledData *cdata, const QList<const QV4::CompiledData::Binding *> &bindings) Q_DECL_OVERRIDE;
};

QT_END_NAMESPACE

#endif
