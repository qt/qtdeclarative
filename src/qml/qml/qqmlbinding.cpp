/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlbinding_p.h"

#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlinfo.h"
#include "qqmldata_p.h"
#include <private/qqmlprofiler_p.h>
#include <private/qqmlexpression_p.h>
#include <private/qqmlscriptstring_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlvaluetypewrapper_p.h>

#include <QVariant>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QQmlBinding *QQmlBinding::create(const QQmlPropertyData *property, const QString &str, QObject *obj, QQmlContext *ctxt)
{
    QQmlBinding *b = newBinding(property);
    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(QQmlContextData::get(ctxt));
    b->setScopeObject(obj);

    b->createQmlBinding(b->context(), obj, str, QString(), 0);

    return b;
}

QQmlBinding *QQmlBinding::create(const QQmlPropertyData *property, const QQmlScriptString &script, QObject *obj, QQmlContext *ctxt)
{
    QQmlBinding *b = newBinding(property);

    if (ctxt && !ctxt->isValid())
        return b;

    const QQmlScriptStringPrivate *scriptPrivate = script.d.data();
    if (!ctxt && (!scriptPrivate->context || !scriptPrivate->context->isValid()))
        return b;

    QString url;
    QV4::Function *runtimeFunction = 0;

    QQmlContextData *ctxtdata = QQmlContextData::get(scriptPrivate->context);
    QQmlEnginePrivate *engine = QQmlEnginePrivate::get(scriptPrivate->context->engine());
    if (engine && ctxtdata && !ctxtdata->urlString().isEmpty() && ctxtdata->typeCompilationUnit) {
        url = ctxtdata->urlString();
        if (scriptPrivate->bindingId != QQmlBinding::Invalid)
            runtimeFunction = ctxtdata->typeCompilationUnit->runtimeFunctions.at(scriptPrivate->bindingId);
    }

    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(QQmlContextData::get(ctxt ? ctxt : scriptPrivate->context));
    b->setScopeObject(obj ? obj : scriptPrivate->scope);

    QV4::ExecutionEngine *v4 = QQmlEnginePrivate::get(b->context()->engine)->v4engine();
    if (runtimeFunction) {
        b->m_function.set(v4, QV4::FunctionObject::createQmlFunction(ctxtdata, b->scopeObject(), runtimeFunction));
    } else {
        QString code = scriptPrivate->script;
        b->createQmlBinding(b->context(), b->scopeObject(), code, url, scriptPrivate->lineNumber);
    }

    return b;
}

QQmlBinding *QQmlBinding::create(const QQmlPropertyData *property, const QString &str, QObject *obj, QQmlContextData *ctxt)
{
    QQmlBinding *b = newBinding(property);

    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(ctxt);
    b->setScopeObject(obj);

    b->createQmlBinding(ctxt, obj, str, QString(), 0);

    return b;
}

QQmlBinding *QQmlBinding::create(const QQmlPropertyData *property, const QString &str, QObject *obj,
                                 QQmlContextData *ctxt, const QString &url, quint16 lineNumber,
                                 quint16 columnNumber)
{
    QQmlBinding *b = newBinding(property);

    Q_UNUSED(columnNumber);
    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(ctxt);
    b->setScopeObject(obj);

    b->createQmlBinding(ctxt, obj, str, url, lineNumber);

    return b;
}

QQmlBinding *QQmlBinding::create(const QQmlPropertyData *property, const QV4::Value &functionPtr, QObject *obj, QQmlContextData *ctxt)
{
    QQmlBinding *b = newBinding(property);

    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(ctxt);
    b->setScopeObject(obj);

    b->m_function.set(functionPtr.as<QV4::Object>()->engine(), functionPtr);

    return b;
}

QQmlBinding::~QQmlBinding()
{
}

void QQmlBinding::setNotifyOnValueChanged(bool v)
{
    QQmlJavaScriptExpression::setNotifyOnValueChanged(v);
}

void QQmlBinding::update(QQmlPropertyPrivate::WriteFlags flags)
{
    if (!enabledFlag() || !context() || !context()->isValid())
        return;

    // Check that the target has not been deleted
    if (QQmlData::wasDeleted(targetObject()))
        return;

    // Check for a binding update loop
    if (Q_UNLIKELY(updatingFlag())) {
        QQmlProperty p = QQmlPropertyPrivate::restore(targetObject(), getPropertyData(), 0);
        QQmlAbstractBinding::printBindingLoopError(p);
        return;
    }
    setUpdatingFlag(true);

    DeleteWatcher watcher(this);

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context()->engine);
    QV4::Scope scope(ep->v4engine());
    QV4::ScopedFunctionObject f(scope, m_function.value());
    Q_ASSERT(f);

    QQmlBindingProfiler prof(ep->profiler, this, f);
    doUpdate(this, watcher, flags, scope, f);

    if (!watcher.wasDeleted())
        setUpdatingFlag(false);
}

