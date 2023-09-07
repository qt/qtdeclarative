// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlbind_p.h"

#include <private/qqmlanybinding_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlnullablevalue_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4persistent_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qv4resolvedtypereference_p.h>

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqmlpropertymap.h>

#include <QtCore/private/qobject_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcBindingRemoval)

enum class QQmlBindEntryKind: quint8 {
    V4Value,
    Variant,
    Binding,
    None
};

/*!
 * \internal
 * QQmlBindEntryContent can store one of QV4::Value, QVariant, QQmlAnyBinding, or nothing,
 * as denoted by QQmlBindEntryKind. It expects the calling code to know what is stored at
 * any time. On each method invocation, the current kind has to be passed as last parameter
 * and the new kind is returned.
 */
union QQmlBindEntryContent {
    Q_DISABLE_COPY_MOVE(QQmlBindEntryContent)
public:
    QQmlBindEntryContent() {}
    ~QQmlBindEntryContent() {}

    [[nodiscard]] QQmlBindEntryKind set(
            QQmlBindEntryContent &&other, QQmlBindEntryKind newKind, QQmlBindEntryKind oldKind)
    {
        silentDestroy(oldKind);
        switch (newKind) {
        case QQmlBindEntryKind::V4Value:
            new (&v4Value) QV4::PersistentValue(std::move(other.v4Value));
            break;
        case QQmlBindEntryKind::Variant:
            new (&variant) QVariant(std::move(other.variant));
            break;
        case QQmlBindEntryKind::Binding:
            new (&binding) QQmlAnyBinding(std::move(other.binding));
            break;
        case QQmlBindEntryKind::None:
            break;
        }
        return newKind;
    }

    [[nodiscard]] QQmlBindEntryKind set(
            const QQmlBindEntryContent &other, QQmlBindEntryKind newKind, QQmlBindEntryKind oldKind)
    {
        silentDestroy(oldKind);
        switch (newKind) {
        case QQmlBindEntryKind::V4Value:
            new (&v4Value) QV4::PersistentValue(other.v4Value);
            break;
        case QQmlBindEntryKind::Variant:
            new (&variant) QVariant(other.variant);
            break;
        case QQmlBindEntryKind::Binding:
            new (&binding) QQmlAnyBinding(other.binding);
            break;
        case QQmlBindEntryKind::None:
            break;
        }
        return newKind;
    }

    [[nodiscard]] QQmlBindEntryKind destroy(QQmlBindEntryKind kind)
    {
        switch (kind) {
        case QQmlBindEntryKind::V4Value:
            v4Value.~PersistentValue();
            break;
        case QQmlBindEntryKind::Variant:
            variant.~QVariant();
            break;
        case QQmlBindEntryKind::Binding:
            binding.~QQmlAnyBinding();
            break;
        case QQmlBindEntryKind::None:
            break;
        }
        return QQmlBindEntryKind::None;
    }

    [[nodiscard]] QQmlBindEntryKind set(QVariant v, QQmlBindEntryKind oldKind)
    {
        silentDestroy(oldKind);
        new (&variant) QVariant(std::move(v));
        return QQmlBindEntryKind::Variant;
    }

    [[nodiscard]] QQmlBindEntryKind set(QV4::PersistentValue v, QQmlBindEntryKind oldKind)
    {
        silentDestroy(oldKind);
        new (&v4Value) QV4::PersistentValue(std::move(v));
        return QQmlBindEntryKind::V4Value;
    }

    [[nodiscard]] QQmlBindEntryKind set(QQmlAnyBinding v, QQmlBindEntryKind oldKind)
    {
        silentDestroy(oldKind);
        new (&binding) QQmlAnyBinding(std::move(v));
        return QQmlBindEntryKind::Binding;
    }

    QV4::PersistentValue v4Value;
    QVariant variant;
    QQmlAnyBinding binding;

private:
    void silentDestroy(QQmlBindEntryKind oldKind)
    {
        const QQmlBindEntryKind dead = destroy(oldKind);
        Q_ASSERT(dead == QQmlBindEntryKind::None);
        Q_UNUSED(dead);
    }
};

/*!
 * \internal
 * QQmlBindEntry holds two QQmlBindEntryContent members, along with their kinds.
 * The \l current content is the value or binding the Binding element installs on
 * the target if enabled (that is, if \l{when}). The \l previous content is what
 * the target holds before the Binding element installs its binding or value. It
 * is restored if !\l{when}. The \l prop member holds the target property.
 */
