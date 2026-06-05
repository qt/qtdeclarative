// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquicktooltip_p.h"
#include "qquicktooltip_p_p.h"
#include "qquickpopup_p_p.h"
#include "qquickpopupitem_p_p.h"
#include "qquickcontrol_p_p.h"

#include <QtCore/qbasictimer.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ToolTip
    \inherits Popup
//!     \nativetype QQuickToolTip
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-popups
    \brief Provides tool tips for any control.

    A tool tip is a short piece of text that informs the user of a control's
    function. It is typically placed above or below the parent control. The
    tip text can be any \l{Rich Text Processing}{rich text} formatted string.

    \image qtquickcontrols-tooltip.png
           {Tooltip displaying helpful text}

    \section2 Attached Tool Tips

    The most straight-forward way to setup tool tips for controls is to
    specify \l text and \l {visible}{visibility} via attached properties.
    The following example illustrates this approach:

    \snippet qtquickcontrols-tooltip.qml 1

    Under normal circumstances, there is only one tool tip visible at a time.
    In order to save resources, all items that use the ToolTip attached property
    share the same visual tool tip label instance. Even though the visuals are
    shared, \c text, \c timeout and \c delay are stored individually for each item
    that uses the respective attached property. However, multiple items cannot
    make the shared tool tip visible at the same time. The shared tool tip is only
    shown for the last item that made it visible. The position of the shared tool
    tip is determined by the framework.

    \include qquicktooltip.qdocinc customize-note

    \section2 Delay and Timeout

    Tool tips are typically transient in a sense that they are shown as a
    result of a certain external event or user interaction, and they usually
    hide after a certain timeout. It is possible to control the delay when
    a tool tip is shown, and the timeout when it is hidden. This makes it
    possible to implement varying strategies for showing and hiding tool tips.

    For example, on touch screens, it is a common pattern to show a tool tip
    as a result of pressing and holding down a button. The following example
    demonstrates how to delay showing a tool tip until the press-and-hold
    interval is reached. In this example, the tool tip hides as soon as the
    button is released.

    \snippet qtquickcontrols-tooltip-pressandhold.qml 1

    With pointer devices, however, it might be desired to show a tool tip as
    a result of hovering a button for a while. The following example presents
    how to show a tool tip after hovering a button for a second, and hide it
    after a timeout of five seconds.

    \snippet qtquickcontrols-tooltip-hover.qml 1

    \section2 Custom Tool Tips

    Should one need more fine-grained control over the tool tip position, or
    multiple simultaneous tool tip instances are needed, it is also possible
    to create local tool tip instances. This way, it is possible to
    \l {Customizing ToolTip}{customize} the tool tip, and the whole \l Popup
    API is available. The following example presents a tool tip that presents
    the value of a slider when the handle is dragged.

    \image qtquickcontrols-tooltip-slider.png
           {Tooltip attached to slider showing current value}

    \snippet qtquickcontrols-tooltip-slider.qml 1

    \sa {Customizing ToolTip}, {Popup Controls},
    {QtQuick.Controls::Popup::closePolicy}{closePolicy}
*/

// These enable auto tests to test the default behaviour of delay and timeout when using
// the automatic policy, while also shortening their execution time.
#ifdef QT_BUILD_INTERNAL
Q_CONSTINIT Q_AUTOTEST_EXPORT
#else
// Can't be constexpr because we set it in our constructor (which we have to do because
// toolTipWakeUpDelay() is not constexpr).
static
#endif
int qt_quicktooltipattachedprivate_delay = -1;

#ifdef QT_BUILD_INTERNAL
Q_CONSTINIT Q_AUTOTEST_EXPORT
#else
constexpr
#endif
bool qt_quicktooltipattachedprivate_short_timeout = false;

class QQuickToolTipPrivate : public QQuickPopupPrivate
{
    Q_DECLARE_PUBLIC(QQuickToolTip)

public:
    QQuickToolTipPrivate();

