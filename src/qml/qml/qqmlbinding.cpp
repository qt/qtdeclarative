// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlbinding_p.h"

#include "qqmlcontext.h"
#include "qqmldata_p.h"

#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qqmldebugconnector_p.h>

#include <private/qqmlprofiler_p.h>
#include <private/qqmlexpression_p.h>
#include <private/qqmlscriptstring_p.h>
#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4variantobject_p.h>
#include <private/qv4jscall_p.h>
#include <private/qjsvalue_p.h>

#include <qtqml_tracepoints_p.h>

#include <QVariant>
#include <QtCore/qdebug.h>
#include <QVector>

QT_BEGIN_NAMESPACE

Q_TRACE_POINT(qtqml, QQmlBinding_entry, const QQmlEngine *engine, const QString &function, const QString &fileName, int line, int column)
Q_TRACE_POINT(qtqml, QQmlBinding_exit)

QQmlBinding *QQmlBinding::create(const QQmlPropertyData *property, const QQmlScriptString &script, QObject *obj, QQmlContext *ctxt)
{
    QQmlBinding *b = newBinding(property);

    if (ctxt && !ctxt->isValid())
        return b;

    const QQmlScriptStringPrivate *scriptPrivate = script.d.data();
    if (!ctxt && (!scriptPrivate->context || !scriptPrivate->context->isValid()))
        return b;

    QString url;
    QV4::Function *runtimeFunction = nullptr;

    QQmlRefPointer<QQmlContextData> ctxtdata = QQmlContextData::get(scriptPrivate->context);
    QQmlEnginePrivate *engine = QQmlEnginePrivate::get(scriptPrivate->context->engine());
    if (engine && ctxtdata && !ctxtdata->urlString().isEmpty() && ctxtdata->typeCompilationUnit()) {
        url = ctxtdata->urlString();
        if (scriptPrivate->bindingId != QQmlBinding::Invalid)
            runtimeFunction = ctxtdata->typeCompilationUnit()->runtimeFunctions.at(scriptPrivate->bindingId);
    }

    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(QQmlContextData::get(ctxt ? ctxt : scriptPrivate->context));
    b->setScopeObject(obj ? obj : scriptPrivate->scope);

    QV4::ExecutionEngine *v4 = b->engine()->handle();
    if (runtimeFunction) {
        QV4::Scope scope(v4);
        QV4::Scoped<QV4::QmlContext> qmlContext(scope, QV4::QmlContext::create(v4->rootContext(), ctxtdata, b->scopeObject()));
        b->setupFunction(qmlContext, runtimeFunction);
    } else {
        QString code = scriptPrivate->script;
        b->createQmlBinding(b->context(), b->scopeObject(), code, url, scriptPrivate->lineNumber);
    }

    return b;
}

QQmlSourceLocation QQmlBinding::sourceLocation() const
{
    if (m_sourceLocation)
        return *m_sourceLocation;
    return QQmlJavaScriptExpression::sourceLocation();
}

void QQmlBinding::setSourceLocation(const QQmlSourceLocation &location)
{
    if (m_sourceLocation)
        delete m_sourceLocation;
    m_sourceLocation = new QQmlSourceLocation(location);
}


QQmlBinding *QQmlBinding::create(
        const QQmlPropertyData *property, const QString &str, QObject *obj,
        const QQmlRefPointer<QQmlContextData> &ctxt, const QString &url, quint16 lineNumber)
{
    QQmlBinding *b = newBinding(property);

    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(ctxt);
    b->setScopeObject(obj);

    b->createQmlBinding(ctxt, obj, str, url, lineNumber);

    return b;
}

QQmlBinding *QQmlBinding::create(
        const QQmlPropertyData *property, QV4::Function *function, QObject *obj,
        const QQmlRefPointer<QQmlContextData> &ctxt, QV4::ExecutionContext *scope)
{
    return create(property ? property->propType() : QMetaType(), function, obj, ctxt, scope);
}

