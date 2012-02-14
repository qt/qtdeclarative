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

#include "qv8bindings_p.h"

#include <private/qv8_p.h>
#include <private/qdeclarativebinding_p.h>
#include <private/qdeclarativecompiler_p.h>
#include <private/qdeclarativeproperty_p.h>
#include <private/qdeclarativebinding_p_p.h>
#include <private/qdeclarativeexpression_p.h>
#include <private/qobject_p.h>
#include <private/qdeclarativetrace_p.h>
#include <private/qdeclarativeprofilerservice_p.h>

QT_BEGIN_NAMESPACE

QV8Bindings::Binding::Binding()
: object(0), parent(0)
{
}

void QV8Bindings::Binding::setEnabled(bool e, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (enabledFlag() != e) {
        setEnabledFlag(e);

        if (e) update(flags);
    }
}

void QV8Bindings::refresh()
{
    for (int ii = 0; ii < bindingsCount; ++ii)
        bindings[ii].refresh();
}

void QV8Bindings::Binding::refresh()
{
    update();
}

void QV8Bindings::Binding::update(QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (!enabledFlag())
        return;

    QDeclarativeTrace trace("V8 Binding Update");
    trace.addDetail("URL", parent->url);
    trace.addDetail("Line", instruction->line);
    trace.addDetail("Column", instruction->column);

    QDeclarativeBindingProfiler prof(parent->url.toString(), instruction->line, instruction->column);

    QDeclarativeContextData *context = parent->context();
    if (!context || !context->isValid())
        return;

    if (!updatingFlag()) {
        setUpdatingFlag(true);
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(context->engine);

        bool isUndefined = false;

        DeleteWatcher watcher(this);
        ep->referenceScarceResources(); 

        v8::HandleScope handle_scope;
        v8::Context::Scope scope(ep->v8engine()->context());
        v8::Local<v8::Value> result =
            evaluate(context,
                     v8::Handle<v8::Function>::Cast(parent->functions->Get(instruction->value)),
                     &isUndefined);

        trace.event("writing V8 result");
        bool needsErrorData = false;
        if (!watcher.wasDeleted() && !hasError()) {
            typedef QDeclarativePropertyPrivate PP;
            needsErrorData = !PP::writeBinding(object, instruction->property, context, this, result,
                                               isUndefined, flags);
        }

        if (!watcher.wasDeleted()) {

            if (needsErrorData) {
                QUrl url = QUrl(parent->url);
                if (url.isEmpty()) url = QUrl(QLatin1String("<Unknown File>"));

                delayedError()->error.setUrl(url);
                delayedError()->error.setLine(instruction->line);
                delayedError()->error.setColumn(-1);
            }

            if (hasError()) {
                if (!delayedError()->addError(ep)) ep->warning(delayedError()->error);
            } else {
                clearError();
            }

            setUpdatingFlag(false);
        }

        ep->dereferenceScarceResources(); 

    } else {
        QDeclarativeProperty p = QDeclarativePropertyPrivate::restore(object, instruction->property,
                                                                      context);
        QDeclarativeBindingPrivate::printBindingLoopError(p);
    }
}

QString QV8Bindings::Binding::expressionIdentifier()
{
    return parent->url.toString() + QLatin1String(":") + QString::number(instruction->line);
}

void QV8Bindings::Binding::expressionChanged()
{
    update(QDeclarativePropertyPrivate::DontRemoveBinding);
}

void QV8Bindings::Binding::destroy()
{
    setEnabledFlag(false);
    removeFromObject();
    clear();
    clearError();
    parent->release();
}

QV8Bindings::QV8Bindings(int index, int line,
                         QDeclarativeCompiledData *compiled, 
                         QDeclarativeContextData *context)
: bindingsCount(0), bindings(0)
{
    QV8Engine *engine = QDeclarativeEnginePrivate::getV8Engine(context->engine);

    if (compiled->v8bindings[index].IsEmpty()) {
        v8::HandleScope handle_scope;
        v8::Context::Scope scope(engine->context());

        v8::Local<v8::Script> script;
        bool compileFailed = false;
        {
            v8::TryCatch try_catch;
            const QByteArray &program = compiled->programs.at(index);
            script = engine->qmlModeCompile(program.constData(), program.length(), compiled->name, line);
            if (try_catch.HasCaught()) {
                // The binding was not compiled.  There are some exceptional cases which the
                // expression rewriter does not rewrite properly (e.g., \r-terminated lines
                // are not rewritten correctly but this bug is demed out-of-scope to fix for
                // performance reasons; see QTBUG-24064).
                compileFailed = true;
                QDeclarativeError error;
                error.setDescription(QString(QLatin1String("Exception occurred during compilation of binding at line: %1")).arg(line));
                v8::Local<v8::Message> message = try_catch.Message();
                if (!message.IsEmpty())
                    QDeclarativeExpressionPrivate::exceptionToError(message, error);
                QDeclarativeEnginePrivate::get(engine->engine())->warning(error);
                compiled->v8bindings[index] = qPersistentNew(v8::Array::New());
            }
        }

        if (!compileFailed) {
            v8::Local<v8::Value> result = script->Run(engine->contextWrapper()->sharedContext());
            if (result->IsArray()) {
                compiled->v8bindings[index] = qPersistentNew(v8::Local<v8::Array>::Cast(result));
                compiled->programs[index].clear(); // We don't need the source anymore
            }
        }
    }

    url = compiled->url;
    functions = qPersistentNew(compiled->v8bindings[index]);
    bindingsCount = functions->Length();
    if (bindingsCount)
        bindings = new QV8Bindings::Binding[bindingsCount];

    cdata = compiled;
    cdata->addref();

    setContext(context);
}

QV8Bindings::~QV8Bindings()
{
    qPersistentDispose(functions);
    cdata->release();

    delete [] bindings;
    bindings = 0;
    bindingsCount = 0;
}

QDeclarativeAbstractBinding *
QV8Bindings::configBinding(QObject *target, QObject *scope,
                           const QDeclarativeInstruction::instr_assignBinding *i)
{
    if (!bindings) // initialization failed.
        return 0;

    QV8Bindings::Binding *rv = bindings + i->value;

    rv->instruction = i;

    rv->object = target;
    rv->setScopeObject(scope);
    rv->setUseSharedContext(true);
    rv->setNotifyOnValueChanged(true);
    rv->parent = this;

    addref(); // This is decremented in Binding::destroy()

    return rv;
}

QT_END_NAMESPACE
