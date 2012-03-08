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

#include "qqmlbinding_p.h"
#include "qqmlbinding_p_p.h"

#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlinfo.h"
#include "qqmlcompiler_p.h"
#include "qqmldata_p.h"
#include <private/qqmlprofilerservice_p.h>
#include <private/qqmltrace_p.h>

#include <QVariant>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QQmlAbstractBinding::QQmlAbstractBinding()
: m_prevBinding(0), m_nextBinding(0)
{
}

QQmlAbstractBinding::~QQmlAbstractBinding()
{
    Q_ASSERT(m_prevBinding == 0);
    Q_ASSERT(*m_mePtr == 0);
}

/*!
Destroy the binding.  Use this instead of calling delete.

Bindings are free to implement their own memory management, so the delete operator is not 
necessarily safe.  The default implementation clears the binding, removes it from the object
and calls delete.
*/
void QQmlAbstractBinding::destroy()
{
    removeFromObject();
    clear();

    delete this;
}

/*!
Add this binding to \a object.

This transfers ownership of the binding to the object, marks the object's property as
being bound.  

However, it does not enable the binding itself or call update() on it.
*/
void QQmlAbstractBinding::addToObject()
{
    Q_ASSERT(!m_prevBinding);

    QObject *obj = object();
    Q_ASSERT(obj);

    int index = propertyIndex();

    QQmlData *data = QQmlData::get(obj, true);

    if (index & 0xFF000000) {
        // Value type

        int coreIndex = index & 0xFFFFFF;

        // Find the value type proxy (if there is one)
        QQmlValueTypeProxyBinding *proxy = 0;
        if (data->hasBindingBit(coreIndex)) {
            QQmlAbstractBinding *b = data->bindings;
            while (b && b->propertyIndex() != coreIndex)
                b = b->m_nextBinding;
            Q_ASSERT(b && b->bindingType() == QQmlAbstractBinding::ValueTypeProxy);
            proxy = static_cast<QQmlValueTypeProxyBinding *>(b);
        }

        if (!proxy) {
            proxy = new QQmlValueTypeProxyBinding(obj, coreIndex);

            Q_ASSERT(proxy->propertyIndex() == coreIndex);
            Q_ASSERT(proxy->object() == obj);

            proxy->addToObject();
        }

        m_nextBinding = proxy->m_bindings;
        if (m_nextBinding) m_nextBinding->m_prevBinding = &m_nextBinding;
        m_prevBinding = &proxy->m_bindings;
        proxy->m_bindings = this;

    } else {
        m_nextBinding = data->bindings;
        if (m_nextBinding) m_nextBinding->m_prevBinding = &m_nextBinding;
        m_prevBinding = &data->bindings;
        data->bindings = this;

        data->setBindingBit(obj, index);
    }
}

/*!
Remove the binding from the object.
*/
void QQmlAbstractBinding::removeFromObject()
{
    if (m_prevBinding) {
        int index = propertyIndex();

        *m_prevBinding = m_nextBinding;
        if (m_nextBinding) m_nextBinding->m_prevBinding = m_prevBinding;
        m_prevBinding = 0;
        m_nextBinding = 0;

        if (index & 0xFF000000) {
            // Value type - we don't remove the proxy from the object.  It will sit their happily
            // doing nothing until it is removed by a write, a binding change or it is reused
            // to hold more sub-bindings.
        } else if (QObject *obj = object()) {
            QQmlData *data = QQmlData::get(obj, false);
            if (data) data->clearBindingBit(index);
        }
    }
}

static void bindingDummyDeleter(QQmlAbstractBinding *)
{
}

QQmlAbstractBinding::Pointer QQmlAbstractBinding::weakPointer()
{
    if (m_mePtr.value().isNull())
        m_mePtr.value() = QSharedPointer<QQmlAbstractBinding>(this, bindingDummyDeleter);

    return m_mePtr.value().toWeakRef();
}

void QQmlAbstractBinding::clear()
{
    if (!m_mePtr.isNull()) {
        **m_mePtr = 0;
        m_mePtr = 0;
    }
}

void QQmlAbstractBinding::retargetBinding(QObject *, int)
{
    qFatal("QQmlAbstractBinding::retargetBinding() called on illegal binding.");
}

QString QQmlAbstractBinding::expression() const
{
    return QLatin1String("<Unknown>");
}

void QQmlAbstractBinding::setEnabled(bool enabled, QQmlPropertyPrivate::WriteFlags flags)
{
    if (enabled) update(flags);
}

QQmlBinding::Identifier QQmlBinding::Invalid = -1;

void QQmlBindingPrivate::refresh()
{
    Q_Q(QQmlBinding);
    q->update();
}

QQmlBindingPrivate::QQmlBindingPrivate()
: updating(false), enabled(false), target(), targetProperty(0)
{
}

QQmlBindingPrivate::~QQmlBindingPrivate()
{
}