QQmlBinding *QQmlBinding::create(QMetaType propertyType, QV4::Function *function, QObject *obj,
                                 const QQmlRefPointer<QQmlContextData> &ctxt,
                                 QV4::ExecutionContext *scope)
{
    QQmlBinding *b = newBinding(propertyType);

    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(ctxt);
    b->setScopeObject(obj);

    Q_ASSERT(scope);
    b->setupFunction(scope, function);

    return b;
}

QQmlBinding::~QQmlBinding()
{
    delete m_sourceLocation;
}

void QQmlBinding::update(QQmlPropertyData::WriteFlags flags)
{
    if (!enabledFlag() || !hasValidContext())
        return;

    // Check that the target has not been deleted
    if (QQmlData::wasDeleted(targetObject()))
        return;

    // Check for a binding update loop
    if (Q_UNLIKELY(updatingFlag())) {
        const QQmlPropertyData *d = nullptr;
        QQmlPropertyData vtd;
        getPropertyData(&d, &vtd);
        Q_ASSERT(d);
        QQmlProperty p = QQmlPropertyPrivate::restore(targetObject(), *d, &vtd, nullptr);
        QQmlAbstractBinding::printBindingLoopError(p);
        return;
    }
    setUpdatingFlag(true);

    DeleteWatcher watcher(this);

    QQmlEngine *qmlEngine = engine();
    QV4::Scope scope(qmlEngine->handle());

    if (canUseAccessor())
        flags.setFlag(QQmlPropertyData::BypassInterceptor);

    Q_TRACE_SCOPE(QQmlBinding, qmlEngine, function() ? function()->name()->toQString() : QString(),
                  sourceLocation().sourceFile, sourceLocation().line, sourceLocation().column);
    QQmlBindingProfiler prof(QQmlEnginePrivate::get(qmlEngine)->profiler, function());
    doUpdate(watcher, flags, scope);

    if (!watcher.wasDeleted())
        setUpdatingFlag(false);
}

QV4::ReturnedValue QQmlBinding::evaluate(bool *isUndefined)
{
    QV4::ExecutionEngine *v4 = engine()->handle();
    int argc = 0;
    const QV4::Value *argv = nullptr;
    const QV4::Value *thisObject = nullptr;
    QV4::BoundFunction *b = nullptr;
    if ((b = static_cast<QV4::BoundFunction *>(m_boundFunction.valueRef()))) {
        QV4::Heap::MemberData *args = b->boundArgs();
        if (args) {
            argc = args->values.size;
            argv = args->values.data();
        }
        thisObject = &b->d()->boundThis;
    }
    QV4::Scope scope(v4);
    QV4::JSCallData jsCall(thisObject, argv, argc);

    return QQmlJavaScriptExpression::evaluate(jsCall.callData(scope), isUndefined);
}

template<int StaticPropType>
class GenericBinding: public QQmlBinding
{
protected:
    // Returns true if successful, false if an error description was set on expression
    Q_ALWAYS_INLINE bool write(void *result, QMetaType type, bool isUndefined,
                               QQmlPropertyData::WriteFlags flags) override final
    {
        const QQmlPropertyData *pd;
        QQmlPropertyData vpd;
        getPropertyData(&pd, &vpd);
        Q_ASSERT(pd);

        if (isUndefined || vpd.isValid())
            return slowWrite(*pd, vpd, result, type, isUndefined, flags);

        if ((StaticPropType == QMetaType::UnknownType && pd->propType() == type)
                || StaticPropType == type.id()) {
            Q_ASSERT(targetObject());
            return pd->writeProperty(targetObject(), result, flags);
        }

        // If the type didn't match, we need to do JavaScript conversion. This should be rare.
        return write(engine()->handle()->metaTypeToJS(type, result), isUndefined, flags);
    }