struct QQmlBindEntry
{
    QQmlBindEntry() = default;
    QQmlBindEntry(QQmlBindEntry &&other) noexcept : prop(std::move(other.prop))
    {
        currentKind = current.set(std::move(other.current), other.currentKind, currentKind);
        previousKind = previous.set(std::move(other.previous), other.previousKind, previousKind);
    }

    QQmlBindEntry(const QQmlBindEntry &other)
        : prop(other.prop)
    {
        currentKind = current.set(other.current, other.currentKind, currentKind);
        previousKind = previous.set(other.previous, other.previousKind, previousKind);
    }

    ~QQmlBindEntry()
    {
        currentKind = current.destroy(currentKind);
        previousKind = previous.destroy(previousKind);
    }

    QQmlBindEntry &operator=(QQmlBindEntry &&other) noexcept
    {
        if (this == &other)
            return *this;
        prop = std::move(other.prop);
        currentKind = current.set(std::move(other.current), other.currentKind, currentKind);
        previousKind = previous.set(std::move(other.previous), other.previousKind, previousKind);
        return *this;
    }

    QQmlBindEntry &operator=(const QQmlBindEntry &other)
    {
        if (this == &other)
            return *this;
        prop = other.prop;
        currentKind = current.set(other.current, other.currentKind, currentKind);
        previousKind = previous.set(other.previous, other.previousKind, previousKind);
        return *this;
    }


    QQmlBindEntryContent current;
    QQmlBindEntryContent previous;
    QQmlProperty prop;
    QQmlBindEntryKind currentKind = QQmlBindEntryKind::None;
    QQmlBindEntryKind previousKind = QQmlBindEntryKind::None;

    void validate(QQmlBind *q) const;
    void clearPrev();
    void setTarget(QQmlBind *q, const QQmlProperty &p);
};

class QQmlBindPrivate : public QObjectPrivate
{
public:
    QQmlBindPrivate()
        : when(true)
        , componentComplete(true)
        , delayed(false)
        , pendingEval(false)
        , restoreBinding(true)
        , restoreValue(true)
        , writingProperty(false)
        , lastIsTarget(false)
    {
    }
    ~QQmlBindPrivate() { }

    // There can be multiple entries when using the generalized grouped
    // property syntax. One is used for target/property/value.
    QVarLengthArray<QQmlBindEntry, 1> entries;

    // The target object if using the \l target property
    QPointer<QObject> obj;

    // Any values we need to create a proxy for. This is necessary when
    // using the \l delayed member on generalized grouped properties. See
    // the note on \l delayed.
    std::unique_ptr<QQmlPropertyMap> delayedValues;

    // The property name if using the \l property property.
    QString propName;

    // Whether the binding is enabled.
    bool when: 1;

    // Whether we have already parsed any generalized grouped properties
    // we might need.
    bool componentComplete:1;

    // Whether we should run in "delayed" mode and proxy all values before
    // applying them to the target.
    bool delayed:1;

    // In delayed mode, when using the target/property mode, the \l value
    // is the proxy. Then pendingEval denotes that a timer is active to
    // apply the value. We should not start another timer then.
    bool pendingEval:1;

    // Whether we should restore bindings on !when.
    // TODO: Deprecate this and always do.
    bool restoreBinding:1;

    // Whether we should restore values on !when.
    // TODO: Deprecate this and always do.
    bool restoreValue:1;

    // writingProperty tracks whether we are updating the target property
    // when using target/property/value. We use this information to warn about
    // binding removal if we detect the target property to be updated while we
    // are not writing it. This doesn't remove the Binding after all.
    // For generalized grouped properties, we don't have to do this as writing
    // the target property does remove the binding, just like it removes any
    // other binding.
    bool writingProperty:1;

    // Whether the last entry is the the target property referred to by the
    // \l target object and the \l property property. This will generally be
    // the case when using \l target and \l property.
    bool lastIsTarget:1;

    QQmlBindEntry *targetEntry();
    void validate(QQmlBind *binding) const;
    void decodeBinding(
            QQmlBind *q, const QString &propertyPrefix, QQmlData::DeferredData *deferredData,
            const QV4::CompiledData::Binding *binding,
            QQmlComponentPrivate::ConstructionState *immediateState);
    void createDelayedValues();
    void onDelayedValueChanged(QString delayedName);
    void evalDelayed();
    void buildBindEntries(QQmlBind *q, QQmlComponentPrivate::DeferredState *deferredState);
};

void QQmlBindEntry::validate(QQmlBind *q) const
{
    if (!prop.isWritable()) {
        qmlWarning(q) << "Property '" << prop.name() << "' on "
                      << QQmlMetaType::prettyTypeName(prop.object()) << " is read-only.";
    }
}

