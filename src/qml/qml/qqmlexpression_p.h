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

#include <private/qv8engine_p.h>
#include <private/qqmlguard_p.h>
#include <private/qqmlengine_p.h>
#include <private/qfieldlist_p.h>
#include <private/qflagpointer_p.h>
#include <private/qdeletewatcher_p.h>
#include <private/qpointervaluepair_p.h>
#include <private/qqmlabstractexpression_p.h>
#include <private/qqmljavascriptexpression_p.h>

QT_BEGIN_NAMESPACE

class QQmlExpression;
class QString;
class Q_QML_PRIVATE_EXPORT QQmlExpressionPrivate : public QObjectPrivate,
                                                   public QQmlJavaScriptExpression,
                                                   public QQmlAbstractExpression
{
    Q_DECLARE_PUBLIC(QQmlExpression)
public:
    QQmlExpressionPrivate();
    ~QQmlExpressionPrivate();

    void init(QQmlContextData *, const QString &, QObject *);
    void init(QQmlContextData *, const QString &, bool, QObject *, const QString &, int, int);
    void init(QQmlContextData *, const QByteArray &, bool, QObject *, const QString &, int, int);

    QVariant value(bool *isUndefined = 0);

    v8::Local<v8::Value> v8value(bool *isUndefined = 0);

    static inline QQmlExpressionPrivate *get(QQmlExpression *expr);
    static inline QQmlExpression *get(QQmlExpressionPrivate *expr);

    void _q_notify();

    static QQmlExpression *create(QQmlContextData *, QObject *, const QString &, bool,
                                  const QString &, int, int);

    bool expressionFunctionValid:1;
    bool expressionFunctionRewritten:1;

    // "Inherited" from QQmlJavaScriptExpression
    static QString expressionIdentifier(QQmlJavaScriptExpression *);
    static void expressionChanged(QQmlJavaScriptExpression *);
    virtual void expressionChanged();

    QString expression;
    QByteArray expressionUtf8;

    v8::Persistent<v8::Object> v8qmlscope;
    v8::Persistent<v8::Function> v8function;

    QString url; // This is a QString for a reason.  QUrls are slooooooow...
    int line;
    int column;
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