    // Returns true if successful, false if an error description was set on expression
    Q_ALWAYS_INLINE bool write(const QV4::Value &result, bool isUndefined,
                               QQmlPropertyData::WriteFlags flags) override final
    {
        Q_ASSERT(targetObject());

        const QQmlPropertyData *pd;
        QQmlPropertyData vpd;
        getPropertyData(&pd, &vpd);
        Q_ASSERT(pd);

        int propertyType = StaticPropType; // If the binding is specialized to a type, the if and switch below will be constant-folded.
        if (propertyType == QMetaType::UnknownType)
            propertyType = pd->propType().id();

        if (Q_LIKELY(!isUndefined && !vpd.isValid())) {
            switch (propertyType) {
            case QMetaType::Bool:
                if (result.isBoolean())
                    return doStore<bool>(result.booleanValue(), pd, flags);
                else
                    return doStore<bool>(result.toBoolean(), pd, flags);
            case QMetaType::Int:
                if (result.isInteger())
                    return doStore<int>(result.integerValue(), pd, flags);
                else if (result.isNumber()) {
                    return doStore<int>(result.toInt32(), pd, flags);
                }
                break;
            case QMetaType::Double:
                if (result.isNumber())
                    return doStore<double>(result.asDouble(), pd, flags);
                break;
            case QMetaType::Float:
                if (result.isNumber())
                    return doStore<float>(result.asDouble(), pd, flags);
                break;
            case QMetaType::QString:
                if (result.isString())
                    return doStore<QString>(result.toQStringNoThrow(), pd, flags);
                break;
            default:
                if (const QV4::QQmlValueTypeWrapper *vtw = result.as<const QV4::QQmlValueTypeWrapper>()) {
                    if (vtw->d()->metaType() == pd->propType()) {
                        return vtw->write(m_target.data(), pd->coreIndex());
                    }
                }
                break;
            }
        }

        return slowWrite(*pd, vpd, result, isUndefined, flags);
    }

    template <typename T>
    Q_ALWAYS_INLINE bool doStore(T value, const QQmlPropertyData *pd, QQmlPropertyData::WriteFlags flags) const
    {
        void *o = &value;
        return pd->writeProperty(targetObject(), o, flags);
    }
};

class QQmlTranslationBinding : public GenericBinding<QMetaType::QString>, public QPropertyObserver {
public:
    QQmlTranslationBinding(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit)
        : QPropertyObserver(&QQmlTranslationBinding::onLanguageChange)
    {
        setCompilationUnit(compilationUnit);
        setSource(QQmlEnginePrivate::get(compilationUnit->engine)->translationLanguage);
    }

    virtual QString bindingValue() const = 0;

    static void onLanguageChange(QPropertyObserver *observer, QUntypedPropertyData *)
    { static_cast<QQmlTranslationBinding *>(observer)->update(); }

    void doUpdate(const DeleteWatcher &watcher,
                  QQmlPropertyData::WriteFlags flags, QV4::Scope &scope) override final
    {
        if (watcher.wasDeleted())
            return;

        if (!isAddedToObject() || hasError())
            return;

        const QString result = this->bindingValue();

        Q_ASSERT(targetObject());

        const QQmlPropertyData *pd;
        QQmlPropertyData vpd;
        getPropertyData(&pd, &vpd);
        Q_ASSERT(pd);
        if (pd->propType().id() == QMetaType::QString) {
            doStore(result, pd, flags);
        } else {
            QV4::ScopedString value(scope, scope.engine->newString(result));
            slowWrite(*pd, vpd, value, /*isUndefined*/false, flags);
        }
    }

    bool hasDependencies() const override final { return true; }
};

class QQmlTranslationBindingFromBinding : public QQmlTranslationBinding
{
    const QV4::CompiledData::Binding *m_binding;

public:
    QQmlTranslationBindingFromBinding(
            const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
            const QV4::CompiledData::Binding *binding)
        : QQmlTranslationBinding(compilationUnit), m_binding(binding)
    {
    }

    QString bindingValue() const override
    {
        return this->m_compilationUnit->bindingValueAsString(m_binding);
    }