QQmlBindEntry *QQmlBindPrivate::targetEntry()
{
    if (!lastIsTarget) {
        entries.append(QQmlBindEntry());
        lastIsTarget = true;
    }
    return &entries.last();
}

void QQmlBindPrivate::validate(QQmlBind *q) const
{
    if (!when)
        return;

    qsizetype iterationEnd = entries.size();
    if (lastIsTarget) {
        if (obj) {
            Q_ASSERT(!entries.isEmpty());
            const QQmlBindEntry &last = entries.last();
            if (!last.prop.isValid()) {
                qmlWarning(q) << "Property '" << propName << "' does not exist on "
                              << QQmlMetaType::prettyTypeName(obj) << ".";
                --iterationEnd;
            }
        } else {
            --iterationEnd;
        }
    }

    for (qsizetype i = 0; i < iterationEnd; ++i)
        entries[i].validate(q);
}

/*!
    \qmltype Binding
    \instantiates QQmlBind
    \inqmlmodule QtQml
    \ingroup qtquick-interceptors
    \brief Enables the arbitrary creation of property bindings.

    In QML, property bindings result in a dependency between the properties of
    different objects.

    \section1 Binding to an Inaccessible Property

    Sometimes it is necessary to bind an object's property to
    that of another object that isn't directly instantiated by QML, such as a
    property of a class exported to QML by C++. You can use the Binding type
    to establish this dependency; binding any value to any object's property.

    For example, in a C++ application that maps an "app.enteredText" property
    into QML, you can use Binding to update the enteredText property.

    \qml
    TextEdit { id: myTextField; text: "Please type here..." }
    Binding { app.enteredText: myTextField.text }
    \endqml

    When \c{text} changes, the C++ property \c{enteredText} will update
    automatically.

    \section1 Conditional Bindings

    In some cases you may want to modify the value of a property when a certain
    condition is met but leave it unmodified otherwise. Often, it's not possible
    to do this with direct bindings, as you have to supply values for all
    possible branches.

    For example, the code snippet below results in a warning whenever you
    release the mouse. This is because the value of the binding is undefined
    when the mouse isn't pressed.

    \qml
    // produces warning: "Unable to assign [undefined] to double value"
    value: if (mouse.pressed) mouse.mouseX
    \endqml

    The Binding type can prevent this warning.

    \qml
    Binding on value {
        when: mouse.pressed
        value: mouse.mouseX
    }
    \endqml

    The Binding type restores any previously set direct bindings on the
    property.

    \sa {Qt QML}
*/
QQmlBind::QQmlBind(QObject *parent)
    : QObject(*(new QQmlBindPrivate), parent)
{
}

/*!
    \qmlproperty bool QtQml::Binding::when

    This property holds when the binding is active.
    This should be set to an expression that evaluates to true when you want the binding to be active.

    \qml
    Binding {
        contactName.text: name
        when: list.ListView.isCurrentItem
    }
    \endqml

    By default, any binding or value that was set perviously is restored when the binding becomes
    inactive. You can customize the restoration behavior using the \l restoreMode property.

    \sa restoreMode
*/
bool QQmlBind::when() const
{
    Q_D(const QQmlBind);
    return d->when;
}

void QQmlBind::setWhen(bool v)
{
    Q_D(QQmlBind);
    if (d->when == v)
        return;

    d->when = v;
    if (v && d->componentComplete)
        d->validate(this);
    eval();
}

/*!
    \qmlproperty QtObject QtQml::Binding::target

    The object to be updated. You only need to use this property if you can't
    supply the binding target declaratively. The following two pieces of code
    are equivalent.

    \qml
    Binding { contactName.text: name }
    \endqml

    \qml
    Binding {
        target: contactName
        property: "text"
        value: name
    }
    \endqml

    The former one is much more compact, but you cannot replace the target
    object or property at run time. With the latter one you can.
*/
QObject *QQmlBind::object()
{
    Q_D(const QQmlBind);
    return d->obj;
}