// QQmlBindingBinding is for target properties which are of type "binding" (instead of, say, int or
// double). The reason for being is that GenericBinding::fastWrite needs a compile-time constant
// expression for the switch for the compiler to generate the optimal code, but
// qMetaTypeId<QQmlBinding *>() needs to be used for the ID. So QQmlBinding::newBinding uses that
// to instantiate this class.
class QQmlBindingBinding: public QQmlBinding
{
protected:
    void doUpdate(QQmlBinding *binding, const DeleteWatcher &,
                  QQmlPropertyPrivate::WriteFlags flags, QV4::Scope &,
                  const QV4::ScopedFunctionObject &) Q_DECL_OVERRIDE Q_DECL_FINAL
    {
        QQmlPropertyData pd = getPropertyData();

        int idx = pd.coreIndex;
        Q_ASSERT(idx != -1);

        int status = -1;
        void *a[] = { &binding, 0, &status, &flags };
        QMetaObject::metacall(*m_target, QMetaObject::WriteProperty, idx, a);
    }
};

#define QUICK_STORE(cpptype, conversion) \
        { \
            cpptype o = (conversion); \
            int status = -1; \
            void *argv[] = { &o, 0, &status, &flags }; \
            QMetaObject::metacall(m_target.data(), QMetaObject::WriteProperty, coreIndex, argv); \
            return true; \
        } \


template<int StaticPropType>
class GenericBinding: public QQmlBinding
{
protected:

    void doUpdate(QQmlBinding *binding, const DeleteWatcher &watcher,
                  QQmlPropertyPrivate::WriteFlags flags, QV4::Scope &scope,
                  const QV4::ScopedFunctionObject &f) Q_DECL_OVERRIDE Q_DECL_FINAL
    {
        auto ep = QQmlEnginePrivate::get(scope.engine);
        ep->referenceScarceResources();

        bool isUndefined = false;

        QV4::ScopedCallData callData(scope);
        binding->QQmlJavaScriptExpression::evaluate(callData, &isUndefined, scope);

        bool error = false;
        if (!watcher.wasDeleted() && isAddedToObject() && !hasError()) {
            if (StaticPropType == QMetaType::UnknownType) {
                error = !write(scope.result, isUndefined, flags);
            } else {
                error = !fastWrite(scope.result, isUndefined, flags);
            }
        }

        if (!watcher.wasDeleted()) {

            if (error) {
                delayedError()->setErrorLocation(f->sourceLocation());
                delayedError()->setErrorObject(m_target.data());
            }

            if (hasError()) {
                if (!delayedError()->addError(ep)) ep->warning(this->error(context()->engine));
            } else {
                clearError();
            }

        }

        ep->dereferenceScarceResources();
    }

private:
    // Returns true if successful, false if an error description was set on expression
    Q_ALWAYS_INLINE bool fastWrite(const QV4::Value &result, bool isUndefined,
                                   QQmlPropertyPrivate::WriteFlags flags)
    {
        int coreIndex = getPropertyCoreIndex();

        Q_ASSERT(m_target.data());

        if (Q_LIKELY(!isUndefined && coreIndex != -1 )) {
            switch (StaticPropType) {
            case QMetaType::Int:
                if (result.isInteger())
                    QUICK_STORE(int, result.integerValue())
                else if (result.isNumber())
                    QUICK_STORE(int, result.doubleValue())
                break;
            case QMetaType::Double:
                if (result.isNumber())
                    QUICK_STORE(double, result.asDouble())
                break;
            case QMetaType::Float:
                if (result.isNumber())
                    QUICK_STORE(float, result.asDouble())
                break;
            case QMetaType::QString:
                if (result.isString())
                    QUICK_STORE(QString, result.toQStringNoThrow())
                break;
            default:
                if (const QV4::QQmlValueTypeWrapper *vtw = result.as<const QV4::QQmlValueTypeWrapper>()) {
                    if (vtw->d()->valueType->typeId == StaticPropType) {
                        return vtw->write(m_target.data(), coreIndex);
                    }
                }
                break;
            }
        }

        return slowWrite(result, isUndefined, flags);
    }
};