    QQmlSourceLocation sourceLocation() const override final
    {
        return QQmlSourceLocation(m_compilationUnit->fileName(), m_binding->valueLocation.line(),
                                  m_binding->valueLocation.column());
    }
};

class QQmlTranslationBindingFromTranslationInfo : public QQmlTranslationBinding
{
    QQmlTranslation m_translationData;

    quint16 m_line;
    quint16 m_column;

public:
    QQmlTranslationBindingFromTranslationInfo(
            const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
            const QQmlTranslation &translationData, quint16 line, quint16 column)
        : QQmlTranslationBinding(compilationUnit),
          m_translationData(translationData),
          m_line(line),
          m_column(column)
    {
    }

    virtual QString bindingValue() const override { return m_translationData.translate(); }

    QQmlSourceLocation sourceLocation() const override final
    {
        return QQmlSourceLocation(m_compilationUnit->fileName(), m_line, m_column);
    }
};

QQmlBinding *QQmlBinding::createTranslationBinding(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
        const QV4::CompiledData::Binding *binding, QObject *obj,
        const QQmlRefPointer<QQmlContextData> &ctxt)
{
    QQmlTranslationBinding *b = new QQmlTranslationBindingFromBinding(unit, binding);

    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(ctxt);
    b->setScopeObject(obj);
#if QT_CONFIG(translation) && QT_CONFIG(qml_debug)
    if (QQmlDebugTranslationService *service
                 = QQmlDebugConnector::service<QQmlDebugTranslationService>()) {
        service->foundTranslationBinding(
                TranslationBindingInformation::create(unit, binding, b->scopeObject(), ctxt));
    }
#endif
    return b;
}

QQmlBinding *QQmlBinding::createTranslationBinding(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
        const QQmlRefPointer<QQmlContextData> &ctxt, const QString &propertyName,
        const QQmlTranslation &translationData, const QQmlSourceLocation &location, QObject *obj)
{
    QQmlTranslationBinding *b = new QQmlTranslationBindingFromTranslationInfo(
            unit, translationData, location.column, location.line);

    b->setNotifyOnValueChanged(true);
    b->QQmlJavaScriptExpression::setContext(ctxt);
    b->setScopeObject(obj);

#if QT_CONFIG(translation) && QT_CONFIG(qml_debug)
    QString originString;
    if (QQmlDebugTranslationService *service =
                QQmlDebugConnector::service<QQmlDebugTranslationService>()) {
        service->foundTranslationBinding({ unit, b->scopeObject(), ctxt,

                                           propertyName, translationData,

                                           location.line, location.column });
    }
#endif
    return b;
}

bool QQmlBinding::slowWrite(
        const QQmlPropertyData &core, const QQmlPropertyData &valueTypeData, const void *result,
        QMetaType resultType, bool isUndefined, QQmlPropertyData::WriteFlags flags)
{
    // The logic in this method is obscure. It follows what the other slowWrite does, minus the type
    // conversions and the checking for binding functions. If you're writing a C++ type, and
    // you're passing a binding function wrapped into QJSValue, you probably want it to be assigned.

    if (hasError())
        return false;

    QQmlEngine *qmlEngine = engine();
    const QMetaType metaType = valueTypeData.isValid() ? valueTypeData.propType() : core.propType();
    QQmlJavaScriptExpression::DeleteWatcher watcher(this);

    if (core.isVarProperty()) {
        QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(m_target.data());
        Q_ASSERT(vmemo);
        vmemo->setVMEProperty(core.coreIndex(),
                              qmlEngine->handle()->metaTypeToJS(resultType, result));
    } else if (isUndefined && core.isResettable()) {
        void *args[] = { nullptr };
        QMetaObject::metacall(m_target.data(), QMetaObject::ResetProperty, core.coreIndex(), args);
    } else if (isUndefined && metaType == QMetaType::fromType<QVariant>()) {
        QQmlPropertyPrivate::writeValueProperty(
                    m_target.data(), core, valueTypeData, QVariant(), context(), flags);
    } else if (metaType == QMetaType::fromType<QJSValue>()) {
        QQmlPropertyPrivate::writeValueProperty(
                    m_target.data(), core, valueTypeData,
                    QVariant(resultType, result), context(), flags);
    } else if (isUndefined) {
        const char *name = metaType.name();
        const QString typeName = name
                ? QString::fromUtf8(name)
                : QStringLiteral("[unknown property type]");
        delayedError()->setErrorDescription(
                    QStringLiteral("Unable to assign [undefined] to ") + typeName);
        return false;
    } else if (!QQmlPropertyPrivate::writeValueProperty(
                   m_target.data(), core, valueTypeData, QVariant(resultType, result),
                   context(), flags)) {
        if (watcher.wasDeleted())
            return true;
        handleWriteError(result, resultType, metaType);
        return false;
    }

    return true;
}