void QQmlBind::setObject(QObject *obj)
{
    Q_D(QQmlBind);
    if (d->obj && d->when) {
        /* if we switch the object at runtime, we need to restore the
           previous binding on the old object before continuing */
        d->when = false;
        eval();
        d->when = true;
    }
    /* if "when" and "target" depend on the same property, we might
       end up here before we could have updated "when". So reevaluate
       when manually here.
     */
    const QQmlProperty whenProp(this, QLatin1StringView("when"));
    const auto potentialWhenBinding = QQmlAnyBinding::ofProperty(whenProp);
    if (auto abstractBinding = potentialWhenBinding.asAbstractBinding()) {
        QQmlBinding *binding = static_cast<QQmlBinding *>(abstractBinding);
        if (binding->hasValidContext()) {
            const auto boolType = QMetaType::fromType<bool>();
            bool when;
            binding->evaluate(&when, boolType);
            d->when = when;
        }
    }
    d->obj = obj;
    if (d->componentComplete) {
        setTarget(QQmlProperty(d->obj, d->propName, qmlContext(this)));
        if (d->when)
            d->validate(this);
    }
    eval();
}

/*!
    \qmlproperty string QtQml::Binding::property

    The property to be updated.

    This can be a group property if the expression results in accessing a
    property of a \l {QML Value Types}{value type}. For example:

    \qml
    Item {
        id: item

        property rect rectangle: Qt.rect(0, 0, 200, 200)
    }

    Binding {
        target: item
        property: "rectangle.x"
        value: 100
    }
    \endqml

    You only need to use this property if you can't supply the binding target
    declaratively. The following snippet of code is equivalent to the above
    binding, but more compact:

    \qml
    Binding { item.rectangle.x: 100 }
    \endqml
*/
QString QQmlBind::property() const
{
    Q_D(const QQmlBind);
    return d->propName;
}

void QQmlBind::setProperty(const QString &p)
{
    Q_D(QQmlBind);
    if (!d->propName.isEmpty() && d->when) {
        /* if we switch the property name at runtime, we need to restore the
           previous binding on the old object before continuing */
        d->when = false;
        eval();
        d->when = true;
    }
    d->propName = p;
    if (d->componentComplete) {
        setTarget(QQmlProperty(d->obj, d->propName, qmlContext(this)));
        if (d->when)
            d->validate(this);
    }
    eval();
}

/*!
    \qmlproperty var QtQml::Binding::value

    The value to be set on the target object and property.  This can be a
    constant (which isn't very useful), or a bound expression.

    You only need to use this property if you can't supply the binding target
    declaratively. Otherwise you can directly bind to the target.
*/
QVariant QQmlBind::value() const
{
    Q_D(const QQmlBind);
    if (!d->lastIsTarget)
        return QVariant();
    Q_ASSERT(d->entries.last().currentKind == QQmlBindEntryKind::Variant);
    return d->entries.last().current.variant;
}

void QQmlBind::setValue(const QVariant &v)
{
    Q_D(QQmlBind);
    QQmlBindEntry *targetEntry = d->targetEntry();
    targetEntry->currentKind = targetEntry->current.set(v, targetEntry->currentKind);
    prepareEval();
}

/*!
    \qmlproperty bool QtQml::Binding::delayed
    \since 5.8

    This property holds whether the binding should be delayed.

    A delayed binding will not immediately update the target, but rather wait
    until the event queue has been cleared. This can be used as an optimization,
    or to prevent intermediary values from being assigned.

    \code
    Binding {
        contactName.text.value: givenName + " " + familyName
        when: list.ListView.isCurrentItem
        delayed: true
    }
    \endcode

    \note Using the \l delayed property incurs a run time cost as the Binding
          element has to create a proxy for the value, so that it can delay its
          application to the actual target. When using the \l target and
          \l property properties, this cost is lower because the \l value
          property can be re-used as proxy. When using the form shown above,
          Binding will allocate a separate object with a dynamic meta-object to
          hold the proxy values.
*/
bool QQmlBind::delayed() const
{
    Q_D(const QQmlBind);
    return d->delayed;
}

void QQmlBind::setDelayed(bool delayed)
{
    Q_D(QQmlBind);
    if (d->delayed == delayed)
        return;

    d->delayed = delayed;
    if (!d->componentComplete)
        return;

    d->delayedValues.reset();

    QVarLengthArray<QQmlBindEntry, 1> oldEntries = std::move(d->entries);
    d->entries.clear();
    d->buildBindEntries(this, nullptr);

    if (d->lastIsTarget) {
        d->entries.append(std::move(oldEntries.last()));
        oldEntries.pop_back();
    }

    for (qsizetype i = 0, end = oldEntries.size(); i < end; ++i) {
        QQmlBindEntry &newEntry = d->entries[i];
        QQmlBindEntry &oldEntry = oldEntries[i];
        newEntry.previousKind = newEntry.previous.set(
                    std::move(oldEntry.previous), oldEntry.previousKind, newEntry.previousKind);
        if (d->delayed && oldEntry.currentKind == QQmlBindEntryKind::Binding)
            QQmlAnyBinding::removeBindingFrom(oldEntry.prop);
    }

    if (!d->delayed)
        eval();
}

