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

#include "qv8bindings_p.h"

#include <private/qv8_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlcompiler_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlexpression_p.h>
#include <private/qobject_p.h>
#include <private/qqmltrace_p.h>
#include <private/qqmlprofilerservice_p.h>

QT_BEGIN_NAMESPACE

QQmlAbstractBinding::VTable QV8Bindings_Binding_vtable = {
    QV8Bindings::Binding::destroy,
    QQmlAbstractBinding::default_expression,
    QV8Bindings::Binding::propertyIndex,
    QV8Bindings::Binding::object,
    QV8Bindings::Binding::setEnabled,
    QV8Bindings::Binding::update,
    QV8Bindings::Binding::retargetBinding
};

static QQmlJavaScriptExpression::VTable QV8Bindings_Binding_jsvtable = {
    QV8Bindings::Binding::expressionIdentifier,
    QV8Bindings::Binding::expressionChanged
};

QV8Bindings::Binding::Binding()
: QQmlJavaScriptExpression(&QV8Bindings_Binding_jsvtable), QQmlAbstractBinding(V8), parent(0)
{
}

void QV8Bindings::Binding::setEnabled(QQmlAbstractBinding *_This, bool e,
                                      QQmlPropertyPrivate::WriteFlags flags)
{
    QV8Bindings::Binding *This = static_cast<QV8Bindings::Binding *>(_This);

    if (This->enabledFlag() != e) {
        This->setEnabledFlag(e);

        if (e) This->update(flags);
    }
}

void QV8Bindings::refresh()
{
    int count = functions()->Length();
    for (int ii = 0; ii < count; ++ii)
        bindings[ii].refresh();
}

void QV8Bindings::Binding::refresh()
{
    update();
}

int QV8Bindings::Binding::propertyIndex(const QQmlAbstractBinding *_This)
{
    const QV8Bindings::Binding *This = static_cast<const QV8Bindings::Binding *>(_This);
    if (This->target.hasValue()) return This->target.constValue()->targetProperty;
    else return This->instruction->property.encodedIndex();
}

QObject *QV8Bindings::Binding::object(const QQmlAbstractBinding *_This)
{
    const QV8Bindings::Binding *This = static_cast<const QV8Bindings::Binding *>(_This);

    if (This->target.hasValue()) return This->target.constValue()->target;
    else return *This->target;
}

QObject *QV8Bindings::Binding::object() const
{
    if (target.hasValue()) return target.constValue()->target;
    else return *target;
}

void QV8Bindings::Binding::retargetBinding(QQmlAbstractBinding *_This, QObject *t, int i)
{
    QV8Bindings::Binding *This = static_cast<QV8Bindings::Binding *>(_This);

    This->target.value().target = t;
    This->target.value().targetProperty = i;
}

void QV8Bindings::Binding::update(QQmlAbstractBinding *_This, QQmlPropertyPrivate::WriteFlags flags)
{
    QV8Bindings::Binding *This = static_cast<QV8Bindings::Binding *>(_This);
    This->update(flags);
}

void QV8Bindings::Binding::dump()
{
    qWarning() << parent->url() << instruction->line << instruction->column;
}

void QV8Bindings::Binding::update(QQmlPropertyPrivate::WriteFlags flags)
{
    if (!enabledFlag())
        return;

    QQmlContextData *context = parent->context();

    if (!context || !context->isValid())
        return;

    // Check that the target has not been deleted
    if (QQmlData::wasDeleted(object()))
        return;

    int lineNo = qmlSourceCoordinate(instruction->line);
    int columnNo = qmlSourceCoordinate(instruction->column);

    QQmlTrace trace("V8 Binding Update");
    trace.addDetail("URL", parent->url());
    trace.addDetail("Line", lineNo);
    trace.addDetail("Column", columnNo);

    QQmlBindingProfiler prof(parent->urlString(), lineNo, columnNo, QQmlProfilerService::V8Binding);

    if (!updatingFlag()) {
        setUpdatingFlag(true);

        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);

        bool isUndefined = false;

        DeleteWatcher watcher(this);
        ep->referenceScarceResources();

        v8::HandleScope handle_scope;
        v8::Context::Scope scope(ep->v8engine()->context());
        v8::Local<v8::Value> result =
            evaluate(context,
                     v8::Handle<v8::Function>::Cast(parent->functions()->Get(instruction->value)),
                     &isUndefined);

        trace.event("writing V8 result");
        bool needsErrorLocationData = false;
        if (!watcher.wasDeleted() && !destroyedFlag() && !hasError()) {
            typedef QQmlPropertyPrivate PP;
            needsErrorLocationData = !PP::writeBinding(*target, instruction->property, context, this, result,
                                               isUndefined, flags);
        }

        if (!watcher.wasDeleted() && !destroyedFlag()) {

            if (needsErrorLocationData)
                delayedError()->setErrorLocation(parent->url(), instruction->line, 0);

            if (hasError()) {
                if (!delayedError()->addError(ep)) ep->warning(this->error(context->engine));
            } else {
                clearError();
            }

            setUpdatingFlag(false);
        }

        ep->dereferenceScarceResources();

    } else {
        QQmlProperty p = QQmlPropertyPrivate::restore(*target, instruction->property, context);
        QQmlAbstractBinding::printBindingLoopError(p);
    }
}