QQmlBinding *
QQmlBinding::createBinding(Identifier id, QObject *obj, QQmlContext *ctxt,
                                   const QString &url, int lineNumber, QObject *parent)
{
    if (id < 0)
        return 0;

    QQmlContextData *ctxtdata = QQmlContextData::get(ctxt);

    QQmlEnginePrivate *engine = QQmlEnginePrivate::get(ctxt->engine());
    QQmlCompiledData *cdata = 0;
    QQmlTypeData *typeData = 0;
    if (engine && ctxtdata && !ctxtdata->url.isEmpty()) {
        typeData = engine->typeLoader.get(ctxtdata->url);
        cdata = typeData->compiledData();
    }
    QQmlBinding *rv = cdata ? new QQmlBinding(cdata->primitives.at(id), true, obj, ctxtdata, url, lineNumber, 0, parent) : 0;
    if (cdata)
        cdata->release();
    if (typeData)
        typeData->release();
    return rv;
}

QQmlBinding::QQmlBinding(const QString &str, QObject *obj, QQmlContext *ctxt, 
                                         QObject *parent)
: QQmlExpression(QQmlContextData::get(ctxt), obj, str, *new QQmlBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

QQmlBinding::QQmlBinding(const QString &str, QObject *obj, QQmlContextData *ctxt, 
                                         QObject *parent)
: QQmlExpression(ctxt, obj, str, *new QQmlBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

QQmlBinding::QQmlBinding(const QString &str, bool isRewritten, QObject *obj, 
                                         QQmlContextData *ctxt, 
                                         const QString &url, int lineNumber, int columnNumber,
                                         QObject *parent)
: QQmlExpression(ctxt, obj, str, isRewritten, url, lineNumber, columnNumber, *new QQmlBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

/*!  
    \internal 

    To avoid exposing v8 in the public API, functionPtr must be a pointer to a v8::Handle<v8::Function>.  
    For example:
        v8::Handle<v8::Function> function;
        new QQmlBinding(&function, scope, ctxt);
 */
QQmlBinding::QQmlBinding(void *functionPtr, QObject *obj, QQmlContextData *ctxt, 
                                         QObject *parent)
: QQmlExpression(ctxt, obj, functionPtr, *new QQmlBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

QQmlBinding::~QQmlBinding()
{
}

void QQmlBinding::setTarget(const QQmlProperty &prop)
{
    Q_D(QQmlBinding);
    d->property = prop;
    d->target = d->property.object();
    d->targetProperty = QQmlPropertyPrivate::get(d->property)->core.encodedIndex();

    update();
}

void QQmlBinding::setTarget(QObject *object,
                                    const QQmlPropertyData &core,
                                    QQmlContextData *ctxt)
{
    Q_D(QQmlBinding);
    d->property = QQmlPropertyPrivate::restore(object, core, ctxt);
    d->target = d->property.object();
    d->targetProperty = QQmlPropertyPrivate::get(d->property)->core.encodedIndex();

    update();
}

QQmlProperty QQmlBinding::property() const 
{
   Q_D(const QQmlBinding);
   return d->property; 
}

void QQmlBinding::setEvaluateFlags(EvaluateFlags flags)
{
    Q_D(QQmlBinding);
    d->setRequiresThisObject(flags & RequiresThisObject);
}

QQmlBinding::EvaluateFlags QQmlBinding::evaluateFlags() const
{
    Q_D(const QQmlBinding);
    return d->requiresThisObject()?RequiresThisObject:None;
}

void QQmlBinding::update(QQmlPropertyPrivate::WriteFlags flags)
{
    Q_D(QQmlBinding);

    if (!d->enabled || !d->context() || !d->context()->isValid()) 
        return;

    QQmlTrace trace("General Binding Update");
    trace.addDetail("URL", d->url);
    trace.addDetail("Line", d->line);
    trace.addDetail("Column", d->columnNumber);

    if (!d->updating) {
        QQmlBindingProfiler prof(d->url, d->line, d->column);
        if (prof.enabled)
            prof.addDetail(expression());
        d->updating = true;

        QQmlAbstractExpression::DeleteWatcher watcher(d);

        if (d->property.propertyType() == qMetaTypeId<QQmlBinding *>()) {

            int idx = d->property.index();
            Q_ASSERT(idx != -1);

            QQmlBinding *t = this;
            int status = -1;
            void *a[] = { &t, 0, &status, &flags };
            QMetaObject::metacall(d->property.object(),
                                  QMetaObject::WriteProperty,
                                  idx, a);

        } else {
            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(d->context()->engine);
            ep->referenceScarceResources(); 

            bool isUndefined = false;

            v8::HandleScope handle_scope;
            v8::Context::Scope scope(ep->v8engine()->context());
            v8::Local<v8::Value> result = d->v8value(0, &isUndefined);

            trace.event("writing binding result");

            bool needsErrorData = false;
            if (!watcher.wasDeleted() && !d->hasError())
                needsErrorData = !QQmlPropertyPrivate::writeBinding(d->property, d->context(),
                                                                            d, result,
                                                                            isUndefined, flags);

            if (!watcher.wasDeleted()) {
               
                if (needsErrorData) {
                    QUrl url = QUrl(d->url);
                    int line = d->line;
                    if (url.isEmpty()) url = QUrl(QLatin1String("<Unknown File>"));

                    d->delayedError()->error.setUrl(url);
                    d->delayedError()->error.setLine(line);
                    d->delayedError()->error.setColumn(-1);
                }

                if (d->hasError()) {
                    if (!d->delayedError()->addError(ep)) ep->warning(this->error());
                } else {
                    d->clearError();
                }

            }

            ep->dereferenceScarceResources(); 
        }

        if (!watcher.wasDeleted())
            d->updating = false;
    } else {
        QQmlBindingPrivate::printBindingLoopError(d->property);
    }
}

void QQmlBindingPrivate::printBindingLoopError(QQmlProperty &prop)
{
    qmlInfo(prop.object()) << QQmlBinding::tr("Binding loop detected for property \"%1\"").arg(prop.name());
}

void QQmlBindingPrivate::expressionChanged()
{
    Q_Q(QQmlBinding);
    q->update();
}

void QQmlBinding::setEnabled(bool e, QQmlPropertyPrivate::WriteFlags flags)
{
    Q_D(QQmlBinding);
    d->enabled = e;
    setNotifyOnValueChanged(e);

    if (e) 
        update(flags);
}

bool QQmlBinding::enabled() const
{
    Q_D(const QQmlBinding);

    return d->enabled;
}

QString QQmlBinding::expression() const
{
    return QQmlExpression::expression();
}

int QQmlBinding::propertyIndex() const
{
    Q_D(const QQmlBinding);
    return d->targetProperty;
}

QObject *QQmlBinding::object() const
{
    Q_D(const QQmlBinding);
    return d->target;
}

void QQmlBinding::retargetBinding(QObject *t, int i)
{
    Q_D(QQmlBinding);
    d->target = t;
    d->targetProperty = i;
}

QQmlValueTypeProxyBinding::QQmlValueTypeProxyBinding(QObject *o, int index)
: m_object(o), m_index(index), m_bindings(0)
{
}

QQmlValueTypeProxyBinding::~QQmlValueTypeProxyBinding()
{
    while (m_bindings) {
        QQmlAbstractBinding *binding = m_bindings;
        binding->setEnabled(false, 0);
        binding->destroy();
    }
}

void QQmlValueTypeProxyBinding::setEnabled(bool e, QQmlPropertyPrivate::WriteFlags flags)
{
    if (e) {
        QQmlAbstractBinding *bindings = m_bindings;
        recursiveEnable(bindings, flags);
    } else {
        QQmlAbstractBinding *bindings = m_bindings;
        recursiveDisable(bindings);
    }
}

void QQmlValueTypeProxyBinding::recursiveEnable(QQmlAbstractBinding *b, QQmlPropertyPrivate::WriteFlags flags)
{
    if (!b)
        return;

    recursiveEnable(b->m_nextBinding, flags);

    if (b)
        b->setEnabled(true, flags);
}

void QQmlValueTypeProxyBinding::recursiveDisable(QQmlAbstractBinding *b)
{
    if (!b)
        return;

    recursiveDisable(b->m_nextBinding);

    if (b)
        b->setEnabled(false, 0);
}

void QQmlValueTypeProxyBinding::update(QQmlPropertyPrivate::WriteFlags)
{
}

QQmlAbstractBinding *QQmlValueTypeProxyBinding::binding(int propertyIndex)
{
    QQmlAbstractBinding *binding = m_bindings;
    
    while (binding && binding->propertyIndex() != propertyIndex) 
        binding = binding->m_nextBinding;

    return binding;
}

/*!
Removes a collection of bindings, corresponding to the set bits in \a mask.
*/
void QQmlValueTypeProxyBinding::removeBindings(quint32 mask)
{
    QQmlAbstractBinding *binding = m_bindings;
    while (binding) {
        if (mask & (1 << (binding->propertyIndex() >> 24))) {
            QQmlAbstractBinding *remove = binding;
            binding = remove->m_nextBinding;
            *remove->m_prevBinding = remove->m_nextBinding;
            if (remove->m_nextBinding) remove->m_nextBinding->m_prevBinding = remove->m_prevBinding;
            remove->m_prevBinding = 0;
            remove->m_nextBinding = 0;
            remove->destroy();
        } else {
            binding = binding->m_nextBinding;
        }
    }
}

int QQmlValueTypeProxyBinding::propertyIndex() const
{
    return m_index;
}

QObject *QQmlValueTypeProxyBinding::object() const
{
    return m_object;
}

QT_END_NAMESPACE
