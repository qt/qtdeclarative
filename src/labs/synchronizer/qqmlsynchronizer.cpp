// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlsynchronizer_p.h"

#include <private/qobject_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qv4scopedvalue_p.h>

#include <QtQml/qqmlinfo.h>
#include <QtCore/qcompare.h>

QT_BEGIN_NAMESPACE

struct QQmlSynchronizerProperty
{
    QQmlSynchronizerProperty() = default;
    QQmlSynchronizerProperty(
            QObject *object, const QQmlPropertyData *core,
            const QQmlPropertyData *valueTypeData = nullptr)
        : m_object(object)
        , m_core(core)
        , m_valueTypeData(valueTypeData)
    {}

    bool isValid() const { return m_object != nullptr && m_core != nullptr; }

    QVariant read() const;
    void write(QVariant &&value) const;

    QObject *object() const { return m_object; }
    const QQmlPropertyData *core() const { return m_core; }
    const QQmlPropertyData *valueTypeData() const { return m_valueTypeData; }

    QString name() const
    {
        if (!m_core)
            return QString();

        const QString coreName = m_core->name(m_object);

        const QQmlPropertyData *vt = valueTypeData();
        if (!vt)
            return coreName;

        const QMetaObject *vtMetaObject = QQmlMetaType::metaObjectForValueType(m_core->propType());
        Q_ASSERT(vtMetaObject);
        const char *vtName = vtMetaObject->property(vt->coreIndex()).name();
        return coreName + QLatin1Char('.') + QString::fromUtf8(vtName);

    }

    int notifyIndex() const { return m_core->notifyIndex(); }

    void clear()
    {
        m_object = nullptr;
        m_core = nullptr;
        m_valueTypeData = nullptr;
    }

    QVariant coerce(const QVariant &source, QQmlSynchronizer *q) const;

private:
    QPointer<QObject> m_object;
    const QQmlPropertyData *m_core = nullptr;
    const QQmlPropertyData *m_valueTypeData = nullptr;

    friend bool comparesEqual(
            const QQmlSynchronizerProperty &a, const QQmlSynchronizerProperty &b) noexcept
    {
        return a.m_core == b.m_core && a.m_valueTypeData == b.m_valueTypeData
                && a.m_object == b.m_object;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QQmlSynchronizerProperty)

    friend size_t qHash(const QQmlSynchronizerProperty &v, size_t seed = 0)
    {
        // We'd better not use m_object here since it may turn to nullptr spontaneously.
        // This results in a weaker hash, but we can live with it.
        return qHashMulti(seed, v.m_core, v.m_valueTypeData);
    }


    QMetaType metaType() const
    {
        return m_valueTypeData
                ? m_valueTypeData->propType()
                : m_core->propType();
    }
};

class QQmlSynchronizerSlotObject : public QtPrivate::QSlotObjectBase
{
public:
    QQmlSynchronizerSlotObject(
            QQmlSynchronizer *receiver, const QQmlSynchronizerProperty &property)
        : QtPrivate::QSlotObjectBase(&impl)
        , m_property(property)
        , m_connection(createConnection(receiver))
    {
    }

    ~QQmlSynchronizerSlotObject()
    {
        QObject::disconnect(m_connection);
    }

    bool contains(const QQmlSynchronizerProperty &p) const { return m_property == p; }
    void write(QVariant value) const { m_property.write(std::move(value)); }
    QVariant coerce(const QVariant &source, QQmlSynchronizer *q) const
    {
        return m_property.coerce(source, q);
    }
    QQmlSynchronizerProperty property() const { return m_property; }

    static void impl(int which, QtPrivate::QSlotObjectBase *self, QObject *r, void **a, bool *ret);
    void operator()(QQmlSynchronizer *receiver)
    {
        impl(QtPrivate::QSlotObjectBase::Call, this, receiver, nullptr, nullptr);
    }