/*!
    \qmlproperty enumeration QtQml::Binding::restoreMode
    \since 5.14

    This property can be used to describe if and how the original value should
    be restored when the binding is disabled.

    The possible values are:

    \value Binding.RestoreNone      The original value is not restored at all
    \value Binding.RestoreBinding   The original value is restored if it was another binding.
                                    In that case the old binding is in effect again.
    \value Binding.RestoreValue     The original value is restored if it was a plain
                                    value rather than a binding.
    \value Binding.RestoreBindingOrValue The original value is always restored.

    The default value is \c Binding.RestoreBindingOrValue.

    \note This property exists for backwards compatibility with earlier versions
          of Qt. Don't use it in new code.
*/
QQmlBind::RestorationMode QQmlBind::restoreMode() const
{
    Q_D(const QQmlBind);
    unsigned result = RestoreNone;
    if (d->restoreValue)
        result |= RestoreValue;
    if (d->restoreBinding)
        result |= RestoreBinding;
    return RestorationMode(result);
}

void QQmlBind::setRestoreMode(RestorationMode newMode)
{
    Q_D(QQmlBind);
    if (newMode != restoreMode()) {
        d->restoreValue = (newMode & RestoreValue);
        d->restoreBinding = (newMode & RestoreBinding);
        emit restoreModeChanged();
    }
}

void QQmlBind::setTarget(const QQmlProperty &p)
{
    Q_D(QQmlBind);
    d->targetEntry()->setTarget(this, p);
}

void QQmlBindEntry::setTarget(QQmlBind *q, const QQmlProperty &p)
{
    if (Q_UNLIKELY(lcBindingRemoval().isInfoEnabled())) {
        if (QObject *oldObject = prop.object()) {
            QMetaProperty metaProp = oldObject->metaObject()->property(prop.index());
            if (metaProp.hasNotifySignal()) {
                QByteArray signal('2' + metaProp.notifySignal().methodSignature());
                QObject::disconnect(oldObject, signal.constData(),
                                    q, SLOT(targetValueChanged()));
            }
        }
        p.connectNotifySignal(q, SLOT(targetValueChanged()));
    }

    prop = p;
}

void QQmlBind::classBegin()
{
    Q_D(QQmlBind);
    d->componentComplete = false;
}

static QQmlAnyBinding createBinding(
        const QQmlProperty &prop, const QV4::CompiledData::Binding *binding,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        const QQmlRefPointer<QQmlContextData> &contextData,
        QObject *scopeObject)
{
    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_Translation:
    case QV4::CompiledData::Binding::Type_TranslationById:
        return QQmlAnyBinding::createTranslationBinding(prop, compilationUnit, binding, scopeObject);
    case QV4::CompiledData::Binding::Type_Script: {
        const QQmlBinding::Identifier id = binding->value.compiledScriptIndex;
        if (id == QQmlBinding::Invalid) {
            return QQmlAnyBinding::createFromCodeString(
                    prop, compilationUnit->bindingValueAsString(binding), scopeObject,
                    contextData, compilationUnit->finalUrlString(), binding->location.line());
        }
        QV4::Scope scope(contextData->engine()->handle());
        QV4::Scoped<QV4::QmlContext> qmlCtxt(
                scope, QV4::QmlContext::create(
                               scope.engine->rootContext(), contextData, scopeObject));
        return QQmlAnyBinding::createFromFunction(
                prop, compilationUnit->runtimeFunctions.at(id), scopeObject, contextData,
                qmlCtxt);
    }
    default:
        break;
    }
    return QQmlAnyBinding();
}

static void initCreator(
        QQmlData::DeferredData *deferredData,
        const QQmlRefPointer<QQmlContextData> &contextData,
        QQmlComponentPrivate::ConstructionState  *immediateState)
{
    if (!immediateState->hasCreator()) {
        immediateState->setCompletePending(true);
        immediateState->initCreator(
                deferredData->context->parent(), deferredData->compilationUnit,
                contextData);
        immediateState->creator()->beginPopulateDeferred(deferredData->context);
    }
}

