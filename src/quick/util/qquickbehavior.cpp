// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickbehavior_p.h"

#include "qquickanimation_p.h"
#include <qqmlcontext.h>
#include <qqmlinfo.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlengine_p.h>
#include <private/qabstractanimationjob_p.h>
#include <private/qquicktransition_p.h>

#include <private/qquickanimatorjob_p.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

/*!
   \internal
   \brief The UntypedProxyProperty class is a property used in Behavior to handle bindable properties.

   Whenever a bindable property with a Behavior gets a request for its bindable interface, we instead
   return the bindable interface of the UntypedProxyProperty. This causes all reads and writes to be
   intercepted to use \c m_storage instead; moreover, any installed binding will also use \c m_storage
   as the property data for the binding.

   The BehaviorPrivate acts as an observer, listening to changes of the proxy property. If those occur,
   QQuickBehavior::write is called with the new value, which will then adjust the actual property (playing
   animations if necessary).

   \warning The interception mechanism works only via the metaobject system, just like it is the case with
   non-binadble properties and writes. Bypassing the metaobject system can thus lead to inconsistent results;
   it is however currently safe, as we do not publically expose the classes, and the code in Quick plays
   nicely.
 */
class UntypedProxyProperty : public QUntypedPropertyData
{
    QtPrivate::QPropertyBindingData m_bindingData;
    QUntypedPropertyData *m_sourcePropertyData;
    const QtPrivate::QBindableInterface *m_sourceInterface;
    QVariant m_storage;
public:
    void static getter(const QUntypedPropertyData *d, void *value)
    {
        auto This = static_cast<const UntypedProxyProperty *>(d);
        // multiplexing: If the flag is set, we want to receive the metatype instead
        if (quintptr(value) & QtPrivate::QBindableInterface::MetaTypeAccessorFlag) {
            *reinterpret_cast<QMetaType *>(quintptr(value) &
                                           ~QtPrivate::QBindableInterface::MetaTypeAccessorFlag)
                    = This->type();
            return;
        }
        This->type().construct(value, This->m_storage.constData());
        This->m_bindingData.registerWithCurrentlyEvaluatingBinding();
    }

    void static setter(QUntypedPropertyData *d, const void *value)
    {
        auto This = static_cast<UntypedProxyProperty *>(d);
        This->type().construct(This->m_storage.data(), value);
        This->m_bindingData.notifyObservers(reinterpret_cast<QUntypedPropertyData *>(This->m_storage.data()));
    }

    static QUntypedPropertyBinding bindingGetter(const QUntypedPropertyData *d)
    {
        auto This = static_cast<const UntypedProxyProperty *>(d);
        return QUntypedPropertyBinding(This->m_bindingData.binding());
    }

    static QUntypedPropertyBinding bindingSetter(QUntypedPropertyData *d,
                                                 const QUntypedPropertyBinding &binding)
    {
        auto This = static_cast<UntypedProxyProperty *>(d);
        const QMetaType type = This->type();
        if (binding.valueMetaType() != type)
            return {};

        // We want to notify in any case here because the target property should be set
        // even if our proxy binding results in the default value.
        QPropertyBindingPrivate::get(binding)->scheduleNotify();
        return This->m_bindingData.setBinding(binding,
                                              reinterpret_cast<QUntypedPropertyData *>(
                                                  This->m_storage.data()));
    }

    static QUntypedPropertyBinding makeBinding(const QUntypedPropertyData *d,
                                               const QPropertyBindingSourceLocation &location)
    {
        auto This = static_cast<const UntypedProxyProperty *>(d);
        return This->m_sourceInterface->makeBinding(This->m_sourcePropertyData, location);
    }

    static void setObserver(const QUntypedPropertyData *d, QPropertyObserver *observer)
    {
        auto This = static_cast<const UntypedProxyProperty *>(d);
        This->m_sourceInterface->setObserver(This->m_sourcePropertyData, observer);
    }



    UntypedProxyProperty(QUntypedBindable bindable, QQuickBehaviorPrivate *behavior);

    QUntypedBindable getBindable();
    QMetaType type() const { return m_storage.metaType(); }
    QVariant value() const {return m_storage;}
};

static constexpr inline QtPrivate::QBindableInterface untypedProxyPropertyBindableInterafce {
    &UntypedProxyProperty::getter,
    &UntypedProxyProperty::setter,
    &UntypedProxyProperty::bindingGetter,
    &UntypedProxyProperty::bindingSetter,
    &UntypedProxyProperty::makeBinding,
    &UntypedProxyProperty::setObserver,
    /*metatype*/nullptr
};

struct UntypedProxyPropertyBindable : QUntypedBindable {
    UntypedProxyPropertyBindable(UntypedProxyProperty *property)
        :QUntypedBindable (property, &untypedProxyPropertyBindableInterafce)
    {}
};

QUntypedBindable UntypedProxyProperty::getBindable()
{
    return UntypedProxyPropertyBindable {const_cast<UntypedProxyProperty *>(this)};
}

