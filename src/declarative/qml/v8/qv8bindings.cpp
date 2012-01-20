/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv8bindings_p.h"

#include <private/qv8_p.h>
#include <private/qdeclarativebinding_p.h>
#include <private/qdeclarativecompiler_p.h>
#include <private/qdeclarativeproperty_p.h>
#include <private/qdeclarativebinding_p_p.h>
#include <private/qdeclarativeexpression_p.h>
#include <private/qobject_p.h>
#include <private/qdeclarativetrace_p.h>

QT_BEGIN_NAMESPACE

QV8Bindings::Binding::Binding()
: index(-1), enabled(false), updating(false), line(-1), column(-1), object(0), parent(0)
{
}

void QV8Bindings::Binding::setEnabled(bool e, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (enabled != e) {
        enabled = e;

        if (e) update(flags);
    }
}

void QV8Bindings::Binding::refresh()
{
    update();
}

void QV8Bindings::Binding::update(QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (!enabled)
        return;

    QDeclarativeTrace trace("V8 Binding Update");
    trace.addDetail("URL", parent->url);
    trace.addDetail("Line", line);
    trace.addDetail("Column", column);

    QDeclarativeContextData *context = QDeclarativeAbstractExpression::context();
    if (!context || !context->isValid())
        return;

    if (!updating) {
        updating = true;
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(context->engine);

        bool isUndefined = false;

        QDeleteWatcher watcher(this);
        ep->referenceScarceResources(); 

        v8::HandleScope handle_scope;
        v8::Context::Scope scope(ep->v8engine()->context());
        v8::Local<v8::Value> result = evaluate(v8::Handle<v8::Function>::Cast(parent->functions->Get(index)), 
                                               &isUndefined);

        trace.event("writing V8 result");
        bool needsErrorData = false;
        if (!watcher.wasDeleted() && !error.isValid()) {
            typedef QDeclarativePropertyPrivate PP;
            needsErrorData = !PP::writeBinding(object, property, this, result, isUndefined, flags);
        }

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
        QDeclarativeProperty p = QDeclarativePropertyPrivate::restore(object, property, context);
        QDeclarativeBindingPrivate::printBindingLoopError(p);
    }
}

QString QV8Bindings::Binding::expressionIdentifier()
{
    return parent->url.toString() + QLatin1String(":") + QString::number(line);
}

void QV8Bindings::Binding::expressionChanged()
{
    update(QDeclarativePropertyPrivate::DontRemoveBinding);
}

void QV8Bindings::Binding::destroy()
{
    enabled = false;
    removeFromObject();
    clear();
    removeError();
    parent->release();
}

QV8Bindings::QV8Bindings(const QString &program, int index, int line,
                         QDeclarativeCompiledData *compiled, 
                         QDeclarativeContextData *context)
: bindingsCount(0), bindings(0)
{
    QV8Engine *engine = QDeclarativeEnginePrivate::getV8Engine(context->engine);

    if (compiled->v8bindings[index].IsEmpty()) {
        v8::HandleScope handle_scope;
        v8::Context::Scope scope(engine->context());

        v8::Local<v8::Script> script = engine->qmlModeCompile(program, compiled->name, line);
        v8::Local<v8::Value> result = script->Run(engine->contextWrapper()->sharedContext());

        if (result->IsArray()) 
            compiled->v8bindings[index] = qPersistentNew(v8::Local<v8::Array>::Cast(result));
    }

    url = compiled->url;
    functions = qPersistentNew(compiled->v8bindings[index]);
    bindingsCount = functions->Length();
    bindings = new QV8Bindings::Binding[bindingsCount];
    
    setContext(context);
}

QV8Bindings::~QV8Bindings()
{
    qPersistentDispose(functions);

    delete [] bindings;
    bindings = 0;
    bindingsCount = 0;
}

QDeclarativeAbstractBinding *QV8Bindings::configBinding(int index, QObject *target, QObject *scope, 
                                                        const QDeclarativePropertyData &p,
                                                        int line, int column)
{
    QV8Bindings::Binding *rv = bindings + index;

    rv->line = line;
    rv->column = column;
    rv->index = index;
    rv->object = target;
    rv->property = p;
    rv->setContext(context());
    rv->setScopeObject(scope);
    rv->setUseSharedContext(true);
    rv->setNotifyOnValueChanged(true);
    rv->parent = this;

    addref(); // This is decremented in Binding::destroy()

    return rv;
}

QT_END_NAMESPACE