Q_NEVER_INLINE bool QQmlBinding::slowWrite(const QQmlPropertyData &core,
                                           const QQmlPropertyData &valueTypeData,
                                           const QV4::Value &result,
                                           bool isUndefined, QQmlPropertyData::WriteFlags flags)
{
    const QMetaType metaType = valueTypeData.isValid() ? valueTypeData.propType() : core.propType();
    const int type = metaType.id();

    QQmlJavaScriptExpression::DeleteWatcher watcher(this);

    QVariant value;
    bool isVarProperty = core.isVarProperty();

    if (isUndefined) {
    } else if (core.isQList()) {
        if (core.propType().flags() & QMetaType::IsQmlList)
            value = QV4::ExecutionEngine::toVariant(result, QMetaType::fromType<QList<QObject*>>());
        else
            value = QV4::ExecutionEngine::toVariant(result, core.propType());
    } else if (result.isNull() && core.isQObject()) {
        value = QVariant::fromValue((QObject *)nullptr);
    } else if (core.propType() == QMetaType::fromType<QList<QUrl>>()) {
        const QVariant resultVariant
                = QV4::ExecutionEngine::toVariant(result, QMetaType::fromType<QList<QUrl>>());
        value = QVariant::fromValue(QQmlPropertyPrivate::resolveUrlsOnAssignment()
                                    ? QQmlPropertyPrivate::urlSequence(resultVariant, context())
                                    : QQmlPropertyPrivate::urlSequence(resultVariant));
    } else if (!isVarProperty && metaType != QMetaType::fromType<QJSValue>()) {
        value = QV4::ExecutionEngine::toVariant(result, metaType);
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
        vmemo->setVMEProperty(core.coreIndex(), result);
    } else if (isUndefined
               && (valueTypeData.isValid() ? valueTypeData.isResettable() : core.isResettable())) {
        QQmlPropertyPrivate::resetValueProperty(
                    m_target.data(), core, valueTypeData, context(), flags);
    } else if (isUndefined && type == QMetaType::QVariant) {
        QQmlPropertyPrivate::writeValueProperty(m_target.data(), core, valueTypeData, QVariant(), context(), flags);
    } else if (metaType == QMetaType::fromType<QJSValue>()) {
        const QV4::FunctionObject *f = result.as<QV4::FunctionObject>();
        if (f && f->isBinding()) {
            delayedError()->setErrorDescription(QLatin1String("Invalid use of Qt.binding() in a binding declaration."));
            return false;
        }
        QQmlPropertyPrivate::writeValueProperty(
                    m_target.data(), core, valueTypeData,
                    QVariant::fromValue(QJSValuePrivate::fromReturnedValue(result.asReturnedValue())),
                    context(), flags);
    } else if (isUndefined) {
        const char *name = QMetaType(type).name();
        const QLatin1String typeName(name ? name : "[unknown property type]");
        delayedError()->setErrorDescription(QLatin1String("Unable to assign [undefined] to ")
                                            + typeName);
        return false;
    } else if (const QV4::FunctionObject *f = result.as<QV4::FunctionObject>()) {
        if (f->isBinding())
            delayedError()->setErrorDescription(QLatin1String("Invalid use of Qt.binding() in a binding declaration."));
        else
            delayedError()->setErrorDescription(QLatin1String("Unable to assign a function to a property of any type other than var."));
        return false;
    } else if (!QQmlPropertyPrivate::writeValueProperty(m_target.data(), core, valueTypeData, value, context(), flags)) {

        if (watcher.wasDeleted())
            return true;
        handleWriteError(value.constData(), value.metaType(), metaType);
        return false;
    }

    return true;
}

