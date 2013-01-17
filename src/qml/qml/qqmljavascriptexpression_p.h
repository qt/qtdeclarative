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

#ifndef QQMLJAVASCRIPTEXPRESSION_P_H
#define QQMLJAVASCRIPTEXPRESSION_P_H

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

#include <private/qv8_p.h>
#include <QtCore/qglobal.h>
#include <QtQml/qqmlerror.h>
#include <private/qqmlengine_p.h>
#include <private/qpointervaluepair_p.h>

QT_BEGIN_NAMESPACE

class QQmlDelayedError
{
public:
    inline QQmlDelayedError() : nextError(0), prevError(0) {}
    inline ~QQmlDelayedError() { qPersistentDispose(m_message); removeError(); }

    bool addError(QQmlEnginePrivate *);

    inline void removeError() {
        if (!prevError) return;
        if (nextError) nextError->prevError = prevError;
        *prevError = nextError;
        nextError = 0;
        prevError = 0;
    }

    inline bool isValid() const { return !m_message.IsEmpty() || m_error.isValid(); }
    inline const QQmlError &error(QQmlEngine *engine) const { convertMessageToError(engine); return m_error; }
    inline void clearError() { qPersistentDispose(m_message); m_error = QQmlError(); }

    void setMessage(v8::Handle<v8::Message> message);
    void setErrorLocation(const QUrl &url, quint16 line, quint16 column);
    void setErrorDescription(const QString &description);

private:
    void convertMessageToError(QQmlEngine *engine) const;

    mutable QQmlError m_error;
    mutable v8::Persistent<v8::Message> m_message;

    QQmlDelayedError  *nextError;
    QQmlDelayedError **prevError;
};

class QQmlJavaScriptExpression
{
public:
    // Although this looks crazy, we implement our own "vtable" here, rather than relying on
    // C++ virtuals, to save memory.  By doing it ourselves, we can overload the storage
    // location that is use for the vtable to also store the rarely used delayed error.
    // If we use C++ virtuals, we can't do this and it consts us an extra sizeof(void *) in
    // memory for every expression.
    struct VTable {
        QString (*expressionIdentifier)(QQmlJavaScriptExpression *);
        void (*expressionChanged)(QQmlJavaScriptExpression *);
    };

    QQmlJavaScriptExpression(VTable *vtable);

    v8::Local<v8::Value> evaluate(QQmlContextData *, v8::Handle<v8::Function>,
                                  bool *isUndefined);
    v8::Local<v8::Value> evaluate(QQmlContextData *, v8::Handle<v8::Function>,
                                  int argc, v8::Handle<v8::Value> args[],
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
        inline DeleteWatcher(QQmlJavaScriptExpression *);
        inline ~DeleteWatcher();
        inline bool wasDeleted() const;
    private:
        friend class QQmlJavaScriptExpression;
        QObject *_c;
        QQmlJavaScriptExpression **_w;
        QQmlJavaScriptExpression *_s;
    };

    inline bool hasError() const;
    inline bool hasDelayedError() const;
    QQmlError error(QQmlEngine *) const;
    void clearError();
    void clearGuards();
    QQmlDelayedError *delayedError();

    static void exceptionToError(v8::Handle<v8::Message>, QQmlError &);
    static v8::Persistent<v8::Function> evalFunction(QQmlContextData *ctxt, QObject *scope,
                                                     const QString &code, const QString &filename,
                                                     quint16 line,
                                                     v8::Persistent<v8::Object> *qmlscope = 0);
    static v8::Persistent<v8::Function> evalFunction(QQmlContextData *ctxt, QObject *scope,
                                                     const char *code, int codeLength,
                                                     const QString &filename, quint16 line,
                                                     v8::Persistent<v8::Object> *qmlscope = 0);
protected:
    ~QQmlJavaScriptExpression();

private:
    typedef QQmlJavaScriptExpressionGuard Guard;
    friend void QQmlJavaScriptExpressionGuard_callback(QQmlNotifierEndpoint *, void **);