void QQmlBindPrivate::decodeBinding(
        QQmlBind *q, const QString &propertyPrefix,
        QQmlData::DeferredData *deferredData,
        const QV4::CompiledData::Binding *binding,
        QQmlComponentPrivate::ConstructionState  *immediateState)
{
    const QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit
            = deferredData->compilationUnit;
    const QString propertySuffix = compilationUnit->stringAt(binding->propertyNameIndex);
    const QString propertyName = propertyPrefix + propertySuffix;

    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_AttachedProperty:
        if (propertyPrefix.isEmpty()) {
            // Top-level attached properties cannot be generalized grouped properties.
            // Treat them as regular properties.
            // ... unless we're not supposed to handle regular properties. Then ignore them.
            if (!immediateState)
                return;

            Q_ASSERT(compilationUnit->stringAt(compilationUnit->objectAt(binding->value.objectIndex)
                                                       ->inheritedTypeNameIndex).isEmpty());

            const QV4::ResolvedTypeReference *typeReference
                    = compilationUnit->resolvedType(binding->propertyNameIndex);
            Q_ASSERT(typeReference);
            QQmlType attachedType = typeReference->type();
            if (!attachedType.isValid()) {
                const QQmlTypeNameCache::Result result
                        = deferredData->context->imports()->query(propertySuffix);
                if (!result.isValid()) {
                    qmlWarning(q).nospace()
                            << "Unknown name " << propertySuffix << ". The binding is ignored.";
                    return;
                }
                attachedType = result.type;
            }

            QQmlContext *context = qmlContext(q);
            QObject *attachedObject = qmlAttachedPropertiesObject(
                    q, attachedType.attachedPropertiesFunction(
                               QQmlEnginePrivate::get(context->engine())));
            if (!attachedObject) {
                qmlWarning(q).nospace() <<"Could not create attached properties object '"
                                        << attachedType.typeName() << "'";
                return;
            }

            initCreator(deferredData, QQmlContextData::get(context), immediateState);
            immediateState->creator()->populateDeferredInstance(
                    q, deferredData->deferredIdx, binding->value.objectIndex, attachedObject,
                    attachedObject, /*value type property*/ nullptr, binding);
            return;
        }
        Q_FALLTHROUGH();
    case QV4::CompiledData::Binding::Type_GroupProperty: {
        const QString pre = propertyName + u'.';
        const QV4::CompiledData::Object *subObj
                = compilationUnit->objectAt(binding->value.objectIndex);
        const QV4::CompiledData::Binding *subBinding = subObj->bindingTable();
        for (quint32 i = 0; i < subObj->nBindings; ++i, ++subBinding)
            decodeBinding(q, pre, deferredData, subBinding, immediateState);
        return;
    }
    default:
        break;
    }

    QQmlBindEntry entry;
    QQmlContext *context = qmlContext(q);
    const QQmlRefPointer<QQmlContextData> contextData = QQmlContextData::get(context);
    entry.prop = QQmlPropertyPrivate::create(nullptr, propertyName, contextData,
                                             QQmlPropertyPrivate::InitFlag::AllowId);
    if (!entry.prop.isValid()) {
        // Try again in the context of this object. If that works, it's a regular property.
        // ... unless we're not supposed to handle regular properties. Then ignore it.
        if (!immediateState)
            return;

        QQmlProperty property = QQmlPropertyPrivate::create(
                    q, propertyName, contextData, QQmlPropertyPrivate::InitFlag::AllowSignal);
        if (property.isValid()) {
            initCreator(deferredData, contextData, immediateState);
            immediateState->creator()->populateDeferredBinding(
                        property, deferredData->deferredIdx, binding);
        } else {
            qmlWarning(q).nospace() << "Unknown name " << propertyName
                                    << ". The binding is ignored.";
        }
        return;
    }

    const auto setVariant = [&entry](QVariant var) {
        entry.currentKind = entry.current.set(std::move(var), entry.currentKind);
    };

    const auto setBinding = [&entry](QQmlAnyBinding binding) {
        entry.currentKind = entry.current.set(binding, entry.currentKind);
    };

    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_AttachedProperty:
    case QV4::CompiledData::Binding::Type_GroupProperty:
        Q_UNREACHABLE(); // Handled above
        break;
    case QV4::CompiledData::Binding::Type_Translation:
    case QV4::CompiledData::Binding::Type_TranslationById:
    case QV4::CompiledData::Binding::Type_Script:
        if (delayed) {
            if (!delayedValues)
                createDelayedValues();
            const QString delayedName = QString::number(entries.size());
            delayedValues->insert(delayedName, QVariant());
            QQmlProperty bindingTarget = QQmlProperty(delayedValues.get(), delayedName);
            Q_ASSERT(bindingTarget.isValid());
            QQmlAnyBinding anyBinding = createBinding(
                    bindingTarget, binding, compilationUnit, contextData, q);
            anyBinding.installOn(bindingTarget);
        } else {
            setBinding(createBinding(entry.prop, binding, compilationUnit, contextData, q));
        }
        break;
    case QV4::CompiledData::Binding::Type_String:
        setVariant(compilationUnit->bindingValueAsString(binding));
        break;
    case QV4::CompiledData::Binding::Type_Number:
        setVariant(compilationUnit->bindingValueAsNumber(binding));
        break;
    case QV4::CompiledData::Binding::Type_Boolean:
        setVariant(binding->valueAsBoolean());
        break;
    case QV4::CompiledData::Binding::Type_Null:
        setVariant(QVariant::fromValue(nullptr));
        break;
    case QV4::CompiledData::Binding::Type_Object:
    case QV4::CompiledData::Binding::Type_Invalid:
        break;
    }

    entries.append(std::move(entry));
}

