// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpropertybinding_p.h"

#include <private/qqmlbinding_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlscriptstring_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4qmlcontext_p.h>

#include <QtQml/qqmlinfo.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQQPropertyBinding, "qt.qml.propertybinding");

QUntypedPropertyBinding QQmlPropertyBinding::create(const QQmlPropertyData *pd, QV4::Function *function,
                                                    QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt,
                                                    QV4::ExecutionContext *scope, QObject *target, QQmlPropertyIndex targetIndex)
{
    Q_ASSERT(pd);
    return create(pd->propType(), function, obj, ctxt, scope, target, targetIndex);
}

QUntypedPropertyBinding QQmlPropertyBinding::create(QMetaType propertyType, QV4::Function *function,
                                                    QObject *obj,
                                                    const QQmlRefPointer<QQmlContextData> &ctxt,
                                                    QV4::ExecutionContext *scope, QObject *target,
                                                    QQmlPropertyIndex targetIndex)
{
    auto buffer = new std::byte[QQmlPropertyBinding::getSizeEnsuringAlignment()
            + sizeof(QQmlPropertyBindingJS)+jsExpressionOffsetLength()]; // QQmlPropertyBinding uses delete[]
    auto binding = new (buffer) QQmlPropertyBinding(propertyType, target, targetIndex,
                                                    TargetData::WithoutBoundFunction);
    auto js = new(buffer + QQmlPropertyBinding::getSizeEnsuringAlignment() + jsExpressionOffsetLength()) QQmlPropertyBindingJS();
    Q_ASSERT(binding->jsExpression() == js);
    Q_ASSERT(js->asBinding() == binding);
    Q_UNUSED(js);
    binding->jsExpression()->setNotifyOnValueChanged(true);
    binding->jsExpression()->setContext(ctxt);
    binding->jsExpression()->setScopeObject(obj);
    binding->jsExpression()->setupFunction(scope, function);
    return QUntypedPropertyBinding(static_cast<QPropertyBindingPrivate *>(QPropertyBindingPrivatePtr(binding).data()));
}

QUntypedPropertyBinding QQmlPropertyBinding::createFromCodeString(const QQmlPropertyData *pd, const QString& str, QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt, const QString &url, quint16 lineNumber, QObject *target, QQmlPropertyIndex targetIndex)
{
    auto buffer = new std::byte[QQmlPropertyBinding::getSizeEnsuringAlignment()
            + sizeof(QQmlPropertyBindingJS)+jsExpressionOffsetLength()]; // QQmlPropertyBinding uses delete[]
    auto binding = new(buffer) QQmlPropertyBinding(QMetaType(pd->propType()), target, targetIndex, TargetData::WithoutBoundFunction);
    auto js = new(buffer + QQmlPropertyBinding::getSizeEnsuringAlignment() + jsExpressionOffsetLength()) QQmlPropertyBindingJS();
    Q_ASSERT(binding->jsExpression() == js);
    Q_ASSERT(js->asBinding() == binding);
    Q_UNUSED(js);
    binding->jsExpression()->setNotifyOnValueChanged(true);
    binding->jsExpression()->setContext(ctxt);
    binding->jsExpression()->createQmlBinding(ctxt, obj, str, url, lineNumber);
    return QUntypedPropertyBinding(static_cast<QPropertyBindingPrivate *>(QPropertyBindingPrivatePtr(binding).data()));
}

QUntypedPropertyBinding QQmlPropertyBinding::createFromScriptString(const QQmlPropertyData *property, const QQmlScriptString &script, QObject *obj, QQmlContext *ctxt, QObject *target, QQmlPropertyIndex targetIndex)
{
    const QQmlScriptStringPrivate *scriptPrivate = script.d.data();
    // without a valid context, we cannot create anything
    if (!ctxt && (!scriptPrivate->context || !scriptPrivate->context->isValid())) {
        return {};
    }

    auto scopeObject = obj ? obj : scriptPrivate->scope;

    QV4::Function *runtimeFunction = nullptr;
    QString url;
    QQmlRefPointer<QQmlContextData> ctxtdata = QQmlContextData::get(scriptPrivate->context);
    QQmlEnginePrivate *engine = QQmlEnginePrivate::get(scriptPrivate->context->engine());
    if (engine && ctxtdata && !ctxtdata->urlString().isEmpty() && ctxtdata->typeCompilationUnit()) {
        url = ctxtdata->urlString();
        if (scriptPrivate->bindingId != QQmlBinding::Invalid)
            runtimeFunction = ctxtdata->typeCompilationUnit()->runtimeFunctions.at(scriptPrivate->bindingId);
    }
    // Do we actually have a function in the script string? If not, this becomes createCodeFromString
    if (!runtimeFunction)
        return createFromCodeString(property, scriptPrivate->script, obj, ctxtdata, url, scriptPrivate->lineNumber, target, targetIndex);

    auto buffer = new std::byte[QQmlPropertyBinding::getSizeEnsuringAlignment()
            + sizeof(QQmlPropertyBindingJS)+jsExpressionOffsetLength()]; // QQmlPropertyBinding uses delete[]
    auto binding = new(buffer) QQmlPropertyBinding(QMetaType(property->propType()), target, targetIndex, TargetData::WithoutBoundFunction);
    auto js = new(buffer + QQmlPropertyBinding::getSizeEnsuringAlignment() + jsExpressionOffsetLength()) QQmlPropertyBindingJS();
    Q_ASSERT(binding->jsExpression() == js);
    Q_ASSERT(js->asBinding() == binding);
    js->setContext(QQmlContextData::get(ctxt ? ctxt : scriptPrivate->context));

    QV4::ExecutionEngine *v4 = engine->v4engine();
    QV4::Scope scope(v4);
    QV4::Scoped<QV4::QmlContext> qmlContext(scope, QV4::QmlContext::create(v4->rootContext(), ctxtdata, scopeObject));
    js->setupFunction(qmlContext, runtimeFunction);
    return QUntypedPropertyBinding(static_cast<QPropertyBindingPrivate *>(QPropertyBindingPrivatePtr(binding).data()));
}