    void addref() { ref(); }
    void release() { destroyIfLastRef(); }

private:
    QMetaObject::Connection createConnection(QQmlSynchronizer *receiver)
    {
        Q_ASSERT(receiver);
        QObject *object = m_property.object();
        Q_ASSERT(object);
        const int notifyIndex = m_property.notifyIndex();
        Q_ASSERT(notifyIndex != -1);

        return QObjectPrivate::connectImpl(
                object, notifyIndex, receiver, nullptr, this, Qt::AutoConnection, nullptr,
                object->metaObject());
    }


    QQmlSynchronizerProperty m_property;
    QMetaObject::Connection m_connection;
};

class QQmlSynchronizerHandler
{
public:
    QQmlSynchronizerHandler(
            QQmlSynchronizerPrivate *receiver, const QQmlSynchronizerProperty &property)
        : m_property(property)
        , m_synchronizer(receiver)
    {}

    bool contains(const QQmlSynchronizerProperty &p) const { return m_property == p; }
    void write(QVariant value) const { m_property.write(std::move(value)); }
    QVariant coerce(const QVariant &source, QQmlSynchronizer *q) const
    {
        return m_property.coerce(source, q);
    }
    QQmlSynchronizerProperty property() const { return m_property; }

    void operator()() const;

protected:
    QPropertyChangeHandler<QQmlSynchronizerHandler> createChangeHandler()
    {
        const QQmlPropertyData *core = m_property.core();
        Q_ASSERT(core && core->isValid());
        QObject *object = m_property.object();
        Q_ASSERT(object);

        QUntypedBindable bindable;
        void *argv[1] { &bindable };
        core->doMetacall<QMetaObject::BindableProperty>(object, core->coreIndex(), argv);
        Q_ASSERT(bindable.isValid());

        return bindable.onValueChanged(*this);
    }

private:
    QQmlSynchronizerProperty m_property;
    QQmlSynchronizerPrivate *m_synchronizer = nullptr;
};

class QQmlSynchronizerChangeHandler : public QQmlSynchronizerHandler
{
public:
    QQmlSynchronizerChangeHandler(
            QQmlSynchronizerPrivate *receiver, const QQmlSynchronizerProperty &property)
        : QQmlSynchronizerHandler(receiver, property)
        , changeHandler(createChangeHandler())
    {}

private:
    QPropertyChangeHandler<QQmlSynchronizerHandler> changeHandler;
};

class QQmlSynchronizerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlSynchronizer)
public:
    enum Result
    {
        Origin,
        Accepted,
        Bounced,
        Ignored
    };

    struct State
    {
        QVariant value;
        QHash<QQmlSynchronizerProperty, Result> results;
    };

    struct OwnedTarget
    {
        QPointer<QObject> object;
        std::unique_ptr<const QQmlPropertyData> core;
        std::unique_ptr<const QQmlPropertyData> auxiliary;
    };

    static QQmlSynchronizerPrivate *get(QQmlSynchronizer *q) { return q->d_func(); }

    std::vector<QQmlRefPointer<QQmlSynchronizerSlotObject>> slotObjects;
    std::vector<QQmlSynchronizerChangeHandler> changeHandlers;

    // Target set by QQmlPropertyValueSource
    // This can't change, but we have to hold on to the QQmlPropertyData.
    OwnedTarget target;

    // Target set using sourceObject/sourceProperty properties
    // The string is the primary source of truth for sourceProperty.
    // The QQmlPropertyData are only used if the object does not have a property cache.
    QString sourceProperty;
    OwnedTarget sourceObjectProperty;

    // Target set using targetObject/targetProperty properties
    // The string is the primary source of truth for targetProperty.
    // The QQmlPropertyData are only used if the object does not have a property cache.
    QString targetProperty;
    OwnedTarget targetObjectProperty;

    State *currentState = nullptr;

    bool isComponentFinalized = false;

    void createConnection(QQmlSynchronizer *q, const QQmlSynchronizerProperty &property);

    void disconnectObjectProperty(const QString &property, OwnedTarget *objectProperty);
    QQmlSynchronizerProperty connectObjectProperty(
            const QString &property, OwnedTarget *objectProperty, QQmlSynchronizer *q);

    QQmlSynchronizerProperty connectTarget(QQmlSynchronizer *q);

    void initialize(QQmlSynchronizer *q);

    void synchronize(const QQmlSynchronizerProperty &property);
};

