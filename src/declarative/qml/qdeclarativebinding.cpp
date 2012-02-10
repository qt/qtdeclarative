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

#include "qdeclarativebinding_p.h"
#include "qdeclarativebinding_p_p.h"

#include "qdeclarative.h"
#include "qdeclarativecontext.h"
#include "qdeclarativeinfo.h"
#include "qdeclarativecompiler_p.h"
#include "qdeclarativedata_p.h"
#include <private/qdeclarativeprofilerservice_p.h>
#include <private/qdeclarativetrace_p.h>

#include <QVariant>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QDeclarativeAbstractBinding::QDeclarativeAbstractBinding()
: m_prevBinding(0), m_nextBinding(0)
{
}

QDeclarativeAbstractBinding::~QDeclarativeAbstractBinding()
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
void QDeclarativeAbstractBinding::destroy()
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
void QDeclarativeAbstractBinding::addToObject()
{
    Q_ASSERT(!m_prevBinding);

    QObject *obj = object();
    Q_ASSERT(obj);

    int index = propertyIndex();

    QDeclarativeData *data = QDeclarativeData::get(obj, true);

    if (index & 0xFF000000) {
        // Value type

        int coreIndex = index & 0xFFFFFF;

        // Find the value type proxy (if there is one)
        QDeclarativeValueTypeProxyBinding *proxy = 0;
        if (data->hasBindingBit(coreIndex)) {
            QDeclarativeAbstractBinding *b = data->bindings;
            while (b && b->propertyIndex() != coreIndex)
                b = b->m_nextBinding;
            Q_ASSERT(b && b->bindingType() == QDeclarativeAbstractBinding::ValueTypeProxy);
            proxy = static_cast<QDeclarativeValueTypeProxyBinding *>(b);
        }

        if (!proxy) {
            proxy = new QDeclarativeValueTypeProxyBinding(obj, coreIndex);

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
void QDeclarativeAbstractBinding::removeFromObject()
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
            QDeclarativeData *data = QDeclarativeData::get(obj, false);
            if (data) data->clearBindingBit(index);
        }
    }
}

static void bindingDummyDeleter(QDeclarativeAbstractBinding *)
{
}

QDeclarativeAbstractBinding::Pointer QDeclarativeAbstractBinding::weakPointer()
{
    if (m_mePtr.value().isNull())
        m_mePtr.value() = QSharedPointer<QDeclarativeAbstractBinding>(this, bindingDummyDeleter);

    return m_mePtr.value().toWeakRef();
}

void QDeclarativeAbstractBinding::clear()
{
    if (!m_mePtr.isNull()) {
        **m_mePtr = 0;
        m_mePtr = 0;
    }
}

void QDeclarativeAbstractBinding::retargetBinding(QObject *, int)
{
    qFatal("QDeclarativeAbstractBinding::retargetBinding() called on illegal binding.");
}

QString QDeclarativeAbstractBinding::expression() const
{
    return QLatin1String("<Unknown>");
}

void QDeclarativeAbstractBinding::setEnabled(bool enabled, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (enabled) update(flags);
}

QDeclarativeBinding::Identifier QDeclarativeBinding::Invalid = -1;

void QDeclarativeBindingPrivate::refresh()
{
    Q_Q(QDeclarativeBinding);
    q->update();
}

QDeclarativeBindingPrivate::QDeclarativeBindingPrivate()
: updating(false), enabled(false), target(), targetProperty(0)
{
}

QDeclarativeBindingPrivate::~QDeclarativeBindingPrivate()
{
}

QDeclarativeBinding *
QDeclarativeBinding::createBinding(Identifier id, QObject *obj, QDeclarativeContext *ctxt,
                                   const QString &url, int lineNumber, QObject *parent)
{
    if (id < 0)
        return 0;

    QDeclarativeContextData *ctxtdata = QDeclarativeContextData::get(ctxt);

    QDeclarativeEnginePrivate *engine = QDeclarativeEnginePrivate::get(ctxt->engine());
    QDeclarativeCompiledData *cdata = 0;
    QDeclarativeTypeData *typeData = 0;
    if (engine && ctxtdata && !ctxtdata->url.isEmpty()) {
        typeData = engine->typeLoader.get(ctxtdata->url);
        cdata = typeData->compiledData();
    }
    QDeclarativeBinding *rv = cdata ? new QDeclarativeBinding(cdata->primitives.at(id), true, obj, ctxtdata, url, lineNumber, 0, parent) : 0;
    if (cdata)
        cdata->release();
    if (typeData)
        typeData->release();
    return rv;
}