void QQmlBindPrivate::createDelayedValues()
{
    delayedValues = std::make_unique<QQmlPropertyMap>();
    QObject::connect(
            delayedValues.get(), &QQmlPropertyMap::valueChanged,
            delayedValues.get(), [this](QString delayedName, const QVariant &value) {
                Q_UNUSED(value);
                onDelayedValueChanged(std::move(delayedName));
            }
    );
}

void QQmlBindPrivate::onDelayedValueChanged(QString delayedName)
{
    Q_ASSERT(delayed);
    Q_ASSERT(delayedValues);
    const QString pendingName = QStringLiteral("pending");
    QStringList pending = qvariant_cast<QStringList>((*delayedValues)[pendingName]);
    if (componentComplete && pending.size() == 0)
        QTimer::singleShot(0, delayedValues.get(), [this]() { evalDelayed(); });
    else if (pending.contains(delayedName))
        return;

    pending.append(std::move(delayedName));
    (*delayedValues)[pendingName].setValue(std::move(pending));
}

void QQmlBindPrivate::evalDelayed()
{
    if (!when || !delayedValues)
        return;

    const QString pendingName = QStringLiteral("pending");
    const QStringList pending = qvariant_cast<QStringList>((*delayedValues)[pendingName]);
    for (const QString &delayedName : pending) {
        bool ok;
        const int delayedIndex = delayedName.toInt(&ok);
        Q_ASSERT(ok);
        Q_ASSERT(delayedIndex >= 0 && delayedIndex < entries.size());
        entries[delayedIndex].prop.write((*delayedValues)[delayedName]);
    }
    (*delayedValues)[pendingName].setValue(QStringList());
}

void QQmlBindPrivate::buildBindEntries(QQmlBind *q, QQmlComponentPrivate::DeferredState *deferredState)
{
    QQmlData *data = QQmlData::get(q);
    if (data && !data->deferredData.isEmpty()) {
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(data->context->engine());
        for (QQmlData::DeferredData *deferredData : data->deferredData) {
            QMultiHash<int, const QV4::CompiledData::Binding *> *bindings = &deferredData->bindings;
            if (deferredState) {
                QQmlComponentPrivate::ConstructionState constructionState;
                for (auto it = bindings->cbegin(); it != bindings->cend(); ++it)
                    decodeBinding(q, QString(), deferredData, *it, &constructionState);


                if (constructionState.hasCreator()) {
                    ++ep->inProgressCreations;
                    constructionState.creator()->finalizePopulateDeferred();
                    constructionState.appendCreatorErrors();
                    deferredState->push_back(std::move(constructionState));
                }
            } else {
                for (auto it = bindings->cbegin(); it != bindings->cend(); ++it)
                    decodeBinding(q, QString(), deferredData, *it, nullptr);
            }
        }

        if (deferredState) {
            data->releaseDeferredData();
            if (!deferredState->empty())
                QQmlComponentPrivate::completeDeferred(ep, deferredState);
        }
    }
}

void QQmlBind::componentComplete()
{
    Q_D(QQmlBind);
    QQmlComponentPrivate::DeferredState deferredState;
    d->buildBindEntries(this, &deferredState);
    d->componentComplete = true;
    if (!d->propName.isEmpty() || d->obj) {
        QQmlBindEntry *target = d->targetEntry();
        if (!target->prop.isValid())
            target->setTarget(this, QQmlProperty(d->obj, d->propName, qmlContext(this)));
    }
    d->validate(this);
    d->evalDelayed();
    eval();
}

void QQmlBind::prepareEval()
{
    Q_D(QQmlBind);
    if (d->delayed) {
        if (!d->pendingEval)
            QTimer::singleShot(0, this, &QQmlBind::eval);
        d->pendingEval = true;
    } else {
        eval();
    }
}