QString QV8Bindings::Binding::expressionIdentifier(QQmlJavaScriptExpression *e)
{
    Binding *This = static_cast<Binding *>(e);
    return This->parent->urlString() + QLatin1Char(':') +
           QString::number(qmlSourceCoordinate(This->instruction->line));
}

void QV8Bindings::Binding::expressionChanged(QQmlJavaScriptExpression *e)
{
    Binding *This = static_cast<Binding *>(e);
    This->update(QQmlPropertyPrivate::DontRemoveBinding);
}

void QV8Bindings::Binding::destroy(QQmlAbstractBinding *_This, QQmlAbstractBinding::DestroyMode mode)
{
    QV8Bindings::Binding *This = static_cast<QV8Bindings::Binding *>(_This);

    if (mode == QQmlAbstractBinding::DisconnectBinding)
        This->clearGuards();

    This->setEnabledFlag(false);
    This->setDestroyedFlag(true);
    This->removeFromObject();
    This->clear();
    This->clearError();
    This->parent->release();
}

QV8Bindings::QV8Bindings(QQmlCompiledData::V8Program *program,
                         quint16 line,
                         QQmlContextData *context)
: program(program), bindings(0), refCount(1)
{
    QV8Engine *engine = QQmlEnginePrivate::getV8Engine(context->engine);

    if (program->bindings.IsEmpty()) {
        v8::HandleScope handle_scope;
        v8::Context::Scope scope(engine->context());

        v8::Local<v8::Script> script;
        bool compileFailed = false;
        {
            v8::TryCatch try_catch;
            const QByteArray &source = program->program;
            script = engine->qmlModeCompile(source.constData(), source.length(),
                                            program->cdata->name, line);
            if (try_catch.HasCaught()) {
                // The binding was not compiled.  There are some exceptional cases which the
                // expression rewriter does not rewrite properly (e.g., \r-terminated lines
                // are not rewritten correctly but this bug is demed out-of-scope to fix for
                // performance reasons; see QTBUG-24064).
                compileFailed = true;
                QQmlError error;
                error.setDescription(QString(QLatin1String("Exception occurred during compilation of binding at line: %1")).arg(line));
                v8::Local<v8::Message> message = try_catch.Message();
                if (!message.IsEmpty())
                    QQmlExpressionPrivate::exceptionToError(message, error);
                QQmlEnginePrivate::get(engine->engine())->warning(error);
                program->bindings = qPersistentNew(v8::Array::New());
            }
        }

        if (!compileFailed) {
            v8::Local<v8::Value> result = script->Run(engine->contextWrapper()->sharedContext());
            if (result->IsArray()) {
                program->bindings = qPersistentNew(v8::Local<v8::Array>::Cast(result));
                program->program.clear(); // We don't need the source anymore
            }
        }
    }

    int bindingsCount = functions()->Length();
    if (bindingsCount) bindings = new QV8Bindings::Binding[bindingsCount];

    setContext(context);
}

QV8Bindings::~QV8Bindings()
{
    program = 0;

    delete [] bindings;
    bindings = 0;
}

QQmlAbstractBinding *
QV8Bindings::configBinding(QObject *target, QObject *scope,
                           const QQmlInstruction::instr_assignBinding *i)
{
    if (!bindings) // initialization failed.
        return 0;

    QV8Bindings::Binding *rv = bindings + i->value;

    rv->instruction = i;
    rv->target = target;
    rv->setScopeObject(scope);
    rv->setUseSharedContext(true);
    rv->setNotifyOnValueChanged(true);
    rv->parent = this;

    if (!i->isFallback)
        addref(); // This is decremented in Binding::destroy()

    return rv;
}

const QUrl &QV8Bindings::url() const
{
    return program->cdata->url;
}

const QString &QV8Bindings::urlString() const
{
    return program->cdata->name;
}

v8::Persistent<v8::Array> &QV8Bindings::functions() const
{
    return program->bindings;
}


QT_END_NAMESPACE