class QQuickBehaviorPrivate : public QObjectPrivate, public QAnimationJobChangeListener, public QPropertyObserver
{
    Q_DECLARE_PUBLIC(QQuickBehavior)
public:
    static void onProxyChanged(QPropertyObserver *, QUntypedPropertyData *);
    QQuickBehaviorPrivate()
        : QPropertyObserver(&QQuickBehaviorPrivate::onProxyChanged) {}
    void animationStateChanged(QAbstractAnimationJob *, QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState) override;

    QQmlProperty property;
    QVariant targetValue;
    QPointer<QQuickAbstractAnimation> animation = nullptr;
    QAbstractAnimationJob *animationInstance = nullptr;
    std::unique_ptr<UntypedProxyProperty> propertyProxy;
    bool enabled = true;
    bool finalized = false;
    bool blockRunningChanged = false;

};

UntypedProxyProperty::UntypedProxyProperty(QUntypedBindable bindable, QQuickBehaviorPrivate *behavior) :
    m_sourcePropertyData(QUntypedBindablePrivate::getPropertyData(bindable)),
    m_sourceInterface(QUntypedBindablePrivate::getInterface(bindable)),
    m_storage(QVariant(bindable.metaType()))
{
    behavior->setSource(m_bindingData);
}

/*!
    \qmltype Behavior
    \instantiates QQuickBehavior
    \inqmlmodule QtQuick
    \ingroup qtquick-transitions-animations
    \ingroup qtquick-interceptors
    \brief Defines a default animation for a property change.

    A Behavior defines the default animation to be applied whenever a
    particular property value changes.

    For example, the following Behavior defines a NumberAnimation to be run
    whenever the \l Rectangle's \c width value changes. When the MouseArea
    is clicked, the \c width is changed, triggering the behavior's animation:

    \snippet qml/behavior.qml 0

    Note that a property cannot have more than one assigned Behavior. To provide
    multiple animations within a Behavior, use ParallelAnimation or
    SequentialAnimation.

    If a \l{Qt Quick States}{state change} has a \l Transition that matches the same property as a
    Behavior, the \l Transition animation overrides the Behavior for that
    state change. For general advice on using Behaviors to animate state changes, see
    \l{Using Qt Quick Behaviors with States}.

    \sa {Animation and Transitions in Qt Quick}, {Qt Quick Examples - Animation#Behaviors}{Behavior example}, {Qt QML}
*/


QQuickBehavior::QQuickBehavior(QObject *parent)
    : QObject(*(new QQuickBehaviorPrivate), parent)
{
}

QQuickBehavior::~QQuickBehavior()
{
    Q_D(QQuickBehavior);
    delete d->animationInstance;
}

/*!
    \qmlproperty Animation QtQuick::Behavior::animation
    \qmldefault

    This property holds the animation to run when the behavior is triggered.
*/

QQuickAbstractAnimation *QQuickBehavior::animation()
{
    Q_D(QQuickBehavior);
    return d->animation;
}

void QQuickBehavior::setAnimation(QQuickAbstractAnimation *animation)
{
    Q_D(QQuickBehavior);
    if (d->animation) {
        qmlWarning(this) << tr("Cannot change the animation assigned to a Behavior.");
        return;
    }

    d->animation = animation;
    if (d->animation) {
        d->animation->setDefaultTarget(d->property);
        d->animation->setDisableUserControl();
    }
}


void QQuickBehaviorPrivate::onProxyChanged(QPropertyObserver *observer, QUntypedPropertyData *)
{
    auto This = static_cast<QQuickBehaviorPrivate *>(observer);
    This->q_func()->write(This->propertyProxy->value());
}

void QQuickBehaviorPrivate::animationStateChanged(QAbstractAnimationJob *, QAbstractAnimationJob::State newState,QAbstractAnimationJob::State)
{
    if (!blockRunningChanged && animation)
        animation->notifyRunningChanged(newState == QAbstractAnimationJob::Running);
}

/*!
    \qmlproperty bool QtQuick::Behavior::enabled

    This property holds whether the behavior will be triggered when the tracked
    property changes value.

    By default a Behavior is enabled.
*/

bool QQuickBehavior::enabled() const
{
    Q_D(const QQuickBehavior);
    return d->enabled;
}

void QQuickBehavior::setEnabled(bool enabled)
{
    Q_D(QQuickBehavior);
    if (d->enabled == enabled)
        return;
    d->enabled = enabled;
    emit enabledChanged();
}

/*!
    \qmlproperty Variant QtQuick::Behavior::targetValue

    This property holds the target value of the property being controlled by the Behavior.
    This value is set by the Behavior before the animation is started.

    \since QtQuick 2.13
*/
QVariant QQuickBehavior::targetValue() const
{
    Q_D(const QQuickBehavior);
    return d->targetValue;
}