/*!
    \qmlmodule Qt.labs.synchronizer
    \title Qt Labs Synchronizer QML Types
    \ingroup qmlmodules
    \brief Synchronizer synchronizes values between two or more properties.

    To use this module, import the module with the following line:

    \qml
    import Qt.labs.synchronizer
    \endqml
*/

/*!
    \qmltype Synchronizer
    \inqmlmodule Qt.labs.synchronizer
    \since 6.10
    \brief Synchronizes values between two or more properties.

    A Synchronizer object binds two or more properties together so that a change
    to any one of them automatically updates all others. While doing so, none of
    the bindings on any of the properties are broken. You can use Synchronizer if
    the direction of data flow between two properties is not pre-determined. For
    example, a TextInput may be initialized with a model value but should also
    update the model value when editing is finished.

    \note The input elements provided by Qt Quick and Qt Quick Controls solve this
          problem by providing user interaction signals separate from value change
          signals and hiding the value assignments in C++ code. You \e{don't} need
          Synchronizer for their internals. However, it may still be useful when
          connecting a control to a model.

    Consider the following example.

    \section1 Without Synchronizer

    \qml
    // MyCustomTextInput.qml
    Item {
        property string text
        function append(characters: string) { text += characters }
        [...]
    }
    \endqml

    You may be inclined to populate the \c text property from a model and update
    the model when the \c textChanged signal is received.

    \qml
    // Does not work!
    Item {
        id: root
        property string model: "lorem ipsum"
        MyCustomTextInput {
            text: root.model
            onTextChanged: root.model = text
        }
    }
    \endqml

    This does \e not work. When the \c append function is called, the
    \c text property is modified, and the binding that updates it from the
    \c model property is broken. The next time the \c model is updated
    independently \c text is not updated anymore.

    To solve this, you can omit the binding altogether and use only signals
    for updating both properties. This way you would need to give up the
    convenience of bindings.

    Or, you can use Synchronizer.

    \section1 With Synchronizer

    \qml
    Item {
        id: root
        property string model: "lorem ipsum"
        MyCustomTextInput {
            Synchronizer on text {
                property alias source: root.model
            }
        }
    }
    \endqml

    Synchronizer makes sure that whenever either the model or
    the text change, the other one is updated.

    You can specify properties to be synchronized in several ways:
    \list
        \li Using the \c on syntax
        \li Populating the \c sourceObject and \c sourceProperty properties
        \li Populating the \c targetObject and \c targetProperty properties
        \li Creating aliases in the scope of the synchronizer
    \endlist

    The following example synchronizes four different properties,
    exercising all the different options:

    \qml
    Item {
        id: root
        property string model: "lorem ipsum"

        MyCustomTextInput {
            Synchronizer on text {
                sourceObject: other
                sourceProperty: "text"

                targetObject: root.children[0]
                targetProperty: "objectName"

                property alias source: root.model
                property alias another: root.objectName
            }
        }

        MyCustomTextInput {
            id: other
        }
    }
    \endqml

    Optionally, Synchronizer will perform an initial synchronization:
    \list
        \li If one of the aliases is called \c{source}, then it will be used to
            initialize the other properties.
        \li Otherwise, if the values assigned to \l{sourceObject} and
            \l{sourceProperty} denote a property, that property will be used
            as source for initial synchronization.
        \li Otherwise, if the \c on syntax is used, the property on which the
            Synchronizer is created that way is used as source for initial
            synchronization.
        \li Otherwise no initial synchronization is performed. Only when one of
            the properties changes the others will be updated.
    \endlist

    Synchronizer automatically \e{de-bounces}. While it is synchronizing
    using a given value as the source, it does not accept further updates from
    one of the properties expected to be the target of the update. Such
    behavior would otherwise easily lead to infinite update loops.
    Synchronizer uses the \l{valueBounced} signal to notify about this
    condition. Furthermore, it detects properties that silently
    refuse an update and emits the \l{valueIgnored} signal for them.
    Silence, in this context, is determined by the lack of a change signal
    after calling the setter for the given property.

    If the properties to be synchronized are of different types, the usual
    QML type coercions are applied.

    \note It is not possible to create an alias to a property of a singleton.
    When using Synchronizer together with singletons, use \l{sourceObject} and
    \l{sourceProperty} and the respective target properties.
 */