    void startDelay();
    void stopDelay();

    void startTimeout();
    void stopTimeout();

    void opened() override;

    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::ToolTip); }

    int delay = 0;
    int timeout = -1;
    QString text;
    QBasicTimer delayTimer;
    QBasicTimer timeoutTimer;
};

QQuickToolTipPrivate::QQuickToolTipPrivate()
{
    windowFlags = Qt::ToolTip;
    if (qt_quicktooltipattachedprivate_delay == -1)
        qt_quicktooltipattachedprivate_delay = qGuiApp->styleHints()->toolTipWakeUpDelay();
#if QT_CONFIG(wayland)
    extendedWindowType = QNativeInterface::Private::QWaylandWindow::ToolTip;
#endif
}

void QQuickToolTipPrivate::startDelay()
{
    Q_Q(QQuickToolTip);
    if (delay > 0)
        delayTimer.start(delay, q);
}

void QQuickToolTipPrivate::stopDelay()
{
    delayTimer.stop();
}

void QQuickToolTipPrivate::startTimeout()
{
    Q_Q(QQuickToolTip);
    if (timeout > 0)
        timeoutTimer.start(timeout, q);
}

void QQuickToolTipPrivate::stopTimeout()
{
    timeoutTimer.stop();
}

void QQuickToolTipPrivate::opened()
{
    QQuickPopupPrivate::opened();
    startTimeout();
}

QQuickToolTip::QQuickToolTip(QQuickItem *parent)
    : QQuickPopup(*(new QQuickToolTipPrivate), parent)
{
    Q_D(QQuickToolTip);
    d->allowVerticalFlip = true;
    d->allowHorizontalFlip = true;
    d->popupItem->setHoverEnabled(false); // QTBUG-63644
}

/*!
    \qmlproperty string QtQuick.Controls::ToolTip::text

    This property holds the text shown on the tool tip.
*/
QString QQuickToolTip::text() const
{
    Q_D(const QQuickToolTip);
    return d->text;
}

void QQuickToolTip::setText(const QString &text)
{
    Q_D(QQuickToolTip);
    if (d->text == text)
        return;

    d->text = text;
    maybeSetAccessibleName(text);
    emit textChanged();
}

/*!
    \qmlproperty int QtQuick.Controls::ToolTip::delay

    This property holds the delay (milliseconds) after which the tool tip is
    shown. A tooltip with a negative delay is shown immediately. The default
    value is \c 0.

    \sa {Delay and Timeout}
*/
int QQuickToolTip::delay() const
{
    Q_D(const QQuickToolTip);
    return d->delay;
}

void QQuickToolTip::setDelay(int delay)
{
    Q_D(QQuickToolTip);
    if (d->delay == delay)
        return;

    d->delay = delay;
    emit delayChanged();
}

/*!
    \qmlproperty int QtQuick.Controls::ToolTip::timeout

    This property holds the timeout (milliseconds) after which the tool tip is
    hidden. A tooltip with a negative timeout does not hide automatically. The
    default value is \c -1.

    \sa {Delay and Timeout}
*/
int QQuickToolTip::timeout() const
{
    Q_D(const QQuickToolTip);
    return d->timeout;
}

void QQuickToolTip::setTimeout(int timeout)
{
    Q_D(QQuickToolTip);
    if (d->timeout == timeout)
        return;

    d->timeout = timeout;

    if (timeout <= 0)
        d->stopTimeout();
    else if (isOpened())
        d->startTimeout();

    emit timeoutChanged();
}

void QQuickToolTip::setVisible(bool visible)
{
    Q_D(QQuickToolTip);
    if (visible) {
        if (!d->visible) {
            // We are being made visible, and we weren't before.
            if (d->delay > 0) {
                d->startDelay();
                return;
            }
        }
    } else {
        d->stopDelay();
    }
    QQuickPopup::setVisible(visible);
}