QDeclarativeBinding::QDeclarativeBinding(const QString &str, QObject *obj, QDeclarativeContext *ctxt, 
                                         QObject *parent)
: QDeclarativeExpression(QDeclarativeContextData::get(ctxt), obj, str, *new QDeclarativeBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

QDeclarativeBinding::QDeclarativeBinding(const QString &str, QObject *obj, QDeclarativeContextData *ctxt, 
                                         QObject *parent)
: QDeclarativeExpression(ctxt, obj, str, *new QDeclarativeBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

QDeclarativeBinding::QDeclarativeBinding(const QString &str, bool isRewritten, QObject *obj, 
                                         QDeclarativeContextData *ctxt, 
                                         const QString &url, int lineNumber, int columnNumber,
                                         QObject *parent)
: QDeclarativeExpression(ctxt, obj, str, isRewritten, url, lineNumber, columnNumber, *new QDeclarativeBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

/*!  
    \internal 

    To avoid exposing v8 in the public API, functionPtr must be a pointer to a v8::Handle<v8::Function>.  
    For example:
        v8::Handle<v8::Function> function;
        new QDeclarativeBinding(&function, scope, ctxt);
 */
QDeclarativeBinding::QDeclarativeBinding(void *functionPtr, QObject *obj, QDeclarativeContextData *ctxt, 
                                         QObject *parent)
: QDeclarativeExpression(ctxt, obj, functionPtr, *new QDeclarativeBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

QDeclarativeBinding::~QDeclarativeBinding()
{
}

void QDeclarativeBinding::setTarget(const QDeclarativeProperty &prop)
{
    Q_D(QDeclarativeBinding);
    d->property = prop;
    d->target = d->property.object();
    d->targetProperty = QDeclarativePropertyPrivate::get(d->property)->core.encodedIndex();

    update();
}

void QDeclarativeBinding::setTarget(QObject *object,
                                    const QDeclarativePropertyData &core,
                                    QDeclarativeContextData *ctxt)
{
    Q_D(QDeclarativeBinding);
    d->property = QDeclarativePropertyPrivate::restore(object, core, ctxt);
    d->target = d->property.object();
    d->targetProperty = QDeclarativePropertyPrivate::get(d->property)->core.encodedIndex();

    update();
}

QDeclarativeProperty QDeclarativeBinding::property() const 
{
   Q_D(const QDeclarativeBinding);
   return d->property; 
}

void QDeclarativeBinding::setEvaluateFlags(EvaluateFlags flags)
{
    Q_D(QDeclarativeBinding);
    d->setRequiresThisObject(flags & RequiresThisObject);
}

QDeclarativeBinding::EvaluateFlags QDeclarativeBinding::evaluateFlags() const
{
    Q_D(const QDeclarativeBinding);
    return d->requiresThisObject()?RequiresThisObject:None;
}

void QDeclarativeBinding::update(QDeclarativePropertyPrivate::WriteFlags flags)
{
    Q_D(QDeclarativeBinding);

    if (!d->enabled || !d->context() || !d->context()->isValid()) 
        return;

    QDeclarativeTrace trace("General Binding Update");
    trace.addDetail("URL", d->url);
    trace.addDetail("Line", d->line);
    trace.addDetail("Column", d->columnNumber);

    if (!d->updating) {
        QDeclarativeBindingProfiler prof(d->url, d->line, d->column);
        prof.addDetail(expression());
        d->updating = true;

        QDeclarativeAbstractExpression::DeleteWatcher watcher(d);

        if (d->property.propertyType() == qMetaTypeId<QDeclarativeBinding *>()) {

            int idx = d->property.index();
            Q_ASSERT(idx != -1);

            QDeclarativeBinding *t = this;
            int status = -1;
            void *a[] = { &t, 0, &status, &flags };
            QMetaObject::metacall(d->property.object(),
                                  QMetaObject::WriteProperty,
                                  idx, a);

        } else {
            QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(d->context()->engine);
            ep->referenceScarceResources(); 

            bool isUndefined = false;

            v8::HandleScope handle_scope;
            v8::Context::Scope scope(ep->v8engine()->context());
            v8::Local<v8::Value> result = d->v8value(0, &isUndefined);

            trace.event("writing binding result");

            bool needsErrorData = false;
            if (!watcher.wasDeleted() && !d->hasError())
                needsErrorData = !QDeclarativePropertyPrivate::writeBinding(d->property, d->context(),
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
        QDeclarativeBindingPrivate::printBindingLoopError(d->property);
    }
}

void QDeclarativeBindingPrivate::printBindingLoopError(QDeclarativeProperty &prop)
{
    qmlInfo(prop.object()) << QDeclarativeBinding::tr("Binding loop detected for property \"%1\"").arg(prop.name());
}

void QDeclarativeBindingPrivate::expressionChanged()
{
    Q_Q(QDeclarativeBinding);
    q->update();
}

void QDeclarativeBinding::setEnabled(bool e, QDeclarativePropertyPrivate::WriteFlags flags)
{
    Q_D(QDeclarativeBinding);
    d->enabled = e;
    setNotifyOnValueChanged(e);

    if (e) 
        update(flags);
}

bool QDeclarativeBinding::enabled() const
{
    Q_D(const QDeclarativeBinding);

    return d->enabled;
}

QString QDeclarativeBinding::expression() const
{
    return QDeclarativeExpression::expression();
}

int QDeclarativeBinding::propertyIndex() const
{
    Q_D(const QDeclarativeBinding);
    return d->targetProperty;
}

QObject *QDeclarativeBinding::object() const
{
    Q_D(const QDeclarativeBinding);
    return d->target;
}

void QDeclarativeBinding::retargetBinding(QObject *t, int i)
{
    Q_D(QDeclarativeBinding);
    d->target = t;
    d->targetProperty = i;
}

QDeclarativeValueTypeProxyBinding::QDeclarativeValueTypeProxyBinding(QObject *o, int index)
: m_object(o), m_index(index), m_bindings(0)
{
}

QDeclarativeValueTypeProxyBinding::~QDeclarativeValueTypeProxyBinding()
{
    while (m_bindings) {
        QDeclarativeAbstractBinding *binding = m_bindings;
        binding->setEnabled(false, 0);
        binding->destroy();
    }
}

void QDeclarativeValueTypeProxyBinding::setEnabled(bool e, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (e) {
        QDeclarativeAbstractBinding *bindings = m_bindings;
        recursiveEnable(bindings, flags);
    } else {
        QDeclarativeAbstractBinding *bindings = m_bindings;
        recursiveDisable(bindings);
    }
}

void QDeclarativeValueTypeProxyBinding::recursiveEnable(QDeclarativeAbstractBinding *b, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (!b)
        return;

    recursiveEnable(b->m_nextBinding, flags);

    if (b)
        b->setEnabled(true, flags);
}

void QDeclarativeValueTypeProxyBinding::recursiveDisable(QDeclarativeAbstractBinding *b)
{
    if (!b)
        return;

    recursiveDisable(b->m_nextBinding);

    if (b)
        b->setEnabled(false, 0);
}

void QDeclarativeValueTypeProxyBinding::update(QDeclarativePropertyPrivate::WriteFlags)
{
}

QDeclarativeAbstractBinding *QDeclarativeValueTypeProxyBinding::binding(int propertyIndex)
{
    QDeclarativeAbstractBinding *binding = m_bindings;
    
    while (binding && binding->propertyIndex() != propertyIndex) 
        binding = binding->m_nextBinding;

    return binding;
}

/*!
Removes a collection of bindings, corresponding to the set bits in \a mask.
*/
void QDeclarativeValueTypeProxyBinding::removeBindings(quint32 mask)
{
    QDeclarativeAbstractBinding *binding = m_bindings;
    while (binding) {
        if (mask & (1 << (binding->propertyIndex() >> 24))) {
            QDeclarativeAbstractBinding *remove = binding;
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

int QDeclarativeValueTypeProxyBinding::propertyIndex() const
{
    return m_index;
}

QObject *QDeclarativeValueTypeProxyBinding::object() const
{
    return m_object;
}

QT_END_NAMESPACE