void QQmlBinding::handleWriteError(const void *result, QMetaType resultType, QMetaType metaType)
{
    const char *valueType = nullptr;
    const char *propertyType = nullptr;

    if (resultType.flags() & QMetaType::PointerToQObject) {
        if (QObject *o = *(QObject *const *)result) {
            valueType = o->metaObject()->className();
            QQmlMetaObject propertyMetaObject = QQmlPropertyPrivate::rawMetaObjectForType(metaType);
            if (!propertyMetaObject.isNull())
                propertyType = propertyMetaObject.className();
        }
    } else if (resultType.isValid()) {
        if (resultType == QMetaType::fromType<std::nullptr_t>()
                || resultType == QMetaType::fromType<void *>()) {
            valueType = "null";
        } else {
            valueType = resultType.name();
        }
    }

    if (!valueType)
        valueType = "undefined";
    if (!propertyType)
        propertyType = metaType.name();
    if (!propertyType)
        propertyType = "[unknown property type]";

    delayedError()->setErrorDescription(QStringLiteral("Unable to assign ")
                                        + QString::fromUtf8(valueType)
                                        + QStringLiteral(" to ")
                                        + QString::fromUtf8(propertyType));
}

QVariant QQmlBinding::evaluate()
{
    QQmlEngine *qmlEngine = engine();
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(qmlEngine);
    ep->referenceScarceResources();

    bool isUndefined = false;

    QV4::Scope scope(qmlEngine->handle());
    QV4::ScopedValue result(scope, QQmlJavaScriptExpression::evaluate(&isUndefined));

    ep->dereferenceScarceResources();

    return QV4::ExecutionEngine::toVariant(result, QMetaType::fromType<QList<QObject*> >());
}

void QQmlBinding::expressionChanged()
{
    update();
}

void QQmlBinding::refresh()
{
    update();
}

void QQmlBinding::setEnabled(bool e, QQmlPropertyData::WriteFlags flags)
{
    const bool wasEnabled = enabledFlag();
    setEnabledFlag(e);
    setNotifyOnValueChanged(e);
    updateCanUseAccessor();

    if (e && !wasEnabled)
        update(flags);
}

QString QQmlBinding::expression() const
{
    return QStringLiteral("function() { [native code] }");
}

QVector<QQmlProperty> QQmlBinding::dependencies() const
{
    QVector<QQmlProperty> dependencies;
    if (!m_target.data())
        return dependencies;

    for (QQmlJavaScriptExpressionGuard *guard = activeGuards.first(); guard; guard = activeGuards.next(guard)) {
        if (guard->signalIndex() == -1) // guard's sender is a QQmlNotifier, not a QObject*.
            continue;

        QObject *senderObject = guard->senderAsObject();
        if (!senderObject)
            continue;

        const QMetaObject *senderMeta = senderObject->metaObject();
        if (!senderMeta)
            continue;

        for (int i = 0; i < senderMeta->propertyCount(); i++) {
            QMetaProperty property = senderMeta->property(i);
            if (property.notifySignalIndex() == QMetaObjectPrivate::signal(senderMeta, guard->signalIndex()).methodIndex()) {
                dependencies.push_back(QQmlProperty(senderObject, QString::fromUtf8(property.name())));
            }
        }
    }

    for (auto trigger = qpropertyChangeTriggers; trigger; trigger = trigger->next) {
        QMetaProperty prop = trigger->property();
        if (prop.isValid())
            dependencies.push_back(QQmlProperty(trigger->target, QString::fromUtf8(prop.name())));
    }

    return dependencies;
}