QQuickToolTipAttached *QQuickToolTip::qmlAttachedProperties(QObject *object)
{
    return new QQuickToolTipAttached(object);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlmethod void QtQuick.Controls::ToolTip::show(string text, int timeout)

    This method shows the \a text as a tooltip, which times out in
    \a timeout (milliseconds).
*/
void QQuickToolTip::show(const QString &text, int ms)
{
    if (ms >= 0)
        setTimeout(ms);
    setText(text);
    open();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlmethod void QtQuick.Controls::ToolTip::hide()

    This method hides the tooltip.
*/
void QQuickToolTip::hide()
{
    close();
}

QFont QQuickToolTip::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ToolTip);
}

void QQuickToolTip::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    Q_D(QQuickToolTip);
    QQuickPopup::itemChange(change, data);
    if (change == QQuickItem::ItemVisibleHasChanged) {
        if (!data.boolValue)
            d->stopTimeout();

        QQuickToolTipAttached *attached = qobject_cast<QQuickToolTipAttached *>(qmlAttachedPropertiesObject<QQuickToolTip>(d->parentItem, false));
        if (attached)
            emit attached->visibleChanged();
    }
}

void QQuickToolTip::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickToolTip);
    if (event->timerId() == d->timeoutTimer.timerId()) {
        d->stopTimeout();
        QQuickPopup::setVisible(false);
        return;
    }
    if (event->timerId() == d->delayTimer.timerId()) {
        d->stopDelay();
        QQuickPopup::setVisible(true);
        return;
    }
    QQuickPopup::timerEvent(event);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickToolTip::accessibleRole() const
{
    return QAccessible::ToolTip;
}

void QQuickToolTip::accessibilityActiveChanged(bool active)
{
    Q_D(QQuickToolTip);
    QQuickPopup::accessibilityActiveChanged(active);

    if (active)
        maybeSetAccessibleName(d->text);
}
#endif

QQuickToolTip *QQuickToolTipAttachedPrivate::instance(bool create) const
{
    QQmlEngine *engine = qmlEngine(parent);
    if (!engine)
        return nullptr;

    // QQuickAttachedPropertyPropagator uses "_q_QQuickToolTip", so we add "shared_"
    // to make this unique.
    static const char *name = "_q_shared_QQuickToolTip";

    QQuickToolTip *tip = engine->property(name).value<QQuickToolTip *>();
    if (!tip && create) {
        QQmlComponent component(engine, "QtQuick.Controls", "ToolTip");

        QObject *object = component.create();
        if (object)
            object->setParent(engine);

        tip = qobject_cast<QQuickToolTip *>(object);
        if (!tip)
            delete object;
        else
            engine->setProperty(name, QVariant::fromValue(object));
    }
    return tip;
}

void QQuickToolTipAttachedPrivate::maybeSetVisibleImplicitly(
    const QObject *attachee, bool visible)
{
    auto *toolTipAttached = qobject_cast<QQuickToolTipAttached *>(
        qmlAttachedPropertiesObject<QQuickToolTip>(attachee, false));
    // Not using an attached tool tip.
    if (!toolTipAttached)
        return;

    auto *toolTipAttachedPrivate = toolTipAttached->d_func();
    // Don't interfere if the user has set ToolTip.visible explicitly or if they've
    // set a manual policy.
    if (toolTipAttachedPrivate->isVisibleExplicitlySet()
            || toolTipAttachedPrivate->policy == QQuickToolTip::Manual) {
        return;
    }

    // Don't show ourselves if we have no text. We save calling code having to get our text and
    // instead just test it here.
    const bool effectiveVisible = visible ? visible && !toolTipAttachedPrivate->text.isEmpty() : false;
    toolTipAttachedPrivate->setVisible(effectiveVisible, QQml::PropertyUtils::State::ImplicitlySet);
}