// Returns true if successful, false if an error description was set on expression
Q_ALWAYS_INLINE bool QQmlBinding::write(const QV4::Value &result, bool isUndefined,
                                        QQmlPropertyPrivate::WriteFlags flags)
{
    Q_ASSERT(m_target.data());

    int coreIndex = getPropertyCoreIndex();
    int propertyType = getPropertyType();

    Q_ASSERT(m_target.data());

    if (Q_LIKELY(!isUndefined && coreIndex != -1 )) {
        switch (propertyType) {
        case QMetaType::Int:
            if (result.isInteger())
                QUICK_STORE(int, result.integerValue())
            else if (result.isNumber())
                QUICK_STORE(int, result.doubleValue())
            break;
        case QMetaType::Double:
            if (result.isNumber())
                QUICK_STORE(double, result.asDouble())
            break;
        case QMetaType::Float:
            if (result.isNumber())
                QUICK_STORE(float, result.asDouble())
            break;
        case QMetaType::QString:
            if (result.isString())
                QUICK_STORE(QString, result.toQStringNoThrow())
            break;
        default:
            if (const QV4::QQmlValueTypeWrapper *vtw = result.as<const QV4::QQmlValueTypeWrapper>()) {
                if (vtw->d()->valueType->typeId == propertyType) {
                    return vtw->write(m_target.data(), coreIndex);
                }
            }
            break;
        }
    }

    return slowWrite(result, isUndefined, flags);
}
#undef QUICK_STORE

Q_NEVER_INLINE bool QQmlBinding::slowWrite(const QV4::Value &result, bool isUndefined,
                                           QQmlPropertyPrivate::WriteFlags flags)
{
    QQmlPropertyData core = getPropertyData();

    QQmlEngine *engine = context()->engine;
    QV8Engine *v8engine = QQmlEnginePrivate::getV8Engine(engine);

    int type = core.isValueTypeVirtual() ? core.valueTypePropType : core.propType;

    QQmlJavaScriptExpression::DeleteWatcher watcher(this);

    QVariant value;
    bool isVarProperty = core.isVarProperty();

    if (isUndefined) {
    } else if (core.isQList()) {
        value = QV8Engine::getV4(v8engine)->toVariant(result, qMetaTypeId<QList<QObject *> >());
    } else if (result.isNull() && core.isQObject()) {
        value = QVariant::fromValue((QObject *)0);
    } else if (core.propType == qMetaTypeId<QList<QUrl> >()) {
        value = QQmlPropertyPrivate::resolvedUrlSequence(QV8Engine::getV4(v8engine)->toVariant(result, qMetaTypeId<QList<QUrl> >()), context());
    } else if (!isVarProperty && type != qMetaTypeId<QJSValue>()) {
        value = QV8Engine::getV4(v8engine)->toVariant(result, type);
    }

    if (hasError()) {
        return false;
    } else if (isVarProperty) {
        const QV4::FunctionObject *f = result.as<QV4::FunctionObject>();
        if (f && f->isBinding()) {
            // we explicitly disallow this case to avoid confusion.  Users can still store one
            // in an array in a var property if they need to, but the common case is user error.
            delayedError()->setErrorDescription(QLatin1String("Invalid use of Qt.binding() in a binding declaration."));
            return false;
        }

        QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(m_target.data());
        Q_ASSERT(vmemo);
        vmemo->setVMEProperty(core.coreIndex, result);
    } else if (isUndefined && core.isResettable()) {
        void *args[] = { 0 };
        QMetaObject::metacall(m_target.data(), QMetaObject::ResetProperty, core.coreIndex, args);
    } else if (isUndefined && type == qMetaTypeId<QVariant>()) {
        QQmlPropertyPrivate::writeValueProperty(m_target.data(), core, QVariant(), context(), flags);
    } else if (type == qMetaTypeId<QJSValue>()) {
        const QV4::FunctionObject *f = result.as<QV4::FunctionObject>();
        if (f && f->isBinding()) {
            delayedError()->setErrorDescription(QLatin1String("Invalid use of Qt.binding() in a binding declaration."));
            return false;
        }
        QQmlPropertyPrivate::writeValueProperty(m_target.data(), core, QVariant::fromValue(
                               QJSValue(QV8Engine::getV4(v8engine), result.asReturnedValue())),
                           context(), flags);
    } else if (isUndefined) {
        QString errorStr = QLatin1String("Unable to assign [undefined] to ");
        if (!QMetaType::typeName(type))
            errorStr += QLatin1String("[unknown property type]");
        else
            errorStr += QLatin1String(QMetaType::typeName(type));
        delayedError()->setErrorDescription(errorStr);
        return false;
    } else if (const QV4::FunctionObject *f = result.as<QV4::FunctionObject>()) {
        if (f->isBinding())
            delayedError()->setErrorDescription(QLatin1String("Invalid use of Qt.binding() in a binding declaration."));
        else
            delayedError()->setErrorDescription(QLatin1String("Unable to assign a function to a property of any type other than var."));
        return false;
    } else if (!QQmlPropertyPrivate::writeValueProperty(m_target.data(), core, value, context(), flags)) {

        if (watcher.wasDeleted())
            return true;

        const char *valueType = 0;
        const char *propertyType = 0;

        if (value.userType() == QMetaType::QObjectStar) {
            if (QObject *o = *(QObject *const *)value.constData()) {
                valueType = o->metaObject()->className();

                QQmlMetaObject propertyMetaObject = QQmlPropertyPrivate::rawMetaObjectForType(QQmlEnginePrivate::get(engine), type);
                if (!propertyMetaObject.isNull())
                    propertyType = propertyMetaObject.className();
            }
        } else if (value.userType() != QVariant::Invalid) {
            if (value.userType() == QMetaType::VoidStar)
                valueType = "null";
            else
                valueType = QMetaType::typeName(value.userType());
        }

        if (!valueType)
            valueType = "undefined";
        if (!propertyType)
            propertyType = QMetaType::typeName(type);
        if (!propertyType)
            propertyType = "[unknown property type]";

        delayedError()->setErrorDescription(QLatin1String("Unable to assign ") +
                                                        QLatin1String(valueType) +
                                                        QLatin1String(" to ") +
                                                        QLatin1String(propertyType));
        return false;
    }

    return true;
}