void QQmlBindEntry::clearPrev()
{
    previousKind = previous.destroy(previousKind);
}

void QQmlBind::eval()
{
    Q_D(QQmlBind);
    d->pendingEval = false;
    if (!d->componentComplete)
        return;

    for (QQmlBindEntry &entry : d->entries) {
        if (!entry.prop.isValid() || (entry.currentKind == QQmlBindEntryKind::None))
            continue;

        if (!d->when) {
            //restore any previous binding
            switch (entry.previousKind) {
            case QQmlBindEntryKind::Binding:
                if (d->restoreBinding) {
                    QQmlAnyBinding p = std::move(entry.previous.binding);
                    entry.clearPrev(); // Do that before setBinding(), as setBinding() may recurse.
                    p.installOn(entry.prop);
                }
                break;
            case QQmlBindEntryKind::V4Value:
                if (d->restoreValue) {
                    auto propPriv = QQmlPropertyPrivate::get(entry.prop);
                    QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(propPriv->object);
                    Q_ASSERT(vmemo);
                    vmemo->setVMEProperty(propPriv->core.coreIndex(),
                                          *entry.previous.v4Value.valueRef());
                    entry.clearPrev();
                }
                break;
            case QQmlBindEntryKind::Variant:
                if (d->restoreValue) {
                    entry.prop.write(entry.previous.variant);
                    entry.clearPrev();
                }
                break;
            case QQmlBindEntryKind::None:
                break;
            }
            continue;
        }

        //save any set binding for restoration
        if (entry.previousKind == QQmlBindEntryKind::None) {
            // try binding first; we need to use takeFrom to properly unlink the binding
            QQmlAnyBinding prevBind = QQmlAnyBinding::takeFrom(entry.prop);
            if (prevBind) {
                entry.previousKind = entry.previous.set(std::move(prevBind), entry.previousKind);
            } else {
                // nope, try a V4 value next
                auto propPriv = QQmlPropertyPrivate::get(entry.prop);
                auto propData = propPriv->core;
                if (!propPriv->valueTypeData.isValid() && propData.isVarProperty()) {
                    QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(propPriv->object);
                    Q_ASSERT(vmemo);
                    auto retVal = vmemo->vmeProperty(propData.coreIndex());
                    entry.previousKind = entry.previous.set(
                                QV4::PersistentValue(vmemo->engine, retVal), entry.previousKind);
                } else {
                    // nope, use the meta object to get a QVariant
                    entry.previousKind = entry.previous.set(entry.prop.read(), entry.previousKind);
                }
            }
        }

        // NOTE: removeBinding has no effect on QProperty classes, but
        // we already used takeBinding to remove it
        QQmlPropertyPrivate::removeBinding(entry.prop);
    }

    if (!d->when)
        return;

    d->writingProperty = true;
    for (qsizetype i = 0, end = d->entries.size(); i != end; ++i) {
        QQmlBindEntry &entry = d->entries[i];
        if (!entry.prop.isValid())
            continue;
        switch (entry.currentKind) {
        case QQmlBindEntryKind::Variant:
            entry.prop.write(entry.current.variant);
            break;
        case QQmlBindEntryKind::Binding:
            Q_ASSERT(!d->delayed);
            entry.current.binding.installOn(entry.prop);
            break;
        case QQmlBindEntryKind::V4Value: {
            auto propPriv = QQmlPropertyPrivate::get(entry.prop);
            QQmlVMEMetaObject::get(propPriv->object)->setVMEProperty(
                        propPriv->core.coreIndex(), *entry.current.v4Value.valueRef());
            break;
        }
        case QQmlBindEntryKind::None:
            break;
        }
    }
    d->writingProperty = false;
}

void QQmlBind::targetValueChanged()
{
    Q_D(QQmlBind);
    if (d->writingProperty)
        return;

    if (!d->when)
        return;

    QUrl url;
    quint16 line = 0;

    const QQmlData *ddata = QQmlData::get(this, false);
    if (ddata && ddata->outerContext) {
        url = ddata->outerContext->url();
        line = ddata->lineNumber;
    }

    qCInfo(lcBindingRemoval,
           "The target property of the Binding element created at %s:%d was changed from "
           "elsewhere. This does not overwrite the binding. The target property will still be "
           "updated when the value of the Binding element changes.",
           qPrintable(url.toString()), line);
}

QT_END_NAMESPACE

#include "moc_qqmlbind_p.cpp"