void QQuickToolTipAttachedPrivate::setVisible(bool visible, QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickToolTipAttached);
    if (warnIfAttacheeIsNotAnItem(QStringLiteral("setVisible")))
        return;

    explicitVisible = isExplicitlySet(propertyState);

    if (!complete && visible) {
        pendingShow = true;
        return;
    }

    if (visible)
        q->show(text);
    else
        q->hide();
}

bool QQuickToolTipAttachedPrivate::isVisibleExplicitlySet() const
{
    return explicitVisible;
}

void QQuickToolTipAttachedPrivate::setDelay(int delay, QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickToolTipAttached);
    if (warnIfAttacheeIsNotAnItem(QStringLiteral("setDelay")))
        return;

    explicitDelay = isExplicitlySet(propertyState);

    if (this->delay == delay)
        return;

    this->delay = delay;
    emit q->delayChanged();

    if (q->isVisible())
        instance(true)->setDelay(delay);
}

bool QQuickToolTipAttachedPrivate::isDelayExplicitlySet() const
{
    return explicitDelay;
}

void QQuickToolTipAttachedPrivate::setTimeout(int timeout, QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickToolTipAttached);
    if (warnIfAttacheeIsNotAnItem(QStringLiteral("setTimeout")))
        return;

    explicitTimeout = isExplicitlySet(propertyState);

    if (this->timeout == timeout)
        return;

    this->timeout = timeout;
    emit q->timeoutChanged();

    if (q->isVisible())
        instance(true)->setTimeout(timeout);
}

bool QQuickToolTipAttachedPrivate::isTimeoutExplicitlySet() const
{
    return explicitTimeout;
}

void QQuickToolTipAttachedPrivate::inheritPolicy(QQuickToolTip::Policy policy)
{
    Q_Q(QQuickToolTipAttached);
    if (this->policy == policy)
        return;

    this->policy = policy;
    propagatePolicy();
    emit q->policyChanged();
}

void QQuickToolTipAttachedPrivate::propagatePolicy()
{
    Q_Q(QQuickToolTipAttached);
    const auto attachedToolTipChildren = q->attachedChildren();
    for (QtPrivate::QQuickAttachedPropertyPropagator *child : attachedToolTipChildren) {
        auto *attachedToolTipChild = qobject_cast<QQuickToolTipAttached *>(child);
        if (attachedToolTipChild)
            attachedToolTipChild->d_func()->inheritPolicy(policy);
    }
}

/*!
    \internal

    We used to warn that the ToolTip attached property must be attached to an object deriving
    from Item. That made sense before the introduction of ToolTip.policy, but now we need to
    be able to set a policy on e.g. ApplicationWindow and have it propagate down to the rest
    of the scene. So instead of warning in the constructor, we warn in the individual functions.
*/
bool QQuickToolTipAttachedPrivate::warnIfAttacheeIsNotAnItem(const QString &functionName)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (Q_LIKELY(item))
        return false;

    qmlWarning(parent).nospace().noquote() << "The attached function ToolTip::" << functionName
        << " can only be called when the attachee derives from Item";
    return true;
}

int QQuickToolTipAttachedPrivate::calculateTimeout(const QString &text)
{
    if (Q_UNLIKELY(qt_quicktooltipattachedprivate_short_timeout)) {
        // For auto tests, to ensure that the default automatic timeout works.
        return 123;
    }

    // Based on QTipLabel::restartExpireTimer.
    return 10000 + 40 * qMax(0, text.length() - 100);
}

QQuickToolTipAttached::QQuickToolTipAttached(QObject *parent)
    : QtPrivate::QQuickAttachedPropertyPropagator(*(new QQuickToolTipAttachedPrivate), parent)
{
    initialize();
}

/*!
    \qmlattachedproperty string QtQuick.Controls::ToolTip::text

    This attached property holds the text of the shared tool tip.
    The property can be attached to any item.

    \sa {Attached Tool Tips}
*/
QString QQuickToolTipAttached::text() const
{
    Q_D(const QQuickToolTipAttached);
    return d->text;
}