QUntypedPropertyBinding QQmlPropertyBinding::createFromBoundFunction(const QQmlPropertyData *pd, QV4::BoundFunction *function, QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt, QV4::ExecutionContext *scope, QObject *target, QQmlPropertyIndex targetIndex)
{
    auto buffer = new std::byte[QQmlPropertyBinding::getSizeEnsuringAlignment()
            + sizeof(QQmlPropertyBindingJSForBoundFunction)+jsExpressionOffsetLength()]; // QQmlPropertyBinding uses delete[]
    auto binding = new(buffer) QQmlPropertyBinding(QMetaType(pd->propType()), target, targetIndex, TargetData::HasBoundFunction);
    auto js = new(buffer + QQmlPropertyBinding::getSizeEnsuringAlignment() + jsExpressionOffsetLength()) QQmlPropertyBindingJSForBoundFunction();
    Q_ASSERT(binding->jsExpression() == js);
    Q_ASSERT(js->asBinding() == binding);
    Q_UNUSED(js);
    binding->jsExpression()->setNotifyOnValueChanged(true);
    binding->jsExpression()->setContext(ctxt);
    binding->jsExpression()->setScopeObject(obj);
    binding->jsExpression()->setupFunction(scope, function->function());
    js->m_boundFunction.set(function->engine(), *function);
    return QUntypedPropertyBinding(static_cast<QPropertyBindingPrivate *>(QPropertyBindingPrivatePtr(binding).data()));
}

/*!
    \fn bool QQmlPropertyBindingJS::hasDependencies()
    \internal

    Returns true if this binding has dependencies.
    Dependencies can be either QProperty dependencies or dependencies of
    the JS expression (aka activeGuards). Translations end up as a QProperty
    dependency, so they do not need any special handling
    Note that a QQmlPropertyBinding never stores qpropertyChangeTriggers.
 */


void QQmlPropertyBindingJS::expressionChanged()
{
    auto binding = asBinding();
    if (!binding->propertyDataPtr)
        return;
    const auto currentTag = m_error.tag();
    if (currentTag == InEvaluationLoop) {
        QQmlError err;
        auto location = QQmlJavaScriptExpression::sourceLocation();
        err.setUrl(QUrl{location.sourceFile});
        err.setLine(location.line);
        err.setColumn(location.column);
        const auto ctxt = context();
        QQmlEngine *engine = ctxt ? ctxt->engine() : nullptr;
        if (engine)
            err.setDescription(asBinding()->createBindingLoopErrorDescription());
        else
            err.setDescription(QString::fromLatin1("Binding loop detected"));
        err.setObject(asBinding()->target());
        qmlWarning(this->scopeObject(), err);
        return;
    }
    m_error.setTag(InEvaluationLoop);
    PendingBindingObserverList bindingObservers;
    binding->evaluateRecursive(bindingObservers);
    binding->notifyNonRecursive(bindingObservers);
    m_error.setTag(NoTag);
}

QQmlPropertyBinding::QQmlPropertyBinding(QMetaType mt, QObject *target, QQmlPropertyIndex targetIndex, TargetData::BoundFunction hasBoundFunction)
    : QPropertyBindingPrivate(mt,
                              bindingFunctionVTableForQQmlPropertyBinding(mt),
                              QPropertyBindingSourceLocation(), true)
{
    static_assert (std::is_trivially_destructible_v<TargetData>);
    static_assert (sizeof(TargetData) + sizeof(DeclarativeErrorCallback) <= sizeof(QPropertyBindingSourceLocation));
    static_assert (alignof(TargetData) <= alignof(QPropertyBindingSourceLocation));
    const auto state = hasBoundFunction ? TargetData::HasBoundFunction : TargetData::WithoutBoundFunction;
    new (&declarativeExtraData) TargetData {target, targetIndex, state};
    errorCallBack = bindingErrorCallback;
}

