/****************************************************************************
**
** Copyright (C) 2016 Ford Motor Company
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

#ifndef SIGNALTRANSITION_H
#define SIGNALTRANSITION_H

#include <QtCore/QSignalTransition>
#include <QtCore/QVariant>
#include <QtQml/QJSValue>

#include <QtQml/qqmlscriptstring.h>
#include <QtQml/qqmlparserstatus.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlboundsignalexpressionpointer_p.h>

QT_BEGIN_NAMESPACE

class SignalTransition : public QSignalTransition, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QJSValue signal READ signal WRITE setSignal NOTIFY qmlSignalChanged)
    Q_PROPERTY(QQmlScriptString guard READ guard WRITE setGuard NOTIFY guardChanged)
    QML_ELEMENT

public:
    explicit SignalTransition(QState *parent = nullptr);

    QQmlScriptString guard() const;
    void setGuard(const QQmlScriptString &guard);

    bool eventTest(QEvent *event) override;
    void onTransition(QEvent *event) override;

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
    void classBegin() override { m_complete = false; }
    void componentComplete() override { m_complete = true; connectTriggered(); }
    void connectTriggered();

    friend class SignalTransitionParser;
    QJSValue m_signal;
    QQmlScriptString m_guard;
    bool m_complete;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> m_compilationUnit;
    QList<const QV4::CompiledData::Binding *> m_bindings;
    QQmlBoundSignalExpressionPointer m_signalExpression;
};

class SignalTransitionParser : public QQmlCustomParser
{
public:
    void verifyBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &props) override;
    void applyBindings(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &bindings) override;
};

template<>
inline QQmlCustomParser *qmlCreateCustomParser<SignalTransition>()
{
    return new SignalTransitionParser;
}

QT_END_NAMESPACE

#endif
