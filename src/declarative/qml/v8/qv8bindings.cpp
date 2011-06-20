/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv8bindings_p.h"

#include <private/qv8_p.h>
#include <private/qdeclarativebinding_p.h>
#include <private/qdeclarativecompiler_p.h>
#include <private/qdeclarativebinding_p_p.h>
#include <private/qdeclarativeexpression_p.h>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QV8BindingsPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QV8Bindings)
public:
    QV8BindingsPrivate();

    struct Binding : public QDeclarativeJavaScriptExpression,
                     public QDeclarativeAbstractBinding {
        Binding();

        // Inherited from QDeclarativeAbstractBinding
        virtual void setEnabled(bool, QDeclarativePropertyPrivate::WriteFlags flags);
        virtual void update(QDeclarativePropertyPrivate::WriteFlags flags);
        virtual void destroy();

        int index:30;
        bool enabled:1;
        bool updating:1;
        int line;
        QDeclarativeProperty property;
        QV8BindingsPrivate *parent;
    };

    QUrl url;
    int bindingsCount;
    Binding *bindings;
    v8::Persistent<v8::Array> functions;
};

QV8BindingsPrivate::QV8BindingsPrivate()
: bindingsCount(0), bindings(0)
{
}

QV8BindingsPrivate::Binding::Binding()
: index(-1), enabled(false), updating(false), line(-1), parent(0)
{
}

void QV8BindingsPrivate::Binding::setEnabled(bool e, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (enabled != e) {
        enabled = e;

        if (e) update(flags);
    }
}

void QV8BindingsPrivate::Binding::update(QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (!enabled)
        return;

    QDeclarativeContextData *context = QDeclarativeAbstractExpression::context();
    if (!context || !context->isValid())
        return;

    if (!updating) {
        updating = true;
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(context->engine);

        bool isUndefined = false;

        QDeclarativeDeleteWatcher watcher(this);
        ep->referenceScarceResources(); 

        v8::HandleScope handle_scope;
        v8::Context::Scope scope(ep->v8engine.context());
        v8::Local<v8::Value> result = evaluate(v8::Handle<v8::Function>::Cast(parent->functions->Get(index)), 
                                               &isUndefined);

        bool needsErrorData = false;
        if (!watcher.wasDeleted() && !error.isValid()) 
            needsErrorData = !QDeclarativeBindingPrivate::writeBindingResult(this, property, result, 
                                                                             isUndefined, flags);

        if (!watcher.wasDeleted()) {

            if (needsErrorData) {
                QUrl url = QUrl(parent->url);
                if (url.isEmpty()) url = QUrl(QLatin1String("<Unknown File>"));

                error.setUrl(url);
                error.setLine(line);
                error.setColumn(-1);
            }

            if (error.isValid()) {
                if (!addError(ep)) ep->warning(error);
            } else {
                removeError();
            }

            updating = false;
        }

        ep->dereferenceScarceResources(); 

    } else {
        QDeclarativeBindingPrivate::printBindingLoopError(property);
    }
}

void QV8BindingsPrivate::Binding::destroy()
{
    enabled = false;
    removeFromObject();
    clear();
    removeError();
    parent->q_func()->release();
}

QV8Bindings::QV8Bindings(const QString &program, int index, int line,
                         QDeclarativeCompiledData *compiled, 
                         QDeclarativeContextData *context)
: QObject(*(new QV8BindingsPrivate))
{
    Q_D(QV8Bindings);

    QV8Engine *engine = QDeclarativeEnginePrivate::getV8Engine(context->engine);

    if (compiled->v8bindings[index].IsEmpty()) {
        v8::HandleScope handle_scope;
        v8::Context::Scope scope(engine->context());

        v8::Local<v8::Script> script = engine->qmlModeCompile(program, compiled->name, line);
        v8::Local<v8::Value> result = script->Run(engine->contextWrapper()->sharedContext());

        if (result->IsArray()) 
            compiled->v8bindings[index] = qPersistentNew(v8::Local<v8::Array>::Cast(result));
    }

    d->url = compiled->url;
    d->functions = qPersistentNew(compiled->v8bindings[index]);
    d->bindingsCount = d->functions->Length();
    d->bindings = new QV8BindingsPrivate::Binding[d->bindingsCount];
    
    setContext(context);
}

QV8Bindings::~QV8Bindings()
{
    Q_D(QV8Bindings);
    qPersistentDispose(d->functions);

    delete [] d->bindings;
    d->bindings = 0;
    d->bindingsCount = 0;
}

QDeclarativeAbstractBinding *QV8Bindings::configBinding(int index, QObject *target, QObject *scope, 
                                                        const QDeclarativeProperty &property, int line)
{
    Q_D(QV8Bindings);
    QV8BindingsPrivate::Binding *rv = d->bindings + index;

    rv->line = line;
    rv->index = index;
    rv->property = property;
    rv->setContext(context());
    rv->setScopeObject(scope);
    rv->setUseSharedContext(true);
    rv->setNotifyOnValueChanged(true);
    rv->setNotifyObject(this, index);
    rv->parent = d;

    addref(); // This is decremented in Binding::destroy()

    return rv;
}

int QV8Bindings::qt_metacall(QMetaObject::Call c, int id, void **)
{
    Q_D(QV8Bindings);

    if (c == QMetaObject::InvokeMetaMethod) {
        QV8BindingsPrivate::Binding *binding = d->bindings + id;
        binding->update(QDeclarativePropertyPrivate::DontRemoveBinding);
    }
    return -1;
}

QT_END_NAMESPACE
