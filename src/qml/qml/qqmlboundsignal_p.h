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

#ifndef QQMLBOUNDSIGNAL_P_H
#define QQMLBOUNDSIGNAL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qmetaobject.h>

#include <private/qqmljavascriptexpression_p.h>
#include <private/qqmlboundsignalexpressionpointer_p.h>
#include <private/qqmlnotifier_p.h>
#include <private/qflagpointer_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qbitfield_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QQmlBoundSignalExpression : public QQmlJavaScriptExpression, public QQmlRefCount
{
public:
    QQmlBoundSignalExpression(QObject *target, int index,
                              QQmlContextData *ctxt, QObject *scope, const QString &expression,
                              const QString &fileName, quint16 line, quint16 column,
                              const QString &handlerName = QString(),
                              const QString &parameterString = QString());

    QQmlBoundSignalExpression(QObject *target, int index,
                              QQmlContextData *ctxt, QObject *scopeObject, QV4::Function *function,
                              QV4::ExecutionContext *scope = nullptr);

    // inherited from QQmlJavaScriptExpression.
    QString expressionIdentifier() const override;
    void expressionChanged() override;

    // evaluation of a bound signal expression doesn't return any value
    void evaluate(void **a);
    void evaluate(const QList<QVariant> &args);

    QString expression() const;
    QObject *target() const { return m_target; }

    QQmlEngine *engine() const { return context() ? context()->engine : nullptr; }

private:
    ~QQmlBoundSignalExpression() override;

    void init(QQmlContextData *ctxt, QObject *scope);

    bool expressionFunctionValid() const { return function() != nullptr; }

    int m_index;
    QObject *m_target;
};

class Q_QML_PRIVATE_EXPORT QQmlBoundSignal : public QQmlNotifierEndpoint
{
public:
    QQmlBoundSignal(QObject *target, int signal, QObject *owner, QQmlEngine *engine);
    ~QQmlBoundSignal();

    void removeFromObject();

    QQmlBoundSignalExpression *expression() const;
    void takeExpression(QQmlBoundSignalExpression *);

    void setEnabled(bool enabled);

private:
    friend void QQmlBoundSignal_callback(QQmlNotifierEndpoint *, void **);
    friend class QQmlPropertyPrivate;
    friend class QQmlData;
    friend class QQmlEngineDebugService;

    void addToObject(QObject *owner);

    QQmlBoundSignal **m_prevSignal;
    QQmlBoundSignal  *m_nextSignal;

    bool m_enabled;

    QQmlBoundSignalExpressionPointer m_expression;
};

QT_END_NAMESPACE

#endif // QQMLBOUNDSIGNAL_P_H