static QtPrivate::QPropertyBindingData *bindingDataFromPropertyData(QUntypedPropertyData *dataPtr, QMetaType type)
{
    // XXX Qt 7: We need a clean way to access the binding data
    /* This function makes the (dangerous) assumption that if we could not get the binding data
       from the binding storage, we must have been handed a QProperty.
       This does hold for anything a user could write, as there the only ways of providing a bindable property
       are to use the Q_X_BINDABLE macros, or to directly expose a QProperty.
       As long as we can ensure that any "fancier" property we implement is not resettable, we should be fine.
       We procede to calculate the address of the binding data pointer from the address of the data pointer
    */
    Q_ASSERT(dataPtr);
    std::byte *qpropertyPointer = reinterpret_cast<std::byte *>(dataPtr);
    qpropertyPointer += type.sizeOf();
    constexpr auto alignment = alignof(QtPrivate::QPropertyBindingData *);
    auto aligned = (quintptr(qpropertyPointer) + alignment - 1) & ~(alignment - 1); // ensure pointer alignment
    return reinterpret_cast<QtPrivate::QPropertyBindingData *>(aligned);
}

void QQmlPropertyBinding::handleUndefinedAssignment(QQmlEnginePrivate *ep, void *dataPtr)
{
    const QQmlPropertyData *propertyData = nullptr;
    QQmlPropertyData valueTypeData;
    QQmlData *data = QQmlData::get(target(), false);
    Q_ASSERT(data);
    if (Q_UNLIKELY(!data->propertyCache))
        data->propertyCache = QQmlMetaType::propertyCache(target()->metaObject());

    propertyData = data->propertyCache->property(targetIndex().coreIndex());
    Q_ASSERT(propertyData);
    Q_ASSERT(!targetIndex().hasValueTypeIndex());
    QQmlProperty prop = QQmlPropertyPrivate::restore(target(), *propertyData, &valueTypeData, nullptr);
    // helper function for writing back value into dataPtr
    // this is necessary  for QObjectCompatProperty, which doesn't give us the "real" dataPtr
    // if we don't write the correct value, we would otherwise set the default constructed value
    auto writeBackCurrentValue = [&](QVariant &&currentValue) {
        if (currentValue.metaType() != valueMetaType())
            currentValue.convert(valueMetaType());
        auto metaType = valueMetaType();
        metaType.destruct(dataPtr);
        metaType.construct(dataPtr, currentValue.constData());
    };
    if (prop.isResettable()) {
        // Normally a reset would remove any existing binding; but now we need to keep the binding alive
        // to handle the case where this binding becomes defined again
        // We therefore detach the binding, call reset, and reattach again
        const auto storage = qGetBindingStorage(target());
        auto bindingData = storage->bindingData(propertyDataPtr);
        if (!bindingData)
            bindingData = bindingDataFromPropertyData(propertyDataPtr, propertyData->propType());
        QPropertyBindingDataPointer bindingDataPointer{bindingData};
        auto firstObserver = takeObservers();
        bindingData->d_ref() = 0;
        if (firstObserver) {
            bindingDataPointer.setObservers(firstObserver.ptr);
        }
        Q_ASSERT(!bindingData->hasBinding());
        setIsUndefined(true);
        //suspend binding evaluation state for reset and subsequent read
        auto state = QtPrivate::suspendCurrentBindingStatus();
        prop.reset(); // May re-allocate the bindingData
        QVariant currentValue = QVariant(prop.propertyMetaType(), propertyDataPtr);
        QtPrivate::restoreBindingStatus(state);
        writeBackCurrentValue(std::move(currentValue));

        // Re-fetch binding data
        bindingData = storage->bindingData(propertyDataPtr);
        if (!bindingData)
            bindingData = bindingDataFromPropertyData(propertyDataPtr, propertyData->propType());
        bindingDataPointer = QPropertyBindingDataPointer {bindingData};

        // reattach the binding (without causing a new notification)
        if (Q_UNLIKELY(bindingData->d() & QtPrivate::QPropertyBindingData::BindingBit)) {
            qCWarning(lcQQPropertyBinding) << "Resetting " << prop.name() << "due to the binding becoming undefined  caused a new binding to be installed\n"
                       << "The old binding binding will be abandoned";
            deref();
            return;
        }
        // reset might have changed observers (?), so refresh firstObserver
        firstObserver = bindingDataPointer.firstObserver();
        bindingData->d_ref() = reinterpret_cast<quintptr>(this) | QtPrivate::QPropertyBindingData::BindingBit;
        if (firstObserver)
            prependObserver(firstObserver);
    } else {
        QQmlError qmlError;
        auto location = jsExpression()->sourceLocation();
        qmlError.setColumn(location.column);
        qmlError.setLine(location.line);
        qmlError.setUrl(QUrl {location.sourceFile});
        const QString description = QStringLiteral(R"(QML %1: Unable to assign [undefined] to "%2")").arg(QQmlMetaType::prettyTypeName(target()) , prop.name());
        qmlError.setDescription(description);
        qmlError.setObject(target());
        ep->warning(qmlError);
    }
}

