/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
#include <private/qflagpointer_p.h>
#include <private/qdeletewatcher_p.h>
#include <private/qdeclarativeguard_p.h>
#include <private/qpointervaluepair_p.h>
#include <private/qdeclarativeengine_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeAbstractExpression
{
public:
    QDeclarativeAbstractExpression();
    virtual ~QDeclarativeAbstractExpression();

    bool isValid() const;

    QDeclarativeContextData *context() const;
    void setContext(QDeclarativeContextData *);

    virtual void refresh();

    class DeleteWatcher {
    public:
        inline DeleteWatcher(QDeclarativeAbstractExpression *);
        inline ~DeleteWatcher();
        inline bool wasDeleted() const;
    private:
        friend class QDeclarativeAbstractExpression;
        QDeclarativeContextData *_c;
        QDeclarativeAbstractExpression **_w;
        QDeclarativeAbstractExpression *_s;
    };

private:
    friend class QDeclarativeContext;
    friend class QDeclarativeContextData;
    friend class QDeclarativeContextPrivate;

    QBiPointer<QDeclarativeContextData, DeleteWatcher> m_context;
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

class QDeclarativeJavaScriptExpression
{
public:
    // Although this looks crazy, we implement our own "vtable" here, rather than relying on
    // C++ virtuals, to save memory.  By doing it ourselves, we can overload the storage
    // location that is use for the vtable to also store the rarely used delayed error.
    // If we use C++ virtuals, we can't do this and it consts us an extra sizeof(void *) in
    // memory for every expression.
    struct VTable {
        QString (*expressionIdentifier)(QDeclarativeJavaScriptExpression *);
        void (*expressionChanged)(QDeclarativeJavaScriptExpression *);
    };

    QDeclarativeJavaScriptExpression(VTable *vtable);

    v8::Local<v8::Value> evaluate(QDeclarativeContextData *, v8::Handle<v8::Function>,
                                  bool *isUndefined);

    inline bool requiresThisObject() const;
    inline void setRequiresThisObject(bool v);
    inline bool useSharedContext() const;
    inline void setUseSharedContext(bool v);
    inline bool notifyOnValueChanged() const;

    void setNotifyOnValueChanged(bool v);
    void resetNotifyOnValueChanged();

    inline QObject *scopeObject() const;
    inline void setScopeObject(QObject *v);

    class DeleteWatcher {
    public:
        inline DeleteWatcher(QDeclarativeJavaScriptExpression *);
        inline ~DeleteWatcher();
        inline bool wasDeleted() const;
    private:
        friend class QDeclarativeJavaScriptExpression;
        QObject *_c;
        QDeclarativeJavaScriptExpression **_w;
        QDeclarativeJavaScriptExpression *_s;
    };

    inline bool hasError() const;
    inline bool hasDelayedError() const;
    QDeclarativeError error() const;
    void clearError();
    QDeclarativeDelayedError *delayedError();

protected:
    ~QDeclarativeJavaScriptExpression();

private:
    typedef QDeclarativeJavaScriptExpressionGuard Guard;
    friend class QDeclarativeJavaScriptExpressionGuard;

    struct GuardCapture : public QDeclarativeEnginePrivate::PropertyCapture {
        GuardCapture(QDeclarativeEngine *engine, QDeclarativeJavaScriptExpression *e)
        : engine(engine), expression(e), errorString(0) { }

        ~GuardCapture()  {
            Q_ASSERT(guards.isEmpty());
            Q_ASSERT(errorString == 0);
        }

        virtual void captureProperty(QDeclarativeNotifier *);
        virtual void captureProperty(QObject *, int, int);

        QDeclarativeEngine *engine;
        QDeclarativeJavaScriptExpression *expression;
        QFieldList<Guard, &Guard::next> guards;
        QStringList *errorString;
    };

    QPointerValuePair<VTable, QDeclarativeDelayedError> m_vtable;

    // We store some flag bits in the following flag pointers.
    //    m_scopeObject:flag1 - requiresThisObject
    //    activeGuards:flag1  - notifyOnValueChanged
    //    activeGuards:flag2  - useSharedContext
    QBiPointer<QObject, DeleteWatcher> m_scopeObject;
    QForwardFieldList<Guard, &Guard::next> activeGuards;

    void clearGuards();
};

class QDeclarativeExpression;
class QString;
class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarativeExpressionPrivate : public QObjectPrivate, public QDeclarativeJavaScriptExpression, public QDeclarativeAbstractExpression
{
    Q_DECLARE_PUBLIC(QDeclarativeExpression)
public:
    QDeclarativeExpressionPrivate();
    ~QDeclarativeExpressionPrivate();

    void init(QDeclarativeContextData *, const QString &, QObject *);
    void init(QDeclarativeContextData *, v8::Handle<v8::Function>, QObject *);
    void init(QDeclarativeContextData *, const QString &, bool, QObject *, const QString &, int, int);
    void init(QDeclarativeContextData *, const QByteArray &, bool, QObject *, const QString &, int, int);

    QVariant value(QObject *secondaryScope = 0, bool *isUndefined = 0);

    v8::Local<v8::Value> v8value(QObject *secondaryScope = 0, bool *isUndefined = 0);

    static inline QDeclarativeExpressionPrivate *get(QDeclarativeExpression *expr);
    static inline QDeclarativeExpression *get(QDeclarativeExpressionPrivate *expr);

    void _q_notify();

    static void exceptionToError(v8::Handle<v8::Message>, QDeclarativeError &);
    static v8::Persistent<v8::Function> evalFunction(QDeclarativeContextData *ctxt, QObject *scope, 
                                                     const QString &code, const QString &filename,
                                                     int line,
                                                     v8::Persistent<v8::Object> *qmlscope = 0);
    static v8::Persistent<v8::Function> evalFunction(QDeclarativeContextData *ctxt, QObject *scope,
                                                     const char *code, int codeLength,
                                                     const QString &filename, int line,
                                                     v8::Persistent<v8::Object> *qmlscope = 0);

    static QDeclarativeExpression *create(QDeclarativeContextData *, QObject *, const QString &, bool,
                                          const QString &, int, int);

    bool expressionFunctionValid:1;
    bool expressionFunctionRewritten:1;
    bool extractExpressionFromFunction:1;

    // "Inherited" from QDeclarativeJavaScriptExpression
    static QString expressionIdentifier(QDeclarativeJavaScriptExpression *);
    static void expressionChanged(QDeclarativeJavaScriptExpression *);
    virtual void expressionChanged();

    QString expression;
    QByteArray expressionUtf8;

    v8::Persistent<v8::Object> v8qmlscope;
    v8::Persistent<v8::Function> v8function;

    QString url; // This is a QString for a reason.  QUrls are slooooooow...
    int line;
    int column;
    QString name; //function name, hint for the debugger

    QDeclarativeRefCount *dataRef;
};

QDeclarativeAbstractExpression::DeleteWatcher::DeleteWatcher(QDeclarativeAbstractExpression *e)
: _c(0), _w(0), _s(e)
{
    if (e->m_context.isT1()) {
        _w = &_s;
        _c = e->m_context.asT1();
        e->m_context = this;
    } else {
        // Another watcher is already registered
        _w = &e->m_context.asT2()->_s;
    }
}

QDeclarativeAbstractExpression::DeleteWatcher::~DeleteWatcher()
{
    Q_ASSERT(*_w == 0 || (*_w == _s && _s->m_context.isT2()));
    if (*_w && _s->m_context.asT2() == this)
        _s->m_context = _c;
}

bool QDeclarativeAbstractExpression::DeleteWatcher::wasDeleted() const
{
    return *_w == 0;
}

QDeclarativeJavaScriptExpression::DeleteWatcher::DeleteWatcher(QDeclarativeJavaScriptExpression *e)
: _c(0), _w(0), _s(e)
{
    if (e->m_scopeObject.isT1()) {
        _w = &_s;
        _c = e->m_scopeObject.asT1();
        e->m_scopeObject = this;
    } else {
        // Another watcher is already registered
        _w = &e->m_scopeObject.asT2()->_s;
    }
}

QDeclarativeJavaScriptExpression::DeleteWatcher::~DeleteWatcher()
{
    Q_ASSERT(*_w == 0 || (*_w == _s && _s->m_scopeObject.isT2()));
    if (*_w && _s->m_scopeObject.asT2() == this)
        _s->m_scopeObject = _c;
}

bool QDeclarativeJavaScriptExpression::DeleteWatcher::wasDeleted() const
{
    return *_w == 0;
}

bool QDeclarativeJavaScriptExpression::requiresThisObject() const 
{ 
    return m_scopeObject.flag();
}

void QDeclarativeJavaScriptExpression::setRequiresThisObject(bool v) 
{ 
    m_scopeObject.setFlagValue(v);
}

bool QDeclarativeJavaScriptExpression::useSharedContext() const 
{ 
    return activeGuards.flag2();
}

void QDeclarativeJavaScriptExpression::setUseSharedContext(bool v) 
{ 
    activeGuards.setFlag2Value(v);
}

bool QDeclarativeJavaScriptExpression::notifyOnValueChanged() const 
{ 
    return activeGuards.flag();
}

QObject *QDeclarativeJavaScriptExpression::scopeObject() const 
{ 
    if (m_scopeObject.isT1()) return m_scopeObject.asT1();
    else return m_scopeObject.asT2()->_c;
}

void QDeclarativeJavaScriptExpression::setScopeObject(QObject *v) 
{ 
    if (m_scopeObject.isT1()) m_scopeObject = v;
    else m_scopeObject.asT2()->_c = v;
}

bool QDeclarativeJavaScriptExpression::hasError() const
{
    return m_vtable.hasValue() && m_vtable.constValue()->error.isValid();
}

bool QDeclarativeJavaScriptExpression::hasDelayedError() const
{
    return m_vtable.hasValue();
}

QDeclarativeExpressionPrivate *QDeclarativeExpressionPrivate::get(QDeclarativeExpression *expr)
{
    return static_cast<QDeclarativeExpressionPrivate *>(QObjectPrivate::get(expr));
}

QDeclarativeExpression *QDeclarativeExpressionPrivate::get(QDeclarativeExpressionPrivate *expr)
{
    return expr->q_func();
}

QDeclarativeJavaScriptExpressionGuard::QDeclarativeJavaScriptExpressionGuard(QDeclarativeJavaScriptExpression *e)
: expression(e), next(0)
{ 
    callback = &endpointCallback;
}

void QDeclarativeJavaScriptExpressionGuard::endpointCallback(QDeclarativeNotifierEndpoint *e)
{
    QDeclarativeJavaScriptExpression *expression =
        static_cast<QDeclarativeJavaScriptExpressionGuard *>(e)->expression;

    expression->m_vtable->expressionChanged(expression);
}

QDeclarativeJavaScriptExpressionGuard *
QDeclarativeJavaScriptExpressionGuard::New(QDeclarativeJavaScriptExpression *e,
                                           QDeclarativeEngine *engine)
{
    Q_ASSERT(e);
    return QDeclarativeEnginePrivate::get(engine)->jsExpressionGuardPool.New(e);
}

void QDeclarativeJavaScriptExpressionGuard::Delete()
{
    QRecyclePool<QDeclarativeJavaScriptExpressionGuard>::Delete(this);
}

QT_END_NAMESPACE

#endif // QDECLARATIVEEXPRESSION_P_H