    struct GuardCapture : public QQmlEnginePrivate::PropertyCapture {
        GuardCapture(QQmlEngine *engine, QQmlJavaScriptExpression *e)
        : engine(engine), expression(e), errorString(0) { }

        ~GuardCapture()  {
            Q_ASSERT(guards.isEmpty());
            Q_ASSERT(errorString == 0);
        }

        virtual void captureProperty(QQmlNotifier *);
        virtual void captureProperty(QObject *, int, int);

        QQmlEngine *engine;
        QQmlJavaScriptExpression *expression;
        QFieldList<Guard, &Guard::next> guards;
        QStringList *errorString;
    };

    QPointerValuePair<VTable, QQmlDelayedError> m_vtable;

    // We store some flag bits in the following flag pointers.
    //    m_scopeObject:flag1 - requiresThisObject
    //    activeGuards:flag1  - notifyOnValueChanged
    //    activeGuards:flag2  - useSharedContext
    QBiPointer<QObject, DeleteWatcher> m_scopeObject;
    QForwardFieldList<Guard, &Guard::next> activeGuards;
};

QQmlJavaScriptExpression::DeleteWatcher::DeleteWatcher(QQmlJavaScriptExpression *e)
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

QQmlJavaScriptExpression::DeleteWatcher::~DeleteWatcher()
{
    Q_ASSERT(*_w == 0 || (*_w == _s && _s->m_scopeObject.isT2()));
    if (*_w && _s->m_scopeObject.asT2() == this)
        _s->m_scopeObject = _c;
}

bool QQmlJavaScriptExpression::DeleteWatcher::wasDeleted() const
{
    return *_w == 0;
}

bool QQmlJavaScriptExpression::requiresThisObject() const
{
    return m_scopeObject.flag();
}

void QQmlJavaScriptExpression::setRequiresThisObject(bool v)
{
    m_scopeObject.setFlagValue(v);
}

bool QQmlJavaScriptExpression::useSharedContext() const
{
    return activeGuards.flag2();
}

void QQmlJavaScriptExpression::setUseSharedContext(bool v)
{
    activeGuards.setFlag2Value(v);
}

bool QQmlJavaScriptExpression::notifyOnValueChanged() const
{
    return activeGuards.flag();
}

QObject *QQmlJavaScriptExpression::scopeObject() const
{
    if (m_scopeObject.isT1()) return m_scopeObject.asT1();
    else return m_scopeObject.asT2()->_c;
}

void QQmlJavaScriptExpression::setScopeObject(QObject *v)
{
    if (m_scopeObject.isT1()) m_scopeObject = v;
    else m_scopeObject.asT2()->_c = v;
}

bool QQmlJavaScriptExpression::hasError() const
{
    return m_vtable.hasValue() && m_vtable.constValue()->isValid();
}

bool QQmlJavaScriptExpression::hasDelayedError() const
{
    return m_vtable.hasValue();
}

QQmlJavaScriptExpressionGuard::QQmlJavaScriptExpressionGuard(QQmlJavaScriptExpression *e)
: expression(e), next(0)
{
    setCallback(QQmlNotifierEndpoint::QQmlJavaScriptExpressionGuard);
}

QQmlJavaScriptExpressionGuard *
QQmlJavaScriptExpressionGuard::New(QQmlJavaScriptExpression *e,
                                           QQmlEngine *engine)
{
    Q_ASSERT(e);
    return QQmlEnginePrivate::get(engine)->jsExpressionGuardPool.New(e);
}

void QQmlJavaScriptExpressionGuard::Delete()
{
    QRecyclePool<QQmlJavaScriptExpressionGuard>::Delete(this);
}


QT_END_NAMESPACE

#endif // QQMLJAVASCRIPTEXPRESSION_P_H
