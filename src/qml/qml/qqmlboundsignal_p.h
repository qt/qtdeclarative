/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
#include <private/qqmlboundsignalexpressionpointer_p.h>
#include <private/qqmlnotifier_p.h>
#include <private/qflagpointer_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qbitfield_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QQmlBoundSignalExpression : public QQmlAbstractExpression, public QQmlJavaScriptExpression, public QQmlRefCount
{
public:
    QQmlBoundSignalExpression(QObject *target, int index,
                              QQmlContextData *ctxt, QObject *scope, const QByteArray &expression,
                              bool isRewritten, const QString &fileName, quint16 line, quint16 column);
    QQmlBoundSignalExpression(QObject *target, int index,
                              QQmlContextData *ctxt, QObject *scope, const QString &expression,
                              bool isRewritten, const QString &fileName, quint16 line, quint16 column);

    // "inherited" from QQmlJavaScriptExpression.
    static QString expressionIdentifier(QQmlJavaScriptExpression *);
    static void expressionChanged(QQmlJavaScriptExpression *);

    void setParameterCountForJS(int count) { m_parameterCountForJS = count; }

    // evaluation of a bound signal expression doesn't return any value
    void evaluate(void **a);

    QString sourceFile() const { return m_fileName; }
    quint16 lineNumber() const { return m_line; }
    quint16 columnNumber() const { return m_column; }
    QString expression() const;
    QObject *target() const { return m_target; }

    QQmlEngine *engine() const { return context() ? context()->engine : 0; }

private:
    ~QQmlBoundSignalExpression();

    void init(QQmlContextData *ctxt, QObject *scope);
    bool hasParameterInfo() const { return m_parameterCountForJS > 0; }

    v8::Persistent<v8::Object> m_v8qmlscope;
    v8::Persistent<v8::Function> m_v8function;

    //either expressionUtf8 or expression will be used (but not both).
    //once m_v8function is valid, we clear both expressions, and
    //extract it from m_v8function if needed.
    QByteArray m_expressionUtf8;
    QString m_expression;   //only used when expression needs to be rewritten
    QString m_fileName;
    quint16 m_line;
    quint16 m_column;

    int m_parameterCountForJS;

    QObject *m_target;
    int m_index;

    bool m_expressionFunctionValid:1;
    bool m_expressionFunctionRewritten:1;
    bool m_invalidParameterName:1;
};

class Q_QML_PRIVATE_EXPORT QQmlAbstractBoundSignal
{
public:
    QQmlAbstractBoundSignal();
    virtual ~QQmlAbstractBoundSignal();

    virtual int index() const = 0;
    virtual QQmlBoundSignalExpression *expression() const = 0;
    virtual QQmlBoundSignalExpressionPointer setExpression(QQmlBoundSignalExpression *) = 0;
    virtual QQmlBoundSignalExpressionPointer takeExpression(QQmlBoundSignalExpression *) = 0;
    virtual bool isEvaluating() const = 0;

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

class Q_QML_PRIVATE_EXPORT QQmlBoundSignal : public QQmlAbstractBoundSignal,
                                             public QQmlNotifierEndpoint
{
public:
    QQmlBoundSignal(QObject *target, int signal, QObject *owner, QQmlEngine *engine);
    virtual ~QQmlBoundSignal();

    int index() const;

    QQmlBoundSignalExpression *expression() const;
    QQmlBoundSignalExpressionPointer setExpression(QQmlBoundSignalExpression *);
    QQmlBoundSignalExpressionPointer takeExpression(QQmlBoundSignalExpression *);

    bool isEvaluating() const { return m_isEvaluating; }

private:
    friend void QQmlBoundSignal_callback(QQmlNotifierEndpoint *, void **);

    QQmlBoundSignalExpressionPointer m_expression;
    int m_index;
    bool m_isEvaluating;
};

QT_END_NAMESPACE

#endif // QQMLBOUNDSIGNAL_P_H