QQmlSynchronizer::QQmlSynchronizer(QObject *parent)
    : QObject(*(new QQmlSynchronizerPrivate), parent)
{
}

void QQmlSynchronizer::setTarget(const QQmlProperty &target)
{
    Q_D(QQmlSynchronizer);

    // Should be set before component completion
    Q_ASSERT(!d->isComponentFinalized);

    QQmlPropertyPrivate *p = QQmlPropertyPrivate::get(target);
    d->target.object = p->object;
    d->target.core = std::make_unique<QQmlPropertyData>(p->core);
    d->target.auxiliary = p->valueTypeData.isValid()
            ? std::make_unique<QQmlPropertyData>(p->valueTypeData)
            : nullptr;
}

void QQmlSynchronizer::componentFinalized()
{
    Q_D(QQmlSynchronizer);

    d->isComponentFinalized = true;
    d->initialize(this);
}

/*!
    \qmlproperty QtObject Qt.labs.synchronizer::Synchronizer::sourceObject

    This property holds the \l{sourceObject} part of the
    \l{sourceObject}/\l{sourceProperty} pair that together can specify one of
    the properties Synchronizer will synchronize.
*/
QObject *QQmlSynchronizer::sourceObject() const
{
    Q_D(const QQmlSynchronizer);
    return d->sourceObjectProperty.object;
}

void QQmlSynchronizer::setSourceObject(QObject *object)
{
    Q_D(QQmlSynchronizer);
    if (object == d->sourceObjectProperty.object)
        return;

    if (d->isComponentFinalized)
        d->disconnectObjectProperty(d->sourceProperty, &d->sourceObjectProperty);

    d->sourceObjectProperty.object = object;
    emit sourceObjectChanged();

    if (d->isComponentFinalized)
        d->connectObjectProperty(d->sourceProperty, &d->sourceObjectProperty, this);
}

/*!
    \qmlproperty string Qt.labs.synchronizer::Synchronizer::sourceProperty

    This sourceProperty holds the \l{sourceProperty} part of the
    \l{sourceObject}/\l{sourceProperty} pair that together can specify one of
    the properties Synchronizer will synchronize.
*/
QString QQmlSynchronizer::sourceProperty() const
{
    Q_D(const QQmlSynchronizer);
    return d->sourceProperty;
}

void QQmlSynchronizer::setSourceProperty(const QString &property)
{
    Q_D(QQmlSynchronizer);
    if (property == d->sourceProperty)
        return;

    if (d->isComponentFinalized)
        d->disconnectObjectProperty(d->sourceProperty, &d->sourceObjectProperty);

    d->sourceProperty = property;
    emit sourcePropertyChanged();

    if (d->isComponentFinalized)
        d->connectObjectProperty(d->sourceProperty, &d->sourceObjectProperty, this);
}

/*!
    \qmlproperty QtObject Qt.labs.synchronizer::Synchronizer::targetObject

    This property holds the \l{targetObject} part of the
    \l{targetObject}/\l{targetProperty} pair that together can specify one of
    the properties Synchronizer will synchronize.
 */
QObject *QQmlSynchronizer::targetObject() const
{
    Q_D(const QQmlSynchronizer);
    return d->targetObjectProperty.object;
}

void QQmlSynchronizer::setTargetObject(QObject *object)
{
    Q_D(QQmlSynchronizer);
    if (object == d->targetObjectProperty.object)
        return;

    if (d->isComponentFinalized)
        d->disconnectObjectProperty(d->targetProperty, &d->targetObjectProperty);

    d->targetObjectProperty.object = object;
    emit targetObjectChanged();

    if (d->isComponentFinalized)
        d->connectObjectProperty(d->targetProperty, &d->targetObjectProperty, this);
}