bool QQmlBinding::hasDependencies() const
{
    return !activeGuards.isEmpty() || qpropertyChangeTriggers;
}

void QQmlBinding::doUpdate(const DeleteWatcher &watcher, QQmlPropertyData::WriteFlags flags, QV4::Scope &scope)
{
    auto ep = QQmlEnginePrivate::get(scope.engine);
    ep->referenceScarceResources();

    bool error = false;
    auto canWrite = [&]() { return !watcher.wasDeleted() && isAddedToObject() && !hasError(); };
    const QV4::Function *v4Function = function();
    if (v4Function && v4Function->kind == QV4::Function::AotCompiled && !hasBoundFunction()) {
        const auto returnType = v4Function->aotCompiledFunction->returnType;
        if (returnType == QMetaType::fromType<QVariant>()) {
            // It expects uninitialized memory
            Q_ALLOCA_VAR(QVariant, result, sizeof(QVariant));
            const bool isUndefined = !evaluate(result, returnType);
            if (canWrite())
                error = !write(result->data(), result->metaType(), isUndefined, flags);
            result->~QVariant();
        } else {
            const auto size = returnType.sizeOf();
            if (Q_LIKELY(size > 0)) {
                Q_ALLOCA_VAR(void, result, size);
                const bool isUndefined = !evaluate(result, returnType);
                if (canWrite())
                    error = !write(result, returnType, isUndefined, flags);
                returnType.destruct(result);
            } else if (canWrite()) {
                error = !write(QV4::Encode::undefined(), true, flags);
            }
        }
    } else {
        bool isUndefined = false;
        QV4::ScopedValue result(scope, evaluate(&isUndefined));
        if (canWrite())
            error = !write(result, isUndefined, flags);
    }

    if (!watcher.wasDeleted()) {

        if (error) {
            delayedError()->setErrorLocation(sourceLocation());
            delayedError()->setErrorObject(m_target.data());
        }

        if (hasError()) {
            if (!delayedError()->addError(ep)) ep->warning(this->error(engine()));
        } else {
            clearError();
        }
    }

    ep->dereferenceScarceResources();
}

class QObjectPointerBinding: public QQmlBinding
{
    QQmlMetaObject targetMetaObject;

public:
    QObjectPointerBinding(QMetaType propertyType)
        : targetMetaObject(QQmlPropertyPrivate::rawMetaObjectForType(propertyType))
    {}

protected:
    Q_ALWAYS_INLINE bool write(void *result, QMetaType type, bool isUndefined,
                               QQmlPropertyData::WriteFlags flags) override final
    {
        const QQmlPropertyData *pd;
        QQmlPropertyData vtpd;
        getPropertyData(&pd, &vtpd);
        if (Q_UNLIKELY(isUndefined || vtpd.isValid()))
            return slowWrite(*pd, vtpd, result, type, isUndefined, flags);

        // Check if the result is a QObject:
        QObject *resultObject = nullptr;
        QQmlMetaObject resultMo;
        const auto typeFlags = type.flags();
        if (!result || ((typeFlags & QMetaType::IsPointer) && !*static_cast<void **>(result))) {
            // Special case: we can always write a nullptr. Don't bother checking anything else.
            return pd->writeProperty(targetObject(), &resultObject, flags);
        } else if (typeFlags & QMetaType::PointerToQObject) {
            resultObject = *static_cast<QObject **>(result);
            if (!resultObject)
                return pd->writeProperty(targetObject(), &resultObject, flags);
            if (QQmlData *ddata = QQmlData::get(resultObject, false))
                resultMo = ddata->propertyCache;
            if (resultMo.isNull())
                resultMo = resultObject->metaObject();
        } else if (type == QMetaType::fromType<QVariant>()) {
            const QVariant value = *static_cast<QVariant *>(result);
            resultMo = QQmlPropertyPrivate::rawMetaObjectForType(value.metaType());
            if (resultMo.isNull())
                return slowWrite(*pd, vtpd, result, type, isUndefined, flags);
            resultObject = *static_cast<QObject *const *>(value.constData());
        } else {
            return slowWrite(*pd, vtpd, result, type, isUndefined, flags);
        }

        return compareAndSet(resultMo, resultObject, pd, flags, [&]() {
            return slowWrite(*pd, vtpd, result, type, isUndefined, flags);
        });
    }

