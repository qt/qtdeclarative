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

#ifndef QQMLEXPRESSION_P_H
#define QQMLEXPRESSION_P_H

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

#include "qqmlexpression.h"

#include <private/qqmlengine_p.h>
#include <private/qfieldlist_p.h>
#include <private/qflagpointer_p.h>
#include <private/qqmljavascriptexpression_p.h>

QT_BEGIN_NAMESPACE

class QQmlExpression;
class QString;
class QQmlExpressionPrivate : public QObjectPrivate,
                              public QQmlJavaScriptExpression
{
    Q_DECLARE_PUBLIC(QQmlExpression)
public:
    QQmlExpressionPrivate();
    ~QQmlExpressionPrivate() override;

    void init(QQmlContextData *, const QString &, QObject *);
    void init(QQmlContextData *, QV4::Function *runtimeFunction, QObject *);

    QVariant value(bool *isUndefined = nullptr);

    QV4::ReturnedValue v4value(bool *isUndefined = nullptr);

    static inline QQmlExpressionPrivate *get(QQmlExpression *expr);
    static inline QQmlExpression *get(QQmlExpressionPrivate *expr);

    void _q_notify();

    bool expressionFunctionValid:1;

    // Inherited from QQmlJavaScriptExpression
    QString expressionIdentifier() const override;
    void expressionChanged() override;

    QString expression;

    QString url; // This is a QString for a reason.  QUrls are slooooooow...
    quint16 line;
    quint16 column;
    QString name; //function name, hint for the debugger
};

QQmlExpressionPrivate *QQmlExpressionPrivate::get(QQmlExpression *expr)
{
    return static_cast<QQmlExpressionPrivate *>(QObjectPrivate::get(expr));
}

QQmlExpression *QQmlExpressionPrivate::get(QQmlExpressionPrivate *expr)
{
    return expr->q_func();
}


QT_END_NAMESPACE

#endif // QQMLEXPRESSION_P_H