/*!
    \qmlproperty string Qt.labs.synchronizer::Synchronizer::targetProperty

    This targetProperty holds the \l{targetProperty} part of the
    \l{targetObject}/\l{targetProperty} pair that together can specify one of
    the properties Synchronizer will synchronize.
*/
QString QQmlSynchronizer::targetProperty() const
{
    Q_D(const QQmlSynchronizer);
    return d->targetProperty;
}

void QQmlSynchronizer::setTargetProperty(const QString &property)
{
    Q_D(QQmlSynchronizer);
    if (property == d->targetProperty)
        return;

    if (d->isComponentFinalized)
        d->disconnectObjectProperty(d->targetProperty, &d->targetObjectProperty);

    d->targetProperty = property;
    emit targetPropertyChanged();

    if (d->isComponentFinalized)
        d->connectObjectProperty(d->targetProperty, &d->targetObjectProperty, this);
}

/*!
    \qmlsignal Qt.labs.synchronizer::Synchronizer::valueBounced(QtObject object, string property)

    This signal is emitted if the \a{property} of the \a{object} refused
    an attempt to set its value as part of the synchronization and produced a
    different value in response. Such \e{bounced} values are ignored and
    \e{don't} trigger another round of synchronization.
 */

/*!
    \qmlsignal Qt.labs.synchronizer::Synchronizer::valueIgnored(QtObject object, string property)

    This signal is emitted if \a{property} of the \a{object} did not
    respond to an attempt to set its value as part of the synchronization.
 */


void QQmlSynchronizerPrivate::createConnection(
        QQmlSynchronizer *q, const QQmlSynchronizerProperty &property)
{
    if (property.core()->notifiesViaBindable()) {
        changeHandlers.push_back(QQmlSynchronizerChangeHandler(this, property));
    } else if (const int notifyIndex = property.notifyIndex(); notifyIndex != -1) {
        slotObjects.push_back(QQmlRefPointer(
                new QQmlSynchronizerSlotObject(q, property),
                QQmlRefPointer<QQmlSynchronizerSlotObject>::AddRef));
    }
}

void QQmlSynchronizerPrivate::disconnectObjectProperty(
        const QString &property, OwnedTarget *objectProperty)
{
    QObject *object = objectProperty->object;
    if (!object || property.isEmpty())
        return;

    QQmlPropertyData localCore;
    const QQmlPropertyData *coreData = objectProperty->core
            ? objectProperty->core.get()
            : QQmlPropertyCache::property(object, property, {}, &localCore);
    if (!coreData || coreData->isFunction())
        return;

    const QQmlSynchronizerProperty synchronizerProperty = QQmlSynchronizerProperty(object, coreData);
    const auto slot = std::find_if(slotObjects.begin(), slotObjects.end(), [&](const auto &slot) {
        return slot->contains(synchronizerProperty);
    });

    if (slot == slotObjects.end()) {
        const auto handler
                = std::find_if(changeHandlers.begin(), changeHandlers.end(), [&](const auto &handler) {
            return handler.contains(synchronizerProperty);
        });

        Q_ASSERT(handler != changeHandlers.end());
        changeHandlers.erase(handler);
    } else {
        slotObjects.erase(slot);
    }

    objectProperty->core.reset();
    objectProperty->auxiliary.reset();
}

QQmlSynchronizerProperty QQmlSynchronizerPrivate::connectObjectProperty(
        const QString &property, OwnedTarget *objectProperty, QQmlSynchronizer *q)
{
    QObject *object = objectProperty->object;
    if (!object || property.isEmpty())
        return QQmlSynchronizerProperty();

    QQmlPropertyData localCore;
    const QQmlPropertyData *coreData = QQmlPropertyCache::property(object, property, {}, &localCore);
    if (!coreData) {
        qmlWarning(q) << "Target object has no property called " << property;
        return QQmlSynchronizerProperty();
    }

    if (coreData->isFunction()) {
        qmlWarning(q) << "Member " << property << " of target object is a function";
        return QQmlSynchronizerProperty();
    }

    if (coreData == &localCore) {
        objectProperty->core = std::make_unique<QQmlPropertyData>(std::move(localCore));
        coreData = objectProperty->core.get();
    }

    const QQmlSynchronizerProperty synchronizerProperty(object, coreData);
    createConnection(q, synchronizerProperty);
    return synchronizerProperty;
}