QString QQmlPropertyBinding::createBindingLoopErrorDescription()
{
    const QQmlPropertyData *propertyData = nullptr;
    QQmlPropertyData valueTypeData;
    QQmlData *data = QQmlData::get(target(), false);
    Q_ASSERT(data);
    if (Q_UNLIKELY(!data->propertyCache))
        data->propertyCache = QQmlMetaType::propertyCache(target()->metaObject());

    propertyData = data->propertyCache->property(targetIndex().coreIndex());
    Q_ASSERT(propertyData);
    Q_ASSERT(!targetIndex().hasValueTypeIndex());
    QQmlProperty prop = QQmlPropertyPrivate::restore(target(), *propertyData, &valueTypeData, nullptr);
    return QStringLiteral(R"(QML %1: Binding loop detected for property "%2")").arg(QQmlMetaType::prettyTypeName(target()) , prop.name());
}

void QQmlPropertyBinding::bindingErrorCallback(QPropertyBindingPrivate *that)
{
    auto This = static_cast<QQmlPropertyBinding *>(that);
    auto target = This->target();
    auto engine = qmlEngine(target);
    if (!engine)
        return;

    auto error = This->bindingError();
    QQmlError qmlError;
    auto location = This->jsExpression()->sourceLocation();
    qmlError.setColumn(location.column);
    qmlError.setLine(location.line);
    qmlError.setUrl(QUrl {location.sourceFile});
    auto description = error.description();
    if (error.type() == QPropertyBindingError::BindingLoop) {
        description = This->createBindingLoopErrorDescription();
    }
    qmlError.setDescription(description);
    qmlError.setObject(target);
    QQmlEnginePrivate::get(engine)->warning(qmlError);
}

template<typename TranslateWithUnit>
auto qQmlTranslationPropertyBindingCreateBinding(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        TranslateWithUnit &&translateWithUnit)
{
    return [compilationUnit, translateWithUnit](QMetaType metaType, void *dataPtr) -> bool {
        // Create a dependency to the translationLanguage
        QQmlEnginePrivate::get(compilationUnit->engine)->translationLanguage.value();

        QVariant resultVariant(translateWithUnit(compilationUnit));
        if (metaType != QMetaType::fromType<QString>())
            resultVariant.convert(metaType);

        const bool hasChanged = !metaType.equals(resultVariant.constData(), dataPtr);
        metaType.destruct(dataPtr);
        metaType.construct(dataPtr, resultVariant.constData());
        return hasChanged;
    };
}

QUntypedPropertyBinding QQmlTranslationPropertyBinding::create(
        const QQmlPropertyData *pd,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        const QV4::CompiledData::Binding *binding)
{
    auto translationBinding = qQmlTranslationPropertyBindingCreateBinding(
            compilationUnit,
            [binding](const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit) {
                return compilationUnit->bindingValueAsString(binding);
            });

    return QUntypedPropertyBinding(QMetaType(pd->propType()), translationBinding,
                                   QPropertyBindingSourceLocation());
}

QUntypedPropertyBinding QQmlTranslationPropertyBinding::create(
        const QMetaType &propertyType,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        const QQmlTranslation &translationData)
{
    auto translationBinding = qQmlTranslationPropertyBindingCreateBinding(
            compilationUnit,
            [translationData](
                    const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit) {
                Q_UNUSED(compilationUnit);
                return translationData.translate();
            });

    return QUntypedPropertyBinding(propertyType, translationBinding,
                                   QPropertyBindingSourceLocation());
}

QV4::ReturnedValue QQmlPropertyBindingJSForBoundFunction::evaluate(bool *isUndefined)
{
    QV4::ExecutionEngine *v4 = engine()->handle();
    int argc = 0;
    const QV4::Value *argv = nullptr;
    const QV4::Value *thisObject = nullptr;
    QV4::BoundFunction *b = nullptr;
    if ((b = m_boundFunction.as<QV4::BoundFunction>())) {
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

QT_END_NAMESPACE