/*!
    \readonly
    \qmlpropertygroup QtQuick::Behavior::targetProperty
    \qmlproperty string QtQuick::Behavior::targetProperty.name
    \qmlproperty QtObject QtQuick::Behavior::targetProperty.object

    \table
    \header
        \li Property
        \li Description
    \row
        \li name
        \li This property holds the name of the property being controlled by this Behavior.
    \row
        \li object
        \li This property holds the object of the property being controlled by this Behavior.
    \endtable

    This property can be used to define custom behaviors based on the name or the object of
    the property being controlled.

    The following example defines a Behavior fading out and fading in its target object
    when the property it controls changes:
    \qml
    // FadeBehavior.qml
    import QtQuick 2.15

    Behavior {
        id: root
        property Item fadeTarget: targetProperty.object
        SequentialAnimation {
            NumberAnimation {
                target: root.fadeTarget
                property: "opacity"
                to: 0
                easing.type: Easing.InQuad
            }
            PropertyAction { } // actually change the controlled property between the 2 other animations
            NumberAnimation {
                target: root.fadeTarget
                property: "opacity"
                to: 1
                easing.type: Easing.OutQuad
            }
        }
    }
    \endqml

    This can be used to animate a text when it changes:
    \qml
    import QtQuick 2.15

    Text {
        id: root
        property int counter
        text: counter
        FadeBehavior on text {}
        Timer {
            running: true
            repeat: true
            interval: 1000
            onTriggered: ++root.counter
        }
    }
    \endqml

    \since QtQuick 2.15
*/
QQmlProperty QQuickBehavior::targetProperty() const
{
    Q_D(const QQuickBehavior);
    return d->property;
}

void QQuickBehavior::componentFinalized()
{
    Q_D(QQuickBehavior);
    d->finalized = true;
}

void QQuickBehavior::write(const QVariant &value)
{
    Q_D(QQuickBehavior);
    const bool targetValueHasChanged = d->targetValue != value;
    if (targetValueHasChanged) {
        d->targetValue = value;
        emit targetValueChanged(); // emitting the signal here should allow
    }                              // d->enabled to change if scripted by the user.
    bool bypass = !d->enabled || !d->finalized || QQmlEnginePrivate::designerMode();
    if (!bypass)
        qmlExecuteDeferred(this);
    if (QQmlData::wasDeleted(d->animation) || bypass) {
        if (d->animationInstance)
            d->animationInstance->stop();
        QQmlPropertyPrivate::write(d->property, value, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
        return;
    }

    bool behaviorActive = d->animation->isRunning();
    if (behaviorActive && !targetValueHasChanged)
        return;

    if (d->animationInstance
            && (d->animationInstance->duration() != -1
                || d->animationInstance->isRenderThreadProxy())
            && !d->animationInstance->isStopped()) {
        d->blockRunningChanged = true;
        d->animationInstance->stop();
    }
    // Render thread animations use "stop" to synchronize the property back
    // to the item, so we need to read the value after.
    const QVariant &currentValue = d->property.read();

    // Don't unnecessarily wake up the animation system if no real animation
    // is needed (value has not changed). If the Behavior was already
    // running, let it continue as normal to ensure correct behavior and state.
    if (!behaviorActive && d->targetValue == currentValue) {
        QQmlPropertyPrivate::write(d->property, value, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
        return;
    }

    QQuickStateOperation::ActionList actions;
    QQuickStateAction action;
    action.property = d->property;
    action.fromValue = currentValue;
    action.toValue = value;
    actions << action;

    QList<QQmlProperty> after;
    auto *newInstance = d->animation->transition(actions, after, QQuickAbstractAnimation::Forward);
    Q_ASSERT(!newInstance || newInstance != d->animationInstance);
    delete d->animationInstance;
    d->animationInstance = newInstance;

    if (d->animationInstance) {
        if (d->animation->threadingModel() == QQuickAbstractAnimation::RenderThread)
            d->animationInstance = new QQuickAnimatorProxyJob(d->animationInstance, d->animation);

        d->animationInstance->addAnimationChangeListener(d, QAbstractAnimationJob::StateChange);
        d->animationInstance->start();
        d->blockRunningChanged = false;
    }

    if (!after.contains(d->property))
        QQmlPropertyPrivate::write(d->property, value, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
}

bool QQuickBehavior::bindable(QUntypedBindable *untypedBindable, QUntypedBindable target)
{
    Q_D(QQuickBehavior);
    if (!d->propertyProxy)
        d->propertyProxy = std::make_unique<UntypedProxyProperty>(target, d);
    *untypedBindable = d->propertyProxy->getBindable();
    return true;
}

void QQuickBehavior::setTarget(const QQmlProperty &property)
{
    Q_D(QQuickBehavior);
    d->property = property;
    if (d->animation)
        d->animation->setDefaultTarget(property);

    if (QMetaProperty metaProp = property.property(); metaProp.isBindable()) {
        QUntypedBindable untypedBindable = metaProp.bindable(property.object());
        d->propertyProxy = std::make_unique<UntypedProxyProperty>(untypedBindable, d);
        if (untypedBindable.hasBinding()) {
            // should not happen as bindings should get initialized only after interceptors
            UntypedProxyProperty::bindingSetter(d->propertyProxy.get(), untypedBindable.takeBinding());
        }
    }

    Q_EMIT targetPropertyChanged();
}

QT_END_NAMESPACE

#include "moc_qquickbehavior_p.cpp"