QQmlSynchronizerProperty QQmlSynchronizerPrivate::connectTarget(QQmlSynchronizer *q)
{
    QObject *object = target.object;
    if (!object)
        return QQmlSynchronizerProperty();

    const QQmlPropertyData *core = target.core.get();
    Q_ASSERT(core->isValid());

    if (const QQmlPropertyData *valueTypeData = target.auxiliary.get()) {
        const QQmlSynchronizerProperty property(object, core, valueTypeData);
        createConnection(q, property);
        return property;
    }

    const QQmlSynchronizerProperty property(object, core);
    createConnection(q, property);
    return property;
}

void QQmlSynchronizerPrivate::initialize(QQmlSynchronizer *q)
{
    changeHandlers.clear();
    slotObjects.clear();

    QQmlSynchronizerProperty initializationSource = connectTarget(q);
    if (QQmlSynchronizerProperty source = connectObjectProperty(
                sourceProperty, &sourceObjectProperty, q); source.isValid()) {
        initializationSource = source;
    }
    connectObjectProperty(targetProperty, &targetObjectProperty, q);

    const QQmlPropertyCache::ConstPtr propertyCache = QQmlData::ensurePropertyCache(q);
    Q_ASSERT(propertyCache);

    const int propertyCount = propertyCache->propertyCount();
    const int propertyOffset = QQmlSynchronizer::staticMetaObject.propertyCount();

    bool foundSource = false;
    for (int i = propertyOffset; i < propertyCount; ++i) {
        const QQmlPropertyData *property = propertyCache->property(i);
        if (!property->isAlias())
            continue;

        const QQmlSynchronizerProperty synchronizerProperty(q, property);
        createConnection(q, synchronizerProperty);

        if (!foundSource && property->name(q) == QLatin1String("source")) {
            initializationSource = synchronizerProperty;
            foundSource = true;
            continue;
        }
    }

    if (initializationSource.isValid())
        synchronize(initializationSource);
}

void QQmlSynchronizerPrivate::synchronize(const QQmlSynchronizerProperty &property)
{
    const QVariant value = property.read();

    if (currentState) {
        // There can be interesting logic attached to the target property that causes it
        // to change multiple times in a row. We only count the last change.
        currentState->results[property] = (value == currentState->value) ? Accepted : Bounced;
        return;
    }

    Q_Q(QQmlSynchronizer);

    State state;
    state.results[property] = Origin;
    const auto guard = QScopedValueRollback(currentState, &state);

    for (const auto &slotObject : slotObjects) {
        if (slotObject->contains(property))
            continue;
        state.results[slotObject->property()] = Ignored;
        state.value = slotObject->coerce(value, q);
        slotObject->write(state.value);
    }

    for (const QQmlSynchronizerChangeHandler &changeHandler : changeHandlers) {
        if (changeHandler.contains(property))
            continue;
        state.results[changeHandler.property()] = Ignored;
        state.value = changeHandler.coerce(value, q);
        changeHandler.write(state.value);
    }


    for (auto it = state.results.constBegin(), end = state.results.constEnd(); it != end; ++it) {
        switch (*it) {
        case Origin:
        case Accepted:
            break;
        case Bounced: {
            const QQmlSynchronizerProperty &key = it.key();
            emit q->valueBounced(key.object(), key.name());
            break;
        }
        case Ignored: {
            const QQmlSynchronizerProperty &key = it.key();
            emit q->valueIgnored(key.object(), key.name());
            break;
        }
        }
    }
}

