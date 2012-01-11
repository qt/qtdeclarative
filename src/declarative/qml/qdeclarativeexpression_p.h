/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVEEXPRESSION_P_H
#define QDECLARATIVEEXPRESSION_P_H

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

#include "qdeclarativeexpression.h"

#include <private/qv8engine_p.h>
#include <private/qfieldlist_p.h>
#include <private/qdeletewatcher_p.h>
#include <private/qdeclarativeguard_p.h>
#include <private/qdeclarativeengine_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeAbstractExpression : public QDeleteWatchable
{
public:
    QDeclarativeAbstractExpression();
    virtual ~QDeclarativeAbstractExpression();

    bool isValid() const;

    QDeclarativeContextData *context() const;
    void setContext(QDeclarativeContextData *);

    virtual void refresh();

private:
    friend class QDeclarativeContext;
    friend class QDeclarativeContextData;
    friend class QDeclarativeContextPrivate;
    QDeclarativeContextData *m_context;
    QDeclarativeAbstractExpression **m_prevExpression;
    QDeclarativeAbstractExpression  *m_nextExpression;
};

class QDeclarativeDelayedError 
{
public:
    inline QDeclarativeDelayedError() : nextError(0), prevError(0) {}
    inline ~QDeclarativeDelayedError() { removeError(); }

    QDeclarativeError error;

    bool addError(QDeclarativeEnginePrivate *);

    inline void removeError() {
        if (!prevError) return;
        if (nextError) nextError->prevError = prevError;
        *prevError = nextError;
        nextError = 0;
        prevError = 0;
    }

private:
    QDeclarativeDelayedError  *nextError;
    QDeclarativeDelayedError **prevError;
};

class QDeclarativeJavaScriptExpression : public QDeclarativeAbstractExpression, 
                                         public QDeclarativeDelayedError
{
public:
    QDeclarativeJavaScriptExpression();
    virtual ~QDeclarativeJavaScriptExpression();

    v8::Local<v8::Value> evaluate(v8::Handle<v8::Function>, bool *isUndefined);

    inline bool requiresThisObject() const;
    inline void setRequiresThisObject(bool v);
    inline bool useSharedContext() const;
    inline void setUseSharedContext(bool v);
    inline bool notifyOnValueChanged() const;

    void setNotifyOnValueChanged(bool v);
    void resetNotifyOnValueChanged();

    inline QObject *scopeObject() const;
    inline void setScopeObject(QObject *v);

    virtual void expressionChanged() {}

protected:
    inline virtual QString expressionIdentifier();

private:
    quint32 m_requiresThisObject:1;
    quint32 m_useSharedContext:1;
    quint32 m_notifyOnValueChanged:1;
    quint32 m_dummy:29;

    QObject *m_scopeObject;

    typedef QDeclarativeJavaScriptExpressionGuard Guard;

    struct GuardCapture : public QDeclarativeEnginePrivate::PropertyCapture {
        GuardCapture(QDeclarativeJavaScriptExpression *e) : expression(e), errorString(0) {
        }
        ~GuardCapture()  {
            Q_ASSERT(guards.isEmpty());
            Q_ASSERT(errorString == 0);
        }

        virtual void captureProperty(QDeclarativeNotifier *);
        virtual void captureProperty(QObject *, int, int);

        QDeclarativeJavaScriptExpression *expression;
        QFieldList<Guard, &Guard::next> guards;
        QStringList *errorString;
    };

    QFieldList<Guard, &Guard::next> activeGuards;
    GuardCapture *guardCapture;

    void clearGuards();
};

class QDeclarativeExpression;
class QString;
class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarativeExpressionPrivate : public QObjectPrivate, public QDeclarativeJavaScriptExpression
{
    Q_DECLARE_PUBLIC(QDeclarativeExpression)
public:
    QDeclarativeExpressionPrivate();
    ~QDeclarativeExpressionPrivate();

    void init(QDeclarativeContextData *, const QString &, QObject *);
    void init(QDeclarativeContextData *, v8::Handle<v8::Function>, QObject *);
    void init(QDeclarativeContextData *, const QString &, bool, QObject *, const QString &, int, int);