QVariant QQmlBinding::evaluate()
{
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context()->engine);
    ep->referenceScarceResources();

    bool isUndefined = false;

    QV4::Scope scope(ep->v4engine());
    QV4::ScopedCallData callData(scope);
    QQmlJavaScriptExpression::evaluate(callData, &isUndefined, scope);

    ep->dereferenceScarceResources();

    return scope.engine->toVariant(scope.result, qMetaTypeId<QList<QObject*> >());
}

QString QQmlBinding::expressionIdentifier()
{
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context()->engine);
    QV4::Scope scope(ep->v4engine());
    QV4::ScopedValue f(scope, m_function.value());
    QV4::Function *function = f->as<QV4::FunctionObject>()->function();

    QString url = function->sourceFile();
    quint16 lineNumber = function->compiledFunction->location.line;
    quint16 columnNumber = function->compiledFunction->location.column;

    return url + QLatin1Char(':') + QString::number(lineNumber) + QLatin1Char(':') + QString::number(columnNumber);
}

void QQmlBinding::expressionChanged()
{
    update();
}

void QQmlBinding::refresh()
{
    update();
}

void QQmlBinding::setEnabled(bool e, QQmlPropertyPrivate::WriteFlags flags)
{
    setEnabledFlag(e);
    setNotifyOnValueChanged(e);

    if (e)
        update(flags);
}

QString QQmlBinding::expression() const
{
    QV4::Scope scope(QQmlEnginePrivate::get(context()->engine)->v4engine());
    QV4::ScopedValue v(scope, m_function.value());
    return v->toQStringNoThrow();
}

void QQmlBinding::setTarget(const QQmlProperty &prop)
{
    setTarget(prop.object(), QQmlPropertyPrivate::get(prop)->core);
}