void QQmlSynchronizerSlotObject::impl(
        int which, QSlotObjectBase *self, QObject *r, void **a, bool *ret)
{
    Q_UNUSED(a);
    QQmlSynchronizerSlotObject *synchronizerSlotObject
            = static_cast<QQmlSynchronizerSlotObject *>(self);
    switch (which) {
    case Destroy:
        delete synchronizerSlotObject;
        break;
    case Call: {
        QQmlSynchronizer *q = static_cast<QQmlSynchronizer *>(r);
        QQmlSynchronizerPrivate::get(q)->synchronize(synchronizerSlotObject->m_property);
        break;
    }
    case Compare:
        if (ret)
            *ret = false;
        break;
    case NumOperations:
        break;
    }
}

static QVariant doReadProperty(QObject *object, const QQmlPropertyData *property)
{
    const QMetaType metaType = property->propType();
    if (metaType == QMetaType::fromType<QVariant>()) {
        QVariant content;
        property->readProperty(object, &content);
        return content;
    }

    QVariant content(metaType);
    property->readProperty(object, content.data());
    return content;
}

QVariant QQmlSynchronizerProperty::read() const
{
    Q_ASSERT(m_object);
    Q_ASSERT(m_core);

    QVariant coreContent = doReadProperty(m_object.data(), m_core);

    if (!m_valueTypeData)
        return coreContent;

    if (QQmlGadgetPtrWrapper *wrapper
            = QQmlGadgetPtrWrapper::instance(qmlEngine(m_object), coreContent.metaType())) {
        return doReadProperty(wrapper, m_valueTypeData);
    }

    QQmlGadgetPtrWrapper wrapper(QQmlMetaType::valueType(coreContent.metaType()));
    return doReadProperty(&wrapper, m_valueTypeData);
}

void QQmlSynchronizerProperty::write(QVariant &&value) const
{
    Q_ASSERT(value.metaType() == metaType());

    if (!m_object) // Was disconnected in some way or other
        return;

    if (!m_valueTypeData) {
        m_core->writeProperty(m_object, value.data(), QQmlPropertyData::DontRemoveBinding);
        return;
    }

    QVariant coreContent = doReadProperty(m_object, m_core);

    if (QQmlGadgetPtrWrapper *wrapper
            = QQmlGadgetPtrWrapper::instance(qmlEngine(m_object), coreContent.metaType())) {
        m_valueTypeData->writeProperty(wrapper, value.data(), QQmlPropertyData::DontRemoveBinding);
        m_core->writeProperty(m_object, coreContent.data(), QQmlPropertyData::DontRemoveBinding);
        return;
    }

    QQmlGadgetPtrWrapper wrapper(QQmlMetaType::valueType(coreContent.metaType()));
    m_valueTypeData->writeProperty(&wrapper, value.data(), QQmlPropertyData::DontRemoveBinding);
    m_core->writeProperty(m_object, coreContent.data(), QQmlPropertyData::DontRemoveBinding);
}

QVariant QQmlSynchronizerProperty::coerce(const QVariant &source, QQmlSynchronizer *q) const
{
    Q_ASSERT(m_core);

    const QMetaType targetMetaType = m_valueTypeData
            ? m_valueTypeData->propType()
            : m_core->propType();
    const QMetaType sourceMetaType = source.metaType();
    if (targetMetaType == sourceMetaType)
        return source;

    QVariant target(targetMetaType);

    QQmlData *ddata = QQmlData::get(q);
    if (ddata && !ddata->jsWrapper.isNullOrUndefined()) {
        QV4::Scope scope(ddata->jsWrapper.engine());
        QV4::ScopedValue scoped(scope, scope.engine->fromData(sourceMetaType, source.constData()));
        if (QV4::ExecutionEngine::metaTypeFromJS(scoped, targetMetaType, target.data()))
            return target;
    }

    if (QMetaType::convert(sourceMetaType, source.constData(), targetMetaType, target.data()))
        return target;

    qmlWarning(q) << "Cannot convert from " << sourceMetaType.name() << " to " << targetMetaType.name();
    return target;
}

void QQmlSynchronizerHandler::operator()() const
{
    m_synchronizer->synchronize(m_property);
}

QT_END_NAMESPACE