    QVariant value(QObject *secondaryScope = 0, bool *isUndefined = 0);

    v8::Local<v8::Value> v8value(QObject *secondaryScope = 0, bool *isUndefined = 0);

    static inline QDeclarativeExpressionPrivate *get(QDeclarativeExpression *expr);
    static inline QDeclarativeExpression *get(QDeclarativeExpressionPrivate *expr);

    void _q_notify();
    virtual void expressionChanged();

    static void exceptionToError(v8::Handle<v8::Message>, QDeclarativeError &);
    static v8::Persistent<v8::Function> evalFunction(QDeclarativeContextData *ctxt, QObject *scope, 
                                                     const QString &code, const QString &filename, int line,
                                                     v8::Persistent<v8::Object> *qmlscope = 0);
    static QDeclarativeExpression *create(QDeclarativeContextData *, QObject *, const QString &, bool,
                                          const QString &, int, int);

    bool expressionFunctionValid:1;
    bool expressionFunctionRewritten:1;
    bool extractExpressionFromFunction:1;

    inline virtual QString expressionIdentifier();

    QString expression;

    v8::Persistent<v8::Object> v8qmlscope;
    v8::Persistent<v8::Function> v8function;

    QString url; // This is a QString for a reason.  QUrls are slooooooow...
    int line;
    int column;
    QString name; //function name, hint for the debugger

    QDeclarativeRefCount *dataRef;
};

bool QDeclarativeJavaScriptExpression::requiresThisObject() const 
{ 
    return m_requiresThisObject; 
}

void QDeclarativeJavaScriptExpression::setRequiresThisObject(bool v) 
{ 
    m_requiresThisObject = v; 
}

bool QDeclarativeJavaScriptExpression::useSharedContext() const 
{ 
    return m_useSharedContext; 
}

void QDeclarativeJavaScriptExpression::setUseSharedContext(bool v) 
{ 
    m_useSharedContext = v; 
}

bool QDeclarativeJavaScriptExpression::notifyOnValueChanged() const 
{ 
    return m_notifyOnValueChanged; 
}

QObject *QDeclarativeJavaScriptExpression::scopeObject() const 
{ 
    return m_scopeObject; 
}

void QDeclarativeJavaScriptExpression::setScopeObject(QObject *v) 
{ 
    m_scopeObject = v; 
}

QString QDeclarativeJavaScriptExpression::expressionIdentifier() 
{ 
    return QString();
}

QDeclarativeExpressionPrivate *QDeclarativeExpressionPrivate::get(QDeclarativeExpression *expr)
{
    return static_cast<QDeclarativeExpressionPrivate *>(QObjectPrivate::get(expr));
}

QDeclarativeExpression *QDeclarativeExpressionPrivate::get(QDeclarativeExpressionPrivate *expr)
{
    return expr->q_func();
}

QString QDeclarativeExpressionPrivate::expressionIdentifier()
{
    return QLatin1String("\"") + expression + QLatin1String("\"");
}

QDeclarativeJavaScriptExpressionGuard::QDeclarativeJavaScriptExpressionGuard(QDeclarativeJavaScriptExpression *e)
: expression(e), next(0)
{ 
    callback = &endpointCallback;
}

void QDeclarativeJavaScriptExpressionGuard::endpointCallback(QDeclarativeNotifierEndpoint *e)
{
    static_cast<QDeclarativeJavaScriptExpressionGuard *>(e)->expression->expressionChanged();
}

QDeclarativeJavaScriptExpressionGuard *
QDeclarativeJavaScriptExpressionGuard::New(QDeclarativeJavaScriptExpression *e)
{
    Q_ASSERT(e);
    return QDeclarativeEnginePrivate::get(e->context()->engine)->jsExpressionGuardPool.New(e);
}

void QDeclarativeJavaScriptExpressionGuard::Delete()
{
    QRecyclePool<QDeclarativeJavaScriptExpressionGuard>::Delete(this);
}

QT_END_NAMESPACE

#endif // QDECLARATIVEEXPRESSION_P_H