void QQmlBinding::setTarget(QObject *object, const QQmlPropertyData &core)
{
    m_target = object;

    if (!object) {
        m_targetIndex = -1;
        return;
    }

    QQmlPropertyData pd = core;

    while (pd.isAlias()) {
        int coreIndex = pd.coreIndex;
        int valueTypeIndex = pd.getValueTypeCoreIndex();
        QQmlVMEMetaObject *vme = QQmlVMEMetaObject::getForProperty(object, coreIndex);

        int aValueTypeIndex;
        if (!vme->aliasTarget(coreIndex, &object, &coreIndex, &aValueTypeIndex)) {
            m_target = 0;
            m_targetIndex = -1;
            return;
        }
        if (valueTypeIndex == -1)
            valueTypeIndex = aValueTypeIndex;

        QQmlData *data = QQmlData::get(object, false);
        if (!data || !data->propertyCache) {
            m_target = 0;
            m_targetIndex = -1;
            return;
        }
        QQmlPropertyData *propertyData = data->propertyCache->property(coreIndex);
        Q_ASSERT(propertyData);

        m_target = object;
        pd = *propertyData;
        if (valueTypeIndex != -1) {
            const QMetaObject *valueTypeMetaObject = QQmlValueTypeFactory::metaObjectForMetaType(pd.propType);
            Q_ASSERT(valueTypeMetaObject);
            QMetaProperty vtProp = valueTypeMetaObject->property(valueTypeIndex);
            pd.setFlags(pd.getFlags() | QQmlPropertyData::IsValueTypeVirtual);
            pd.valueTypeFlags = QQmlPropertyData::flagsForProperty(vtProp);
            pd.valueTypePropType = vtProp.userType();
            pd.valueTypeCoreIndex = valueTypeIndex;
        }
    }
    m_targetIndex = pd.encodedIndex();

    QQmlData *data = QQmlData::get(*m_target, true);
    if (!data->propertyCache) {
        data->propertyCache = QQmlEnginePrivate::get(context()->engine)->cache(m_target->metaObject());
        data->propertyCache->addref();
    }
}

QQmlPropertyData QQmlBinding::getPropertyData() const
{
    int coreIndex;
    int valueTypeIndex = QQmlPropertyData::decodeValueTypePropertyIndex(m_targetIndex, &coreIndex);

    QQmlData *data = QQmlData::get(*m_target, false);
    Q_ASSERT(data && data->propertyCache);

    QQmlPropertyData *propertyData = data->propertyCache->property(coreIndex);
    Q_ASSERT(propertyData);

    QQmlPropertyData d = *propertyData;
    if (Q_UNLIKELY(valueTypeIndex != -1)) {
        const QMetaObject *valueTypeMetaObject = QQmlValueTypeFactory::metaObjectForMetaType(d.propType);
        Q_ASSERT(valueTypeMetaObject);
        QMetaProperty vtProp = valueTypeMetaObject->property(valueTypeIndex);
        d.setFlags(d.getFlags() | QQmlPropertyData::IsValueTypeVirtual);
        d.valueTypeFlags = QQmlPropertyData::flagsForProperty(vtProp);
        d.valueTypePropType = vtProp.userType();
        d.valueTypeCoreIndex = valueTypeIndex;
    }
    return d;
}

Q_ALWAYS_INLINE int QQmlBinding::getPropertyCoreIndex() const
{
    int coreIndex;
    int valueTypeIndex = QQmlPropertyData::decodeValueTypePropertyIndex(m_targetIndex, &coreIndex);
    if (valueTypeIndex != -1) {
        return -1;
    } else {
        return coreIndex;
    }
}

int QQmlBinding::getPropertyType() const
{
    int coreIndex;
    int valueTypeIndex = QQmlPropertyData::decodeValueTypePropertyIndex(m_targetIndex, &coreIndex);

    QQmlData *data = QQmlData::get(*m_target, false);
    Q_ASSERT(data && data->propertyCache);

    QQmlPropertyData *propertyData = data->propertyCache->property(coreIndex);
    Q_ASSERT(propertyData);

    if (valueTypeIndex == -1)
        return propertyData->propType;
    else
        return QMetaType::UnknownType;
}

QQmlBinding *QQmlBinding::newBinding(const QQmlPropertyData *property)
{
    const int type = (property && property->isFullyResolved()) ? property->propType : QMetaType::UnknownType;

    if (type == qMetaTypeId<QQmlBinding *>()) {
        return new QQmlBindingBinding;
    } else if (type == QMetaType::Int) {
        return new GenericBinding<QMetaType::Int>;
    } else if (type == QMetaType::Double) {
        return new GenericBinding<QMetaType::Double>;
    } else if (type == QMetaType::Float) {
        return new GenericBinding<QMetaType::Float>;
    } else if (type == QMetaType::QString) {
        return new GenericBinding<QMetaType::QString>;
    } else {
        return new GenericBinding<QMetaType::UnknownType>;
    }
}

QT_END_NAMESPACE