    Q_ALWAYS_INLINE bool write(const QV4::Value &result, bool isUndefined,
                               QQmlPropertyData::WriteFlags flags) override final
    {
        const QQmlPropertyData *pd;
        QQmlPropertyData vtpd;
        getPropertyData(&pd, &vtpd);
        if (Q_UNLIKELY(isUndefined || vtpd.isValid()))
            return slowWrite(*pd, vtpd, result, isUndefined, flags);

        // Check if the result is a QObject:
        QObject *resultObject = nullptr;
        QQmlMetaObject resultMo;
        if (result.isNull()) {
            // Special case: we can always write a nullptr. Don't bother checking anything else.
            return pd->writeProperty(targetObject(), &resultObject, flags);
        } else if (auto wrapper = result.as<QV4::QObjectWrapper>()) {
            resultObject = wrapper->object();
            if (!resultObject)
                return pd->writeProperty(targetObject(), &resultObject, flags);
            if (QQmlData *ddata = QQmlData::get(resultObject, false))
                resultMo = ddata->propertyCache;
            if (resultMo.isNull()) {
                resultMo = resultObject->metaObject();
            }
        } else if (auto variant = result.as<QV4::VariantObject>()) {
            const QVariant value = variant->d()->data();
            resultMo = QQmlPropertyPrivate::rawMetaObjectForType(value.metaType());
            if (resultMo.isNull())
                return slowWrite(*pd, vtpd, result, isUndefined, flags);
            resultObject = *static_cast<QObject *const *>(value.constData());
        } else {
            return slowWrite(*pd, vtpd, result, isUndefined, flags);
        }

        return compareAndSet(resultMo, resultObject, pd, flags, [&]() {
            return slowWrite(*pd, vtpd, result, isUndefined, flags);
        });
    }

private:
    using QQmlBinding::slowWrite;

    template<typename SlowWrite>
    bool compareAndSet(const QQmlMetaObject &resultMo, QObject *resultObject, const QQmlPropertyData *pd,
                       QQmlPropertyData::WriteFlags flags, const SlowWrite &slowWrite) const
    {
        if (QQmlMetaObject::canConvert(resultMo, targetMetaObject)) {
            return pd->writeProperty(targetObject(), &resultObject, flags);
        } else if (!resultObject && QQmlMetaObject::canConvert(targetMetaObject, resultMo)) {
            // In the case of a null QObject, we assign the null if there is
            // any change that the null variant type could be up or down cast to
            // the property type.
            return pd->writeProperty(targetObject(), &resultObject, flags);
        } else {
            return slowWrite();
        }
    }
};

QQmlBinding *QQmlBinding::newBinding(const QQmlPropertyData *property)
{
    return newBinding(property ? property->propType() : QMetaType());
}

QQmlBinding *QQmlBinding::newBinding(QMetaType propertyType)
{
    if (propertyType.flags() & QMetaType::PointerToQObject)
        return new QObjectPointerBinding(propertyType);

    switch (propertyType.id()) {
    case QMetaType::Bool:
        return new GenericBinding<QMetaType::Bool>;
    case QMetaType::Int:
        return new GenericBinding<QMetaType::Int>;
    case QMetaType::Double:
        return new GenericBinding<QMetaType::Double>;
    case QMetaType::Float:
        return new GenericBinding<QMetaType::Float>;
    case QMetaType::QString:
        return new GenericBinding<QMetaType::QString>;
    default:
        return new GenericBinding<QMetaType::UnknownType>;
    }
}

QT_END_NAMESPACE