void QQuickToolTipAttached::setText(const QString &text)
{
    Q_D(QQuickToolTipAttached);
    if (d->warnIfAttacheeIsNotAnItem(QStringLiteral("setText")))
        return;
    if (d->text == text)
        return;

    d->text = text;
    emit textChanged();

    if (isVisible())
        d->instance(true)->setText(text);
}

/*!
    \qmlattachedproperty int QtQuick.Controls::ToolTip::delay

    This attached property holds the delay (milliseconds) of the shared tool tip.
    The property can be attached to any item.

    The default value is \c 0 if \l policy is \c ToolTip.Manual, otherwise it
    is \l QStyleHints::toolTipWakeUpDelay().

    \sa {Attached Tool Tips}, {Delay and Timeout}
*/
int QQuickToolTipAttached::delay() const
{
    Q_D(const QQuickToolTipAttached);
    return d->explicitDelay || d->policy == QQuickToolTip::Manual ? d->delay
        : qt_quicktooltipattachedprivate_delay;
}

void QQuickToolTipAttached::setDelay(int delay)
{
    Q_D(QQuickToolTipAttached);
    d->setDelay(delay, QQml::PropertyUtils::State::ExplicitlySet);
}

/*!
    \qmlattachedproperty int QtQuick.Controls::ToolTip::timeout

    This attached property holds the timeout (milliseconds) of the shared tool tip.
    The property can be attached to any item.

    The default value is based on the length of the text, and will always be
    at least 10 seconds long.

    \sa {Attached Tool Tips}, {Delay and Timeout}
*/
int QQuickToolTipAttached::timeout() const
{
    Q_D(const QQuickToolTipAttached);
    return d->explicitTimeout || d->policy == QQuickToolTip::Manual ? d->timeout
        : d->calculateTimeout(d->text);
}

void QQuickToolTipAttached::setTimeout(int timeout)
{
    Q_D(QQuickToolTipAttached);
    d->setTimeout(timeout, QQml::PropertyUtils::State::ExplicitlySet);
}

/*!
    \qmlattachedproperty bool QtQuick.Controls::ToolTip::visible

    This attached property holds whether the shared tool tip is visible.
    The property can be attached to any item.

    \sa {Attached Tool Tips}
*/
bool QQuickToolTipAttached::isVisible() const
{
    Q_D(const QQuickToolTipAttached);
    QQuickToolTip *tip = d->instance(false);
    if (!tip)
        return false;

    return tip->isVisible() && tip->parentItem() == parent();
}

void QQuickToolTipAttached::setVisible(bool visible)
{
    Q_D(QQuickToolTipAttached);
    d->setVisible(visible, QQml::PropertyUtils::State::ExplicitlySet);
}

/*!
    \qmlattachedproperty ToolTip QtQuick.Controls::ToolTip::toolTip

    This attached property holds the shared tool tip instance. The property
    can be attached to any item.

    \sa {Attached Tool Tips}
*/
QQuickToolTip *QQuickToolTipAttached::toolTip() const
{
    Q_D(const QQuickToolTipAttached);
    return d->instance(true);
}

