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

#include <private/qqmlabstractexpression_p.h>
#include <private/qqmljavascriptexpression_p.h>
#include <private/qqmlnotifier_p.h>
#include <private/qflagpointer_p.h>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QQmlBoundSignalExpression : public QQmlAbstractExpression, public QQmlJavaScriptExpression
{
public:
    QQmlBoundSignalExpression(QQmlContextData *ctxt, QObject *scope, const QByteArray &expression,
                              bool isRewritten, const QString &fileName, int line, int column);
    QQmlBoundSignalExpression(QQmlContextData *ctxt, QObject *scope, const QString &expression,
                              bool isRewritten, const QString &fileName, int line, int column);
    ~QQmlBoundSignalExpression();

    // "inherited" from QQmlJavaScriptExpression.
    static QString expressionIdentifier(QQmlJavaScriptExpression *);
    static void expressionChanged(QQmlJavaScriptExpression *);

    // evaluation of a bound signal expression doesn't return any value
    void evaluate(QObject *secondaryScope = 0);

    QString sourceFile() const { return m_fileName; }
    int lineNumber() const { return m_line; }
    int columnNumber() const { return m_column; }
    QString expression() const { return m_expression; }

    QQmlEngine *engine() const { return context() ? context()->engine : 0; }

private:
    v8::Persistent<v8::Object> m_v8qmlscope;
    v8::Persistent<v8::Function> m_v8function;

    QString m_expression;
    QString m_functionName; // hint for debugger
    QString m_fileName;
    int m_line;
    int m_column;

    bool m_expressionFunctionValid:1;
    bool m_expressionFunctionRewritten:1;
};

class Q_QML_EXPORT QQmlAbstractBoundSignal
{
public:
    QQmlAbstractBoundSignal();
    virtual ~QQmlAbstractBoundSignal();

    virtual int index() const = 0;
    virtual QQmlBoundSignalExpression *expression() const = 0;
    virtual QQmlBoundSignalExpression *setExpression(QQmlBoundSignalExpression *) = 0;
    virtual QObject *scope() = 0;

    void removeFromObject();
protected:
    void addToObject(QObject *owner);

private:
    friend class QQmlData;
    friend class QQmlPropertyPrivate;
    friend class QQmlEngineDebugService;
    QQmlAbstractBoundSignal **m_prevSignal;
    QQmlAbstractBoundSignal  *m_nextSignal;
};

class QQmlBoundSignalParameters;
class Q_QML_EXPORT QQmlBoundSignal : public QQmlAbstractBoundSignal,
                                     public QQmlNotifierEndpoint
{
public:
    QQmlBoundSignal(QObject *scope, const QMetaMethod &signal, QObject *owner);
    virtual ~QQmlBoundSignal();

    int index() const;

    QQmlBoundSignalExpression *expression() const;
    QQmlBoundSignalExpression *setExpression(QQmlBoundSignalExpression *);
    QObject *scope() { return *m_scope; }

    static void subscriptionCallback(QQmlNotifierEndpoint *e, void **);

    bool isEvaluating() const { return m_scope.flag(); }

private:
    QQmlBoundSignalExpression *m_expression;
    QQmlBoundSignalParameters *m_params;
    // We store some flag bits in the following flag pointer.
    //    m_scope:flag1 - m_isEvaluating
    //    m_scope:flag2 - m_paramsValid
    QFlagPointer<QObject> m_scope;
    int m_index;

    void setIsEvaluating(bool v) { m_scope.setFlagValue(v); }
    void setParamsValid(bool v) { m_scope.setFlag2Value(v); }
    bool paramsValid() const { return m_scope.flag2(); }
};


QT_END_NAMESPACE

#endif // QQMLBOUNDSIGNAL_P_H