/*!
    \qmlattachedproperty enumeration QtQuick.Controls::ToolTip::policy
    \since 6.12

    This attached property controls whether the visibility of the
    \l {toolTip}{shared tool tip instance} is handled automatically.
    It only has an effect for items on which \c ToolTip.visible \e {has not}
    been set. Only items that set \c ToolTip.text will be made visible. It only
    has an effect for the following items and their derived types: \l Control,
    \l TextArea and \l TextField. All other types require \l visible to be
    manually set.

    It also determines the default values for the delay and timeout properties
    of the shared tool tip instance (regardless of whether \c ToolTip.visible
    or \c ToolTip.text have been set).

    This property is propagated to attached ToolTip children.

    Available values:
    \value ToolTip.Automatic The shared tool tip will be shown when the
        attachee is hovered or long-pressed (if triggered by touch and the
        attachee is an \l AbstractButton or one of its derived types). If
        the visible property has been explicitly set, or the text property has
        not been set, this value has no effect, and the behavior will be
        equivalent to \c ToolTip.Manual.
        The shared tool tip will also default to platform-specific values
        for its delay and timeout properties.
    \value ToolTip.Manual The shared tool tip will not be shown automatically,
        and the developer is responsible for setting the visible property.
        The shared tool tip will not default to platform-specific values
        for its delay and timeout properties.

    The default value is \c {ToolTip.Automatic}.

    The property provides a way to opt-out of the default values for shared
    tool tips introduced in Qt 6.12. This is particularly relevant for legacy
    code that doesn't explicitly set the visible property before an item is
    interacted with. For applications that declaratively set the visible
    property and find the new default values for delay and timeout acceptable,
    \c policy is not needed.

    \sa {Attached Tool Tips}
*/
QQuickToolTip::Policy QQuickToolTipAttached::policy() const
{
    Q_D(const QQuickToolTipAttached);
    return d->policy;
}

void QQuickToolTipAttached::setVisiblePolicy(QQuickToolTip::Policy policy)
{
    Q_D(QQuickToolTipAttached);
    if (d->policy == policy)
        return;

    d->policy = policy;
    d->propagatePolicy();
    emit policyChanged();
}

void QQuickToolTipAttached::resetVisiblePolicy()
{
    setVisiblePolicy(QQuickToolTip::Automatic);
}

/*!
    \qmlattachedmethod void QtQuick.Controls::ToolTip::show(string text, int timeout = -1)

    This attached method shows the shared tooltip with \a text and \a timeout (milliseconds).
    The method can be attached to any item.

    \sa {Attached Tool Tips}
*/
void QQuickToolTipAttached::show(const QString &text, int ms)
{
    Q_D(QQuickToolTipAttached);
    if (d->warnIfAttacheeIsNotAnItem(QStringLiteral("show")))
        return;

    QQuickToolTip *tip = d->instance(true);
    if (!tip)
        return;

    tip->resetWidth();
    tip->resetHeight();
    tip->setParentItem(qobject_cast<QQuickItem *>(parent()));
    tip->setDelay(delay());
    tip->setTimeout(ms >= 0 ? ms : timeout());
    tip->show(text);
}

/*!
    \qmlattachedmethod void QtQuick.Controls::ToolTip::hide()

    This attached method hides the shared tooltip. The method can be attached to any item.

    \sa {Attached Tool Tips}
*/
void QQuickToolTipAttached::hide()
{
    Q_D(QQuickToolTipAttached);
    QQuickToolTip *tip = d->instance(false);
    if (!tip)
        return;
    // check the parent item to prevent unexpectedly closing tooltip by new created invisible tooltip
    if (parent() == tip->parentItem())
        tip->close();
}

void QQuickToolTipAttached::attachedParentChange(QQuickAttachedPropertyPropagator *newParent,
    QQuickAttachedPropertyPropagator */*oldParent*/)
{
    auto *attachedToolTipParent = qobject_cast<QQuickToolTipAttached *>(newParent);
    if (!attachedToolTipParent)
        return;

    Q_D(QQuickToolTipAttached);
    d->inheritPolicy(attachedToolTipParent->policy());
}

void QQuickToolTipAttached::classBegin()
{
    Q_D(QQuickToolTipAttached);
    d->complete = false;
}

void QQuickToolTipAttached::componentComplete()
{
    Q_D(QQuickToolTipAttached);
    d->complete = true;

    if (d->pendingShow) {
        d->pendingShow = false;
        show(d->text);
    }
}

QT_END_NAMESPACE

#include "moc_qquicktooltip_p.cpp"
