// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qquicklayoutitemproxy_p.h"
#include "qquicklayout_p.h"

/*!
    \qmltype LayoutItemProxy
    \nativetype QQuickLayoutItemProxy
    \inherits Item
    \inqmlmodule QtQuick.Layouts
    \ingroup layouts
    \since QtQuick.Layouts 6.6
    \brief The LayoutItemProxy class provides a placeholder for \l{QQuickItem}s
    in layouts.

    Some responsive layouts require different layout hierarchies for different
    screen sizes, but the layout hierarchy is the same as the QML structure and
    can therefore not be changed at runtime. LayoutItemProxy overcomes this
    limitation by representing a \l{target} item within the layout. The
    \l{target} item itself can be defined anywhere in the QML hierarchy. This
    allows declaration of multiple layouts with the same content items. The
    layouts can be shown and hidden to switch between them.

    The LayoutItemProxy will try to take control of the \l{target} item if it
    is \l [QML] {Item::}{visible}. Taking control will position and resize the
    \l{target} item to match the position and size of the LayoutItemProxy.
    Further, the LayoutItemProxy will set itself as the parent of the
    \l{target} (to ensure event delivery and useful drawing order) and set the
    visibility to \c true. Multiple LayoutItemProxies can \l{target} the same
    item, but only one LayoutItemProxy can control an item at a time. Therefore
    only one of the proxies targeting the same item should be visible at a
    time. If multiple proxies target the same item but \e visible is set to
    false for each proxy, the item will also be invisible.

    All \l{Layout} attached properties of the \l {target}, as well as the
    \l{QQuickItem::implicitWidth} and \l{QQuickItem::implicitHeight} of the
    \l{target} are forwarded by the LayoutItemProxy. The LayoutItemProxy will
    mimic the \l{target} as closely as possible in terms of \l{Layout}
    properties and size. \l{Layout} attached properties can also be set
    explicitly on the LayoutItemProxy which will stop the forwarding of the
    \l {target} properties.

    \section1 Example Usage

    This is a minimalistic example, changing between two layouts using proxies
    to use the same items in both layouts. The items that populate the layouts
    can be defined at an arbitrary point in the QML structure.

    \snippet layouts/simpleProxy.qml item definition

    Then we can define the Layouts with LayoutItemProxys

    \snippet layouts/simpleProxy.qml layout definition

    We can switch now between the layouts, depending on a criterion of our
    choice by toggling the visibility of the layouts on and off.

    \snippet layouts/simpleProxy.qml layout choice

    The two resulting layouts look like this:

    \div {class="float-right"}
    \inlineimage simpleProxy.png
    \enddiv

    The LayoutItemProxy can also be used without layouts, e.g. by anchoring it
    to different items. A mix of real \l {Item}{Items} and proxy items is
    equally possible, as well as nested structures of layouts and items.

    \warning The LayoutItemProxy will set the parent of its target to itself.
    Keep this in mind when referring to the parent of the target item.

    \sa Item, GridLayout, RowLayout, ColumnLayout
*/

Q_LOGGING_CATEGORY(lcLayouts, "qt.quick.layouts")


QQuickLayoutItemProxy::QQuickLayoutItemProxy(QQuickItem *parent)
    : QQuickItem(*new QQuickLayoutItemProxyPrivate, parent)
{

}

QQuickLayoutItemProxy::~QQuickLayoutItemProxy()
{
    Q_D(QQuickLayoutItemProxy);

    if (!d->target)
        return;

    QQuickLayoutItemProxyAttachedData * attachedData = d->target->property("QQuickLayoutItemProxyAttachedData").value<QQuickLayoutItemProxyAttachedData*>();
    // De-register this proxy from the proxies controlling the target
    if (attachedData) {
        if (attachedData->getControllingProxy() == this) {
            attachedData->releaseControl(this);
            d->target->setParentItem(nullptr);
        }
        attachedData->releaseProxy(this);
    }
    // The target item still has a QObject parent that takes care of its destrctuion.
    // No need to invoke destruction of the target tiem from here.
}

/*! \internal
    \brief QQuickLayoutItemProxy::geometryChange Reimplementation of
    QQuickItem::geometryChange to update the target geometry too.
*/
void QQuickLayoutItemProxy::geometryChange(const QRectF &newGeom, const QRectF &oldGeom)
{
    QQuickItem::geometryChange(newGeom, oldGeom);
    if (!isVisible())
        return;

    const QSizeF sz = newGeom.size();
    QPointF pos(0., 0.);

    if (QQuickItem *t = effectiveTarget()) {
        if (QQuickLayoutItemProxyAttachedData * attachedData = target()->property("QQuickLayoutItemProxyAttachedData").value<QQuickLayoutItemProxyAttachedData*>()) {
            if (attachedData->getControllingProxy() != this)
                return;
        }

        // Should normally not be the case, except the user resets the parent
        // This is a failsave for this case and positions the item correctly
        if (t->parentItem() != this)
            pos = t->parentItem()->mapFromGlobal(mapToGlobal(0, 0));

        if (t->size() == sz && t->position() == pos && newGeom == oldGeom)
            return;

        t->setSize(sz);
        t->setPosition(pos);
    }
}

/*! \internal
    \brief QQuickLayoutItemProxy::itemChange is a reimplementation of
    QQuickItem::itemChange to react to changes in visibility.
*/
void QQuickLayoutItemProxy::itemChange(ItemChange c, const ItemChangeData &d)
{
    if (c == QQuickItem::ItemVisibleHasChanged)
    {
        maybeTakeControl();
    }
    QQuickItem::itemChange(c, d);
}

// Implementation of the slots to react to changes of the Layout attached properties.
// If the target Layout propertie change, we change the proxy Layout properties accordingly
// If the proxy Layout properties have been changed externally, we want to remove this binding.
// The member variables m_expectProxy##Property##Change help us keep track about who invokes
// the change of the parameter. If it is invoked by the target we expect a proxy property
// change and will not remove the connection.
#define propertyForwarding(property, Property) \
    void QQuickLayoutItemProxy::target##Property##Changed() { \
        Q_D(QQuickLayoutItemProxy); \
        QQuickLayoutAttached *attTarget = attachedLayoutObject(target(), false); \
        QQuickLayoutAttached *attProxy = attachedLayoutObject(this, false); \
        if (!attTarget) return; \
        if (attProxy->property() == attTarget->property()) \
            return; \
        d->m_expectProxy##Property##Change = true; \
        attProxy->set##Property(attTarget->property()); \
    } \
    void QQuickLayoutItemProxy::proxy##Property##Changed() { \
        Q_D(QQuickLayoutItemProxy); \
        if (d->m_expectProxy##Property##Change) { \
            d->m_expectProxy##Property##Change = false; \
            return; \
        } \
        QQuickLayoutAttached *attTarget = attachedLayoutObject(target(), false); \
        if (!attTarget) return;  \
        disconnect(attTarget, &QQuickLayoutAttached::property##Changed, this, &QQuickLayoutItemProxy::target##Property##Changed); \
    }

propertyForwarding(minimumWidth, MinimumWidth)
propertyForwarding(minimumHeight, MinimumHeight)
propertyForwarding(preferredWidth, PreferredWidth)
propertyForwarding(preferredHeight, PreferredHeight)
propertyForwarding(maximumWidth, MaximumWidth)
propertyForwarding(maximumHeight, MaximumHeight)
propertyForwarding(fillWidth, FillWidth)
propertyForwarding(fillHeight, FillHeight)
propertyForwarding(alignment, Alignment)
propertyForwarding(horizontalStretchFactor, HorizontalStretchFactor)
propertyForwarding(verticalStretchFactor, VerticalStretchFactor)
propertyForwarding(margins, Margins)
propertyForwarding(leftMargin, LeftMargin)
propertyForwarding(topMargin, TopMargin)
propertyForwarding(rightMargin, RightMargin)
propertyForwarding(bottomMargin, BottomMargin)

#undef propertyForwarding

/*!
    \qmlproperty Item LayoutItemProxy::target

    This property holds the \l Item that the proxy should represent in a
    \l {Layout} hierarchy.
*/

/*! \internal
    \brief QQuickLayoutItemProxy::target
    \return The target item of the proxy
*/
QQuickItem *QQuickLayoutItemProxy::target() const
{
    Q_D(const QQuickLayoutItemProxy);
    return d->target;
}

/*! \internal
    \brief QQuickLayoutItemProxy::setTarget sets the target
    \param newTarget The item that the proxy stands in place for.

    All layout properties of the target are connected to the layout properties
    of the LayoutItemProxy. It the LayoutItemProxy is visible, it will try to
    take control of the target.
*/
void QQuickLayoutItemProxy::setTarget(QQuickItem *newTarget)
{
    Q_D(QQuickLayoutItemProxy);

    if (newTarget == d->target)
        return;

    if (d->target && d->target->property("QQuickLayoutItemProxyAttachedData").isValid()) {
        QQuickLayoutItemProxyAttachedData *attachedData = d->target->property("QQuickLayoutItemProxyAttachedData").value<QQuickLayoutItemProxyAttachedData*>();
        attachedData->releaseProxy(this);
    }
    d->target = newTarget;

    if (newTarget) {

        QQuickLayoutItemProxyAttachedData *attachedData;
        if (newTarget->property("QQuickLayoutItemProxyAttachedData").isValid()) {
            attachedData = newTarget->property("QQuickLayoutItemProxyAttachedData").value<QQuickLayoutItemProxyAttachedData*>();
        } else {
            attachedData = new QQuickLayoutItemProxyAttachedData(newTarget);
            QVariant v;
            v.setValue(attachedData);
            newTarget->setProperty("QQuickLayoutItemProxyAttachedData", v);
        }
        attachedData->registerProxy(this);

        // If there is no other controlling proxy, we will hide the target
        if (!attachedData->proxyHasControl())
            newTarget->setVisible(false);
        // We are calling maybeTakeControl at the end to eventually take
        // responsibility of showing the target.

        if (QQuickLayoutAttached *attTarget = attachedLayoutObject(newTarget)) {
            QQuickLayoutAttached *attProxy = attachedLayoutObject(this, true);

            disconnect(attTarget, nullptr, attProxy, nullptr);

            // bind item-specific layout properties:

#define connectPropertyForwarding(property, Property) \
            if (!attProxy->is##Property##Set()) { \
                connect(attTarget, &QQuickLayoutAttached::property##Changed, this, &QQuickLayoutItemProxy::target##Property##Changed); \
                connect(attProxy, &QQuickLayoutAttached::property##Changed, this, &QQuickLayoutItemProxy::proxy##Property##Changed); \
                target##Property##Changed(); \
            }
            connectPropertyForwarding(minimumWidth, MinimumWidth)
            connectPropertyForwarding(minimumHeight, MinimumHeight)
            connectPropertyForwarding(preferredWidth, PreferredWidth)
            connectPropertyForwarding(preferredHeight, PreferredHeight)
            connectPropertyForwarding(maximumWidth, MaximumWidth)
            connectPropertyForwarding(maximumHeight, MaximumHeight)
            connectPropertyForwarding(fillWidth, FillWidth)
            connectPropertyForwarding(fillHeight, FillHeight)
            connectPropertyForwarding(alignment, Alignment)
            connectPropertyForwarding(horizontalStretchFactor, HorizontalStretchFactor)
            connectPropertyForwarding(verticalStretchFactor, VerticalStretchFactor)
            connectPropertyForwarding(margins, Margins)
            connectPropertyForwarding(leftMargin, LeftMargin)
            connectPropertyForwarding(topMargin, TopMargin)
            connectPropertyForwarding(rightMargin, RightMargin)
            connectPropertyForwarding(bottomMargin, BottomMargin)
#undef connectPropertyForwarding

            // proxy.implicitWidth: target.implicitWidth
            auto fnBindImplW = [newTarget, this](){ this->setImplicitWidth(newTarget->implicitWidth()); };
            fnBindImplW();
            connect(newTarget, &QQuickItem::implicitWidthChanged, fnBindImplW);

            // proxy.implicitHeight: target.implicitHeight
            auto fnBindImplH = [newTarget, this](){ this->setImplicitHeight(newTarget->implicitHeight()); };
            fnBindImplH();
            connect(newTarget, &QQuickItem::implicitHeightChanged, fnBindImplH);
        }
    }

    if (isVisible())
        maybeTakeControl();

    emit targetChanged();
}

/*! \internal
    \brief QQuickLayoutItemProxy::effectiveTarget
    \return The target item of the proxy if it is in control, \c null otherwise.
*/
QQuickItem *QQuickLayoutItemProxy::effectiveTarget() const
{
    if (target() == nullptr)
        return nullptr;

    QQuickLayoutItemProxyAttachedData * attachedData = target()->property("QQuickLayoutItemProxyAttachedData").value<QQuickLayoutItemProxyAttachedData*>();
    return (attachedData->getControllingProxy() == this) ? target() : nullptr;
}

/*! \internal
    \brief QQuickLayoutItemProxy::clearTarget sets the target to null.

    This function is called if the target is destroyed to make sure we do not
    try to access a non-existing object.
*/
void QQuickLayoutItemProxy::clearTarget()
{
    setTarget(nullptr);
}

/*! \internal
    \brief QQuickLayoutItemProxy::maybeTakeControl checks and takes over control
    of the item.

    If the proxy is visible it will try to take control over the target and set
    its visibility to true. If the proxy is hidden it will also hide the target
    and another LayoutItemProxy has to set the visibility to \c true or the
    target will stay invisible.
*/
void QQuickLayoutItemProxy::maybeTakeControl()
{
    Q_D(QQuickLayoutItemProxy);
    if (!d->target)
        return;

    QQuickLayoutItemProxyAttachedData * attachedData = d->target->property("QQuickLayoutItemProxyAttachedData").value<QQuickLayoutItemProxyAttachedData*>();
    if (isVisible() && attachedData->getControllingProxy() != this) {
        if (attachedData->takeControl(this)) {
            d->target->setVisible(true);
            d->target->setParentItem(this);
            updatePos();
        }
    }
    if (!isVisible() && attachedData->getControllingProxy() == this){
        if (d->target->parentItem() == this) {
            d->target->setParentItem(nullptr);
        } else
            qCDebug(lcLayouts) << "Parent was changed to" << d->target->parentItem() << "while an ItemProxy had control";
        d->target->setVisible(false);
        attachedData->releaseControl(this);
    }
}

/*! \internal
    \brief QQuickLayoutItemProxy::updatePos sets the geometry of the target to
    the geometry of the proxy
*/
void QQuickLayoutItemProxy::updatePos()
{
    if (!isVisible())
        return;
    if (target()) {
        if (QQuickLayoutItemProxyAttachedData * attachedData = target()->property("QQuickLayoutItemProxyAttachedData").value<QQuickLayoutItemProxyAttachedData*>()) {
            if (attachedData->getControllingProxy() == this)
                geometryChange(boundingRect(), boundingRect());
        }
    }
}

QQuickLayoutItemProxyPrivate::QQuickLayoutItemProxyPrivate()
    : QQuickItemPrivate(),
      m_expectProxyMinimumWidthChange(false),
      m_expectProxyMinimumHeightChange(false),
      m_expectProxyPreferredWidthChange(false),
      m_expectProxyPreferredHeightChange(false),
      m_expectProxyMaximumWidthChange(false),
      m_expectProxyMaximumHeightChange(false),
      m_expectProxyFillWidthChange(false),
      m_expectProxyFillHeightChange(false),
      m_expectProxyAlignmentChange(false),
      m_expectProxyHorizontalStretchFactorChange(false),
      m_expectProxyVerticalStretchFactorChange(false),
      m_expectProxyMarginsChange(false),
      m_expectProxyLeftMarginChange(false),
      m_expectProxyTopMarginChange(false),
      m_expectProxyRightMarginChange(false),
      m_expectProxyBottomMarginChange(false)
{

}

/*! \internal
    \class QQuickLayoutItemProxyAttachedData
    \brief Provides attached properties for items that are managed by one or
    more LayoutItemProxy.

    It stores all proxies that target the item, and will emit signals when the
    proxies or the controlling proxy changes. Proxies can listen to the signal
    and pick up control if they wish to.
*/
QQuickLayoutItemProxyAttachedData::QQuickLayoutItemProxyAttachedData(QObject *parent)
    : QObject(parent), controllingProxy(nullptr)
{

}

QQuickLayoutItemProxyAttachedData::~QQuickLayoutItemProxyAttachedData()
{
    if (QObject *par = parent())
        par->setProperty("QQuickLayoutItemProxyAttachedData", QVariant());

    // If this is destroyed, so is the target. Clear the target from the
    // proxies so they do not try to access a destroyed object
    for (auto &proxy: std::as_const(proxies))
        proxy->clearTarget();
}

/*! \internal
    \brief QQuickLayoutItemProxyAttachedData::registerProxy registers a proxy
    that manages the item this data is attached to.

    This is required to easily notify proxies when the target is destroyed or
    when it is free to take over control.
*/
void QQuickLayoutItemProxyAttachedData::registerProxy(QQuickLayoutItemProxy *proxy)
{
    if (proxies.contains(proxy))
        return;

    proxies.append(proxy);
    emit proxiesChanged();
}

/*! \internal
    \brief QQuickLayoutItemProxyAttachedData::releaseProxy removes a proxy from
    a list of known proxies that manage the item this data is attached to.
*/
void QQuickLayoutItemProxyAttachedData::releaseProxy(QQuickLayoutItemProxy *proxy)
{
    if (proxy == controllingProxy)
        releaseControl(proxy);

    proxies.removeAll(proxy);

    if (proxies.isEmpty())
        deleteLater();

    emit proxiesChanged();
}

/*! \internal
    \brief QQuickLayoutItemProxyAttachedData::takeControl is called by
    LayoutItemProxies when they try to take control over the item this data is
    attached to.
    \return \c true if no other proxy controls the item and if control is
    granted to the proxy, \c false otherwise.

    \param proxy The proxy that tries to take control.
*/
bool QQuickLayoutItemProxyAttachedData::takeControl(QQuickLayoutItemProxy *proxy)
{
    if (controllingProxy || !proxies.contains(proxy))
        return false;

    qCDebug(lcLayouts) << proxy
                       << "takes control of"
                       << parent();

    controllingProxy = proxy;
    emit controlTaken();
    emit controllingProxyChanged();
    return true;
}

/*! \internal
    \brief QQuickLayoutItemProxyAttachedData::releaseControl is called by
    LayoutItemProxies when they try no longer control the item

    \param proxy The proxy that gives up control.
*/
void QQuickLayoutItemProxyAttachedData::releaseControl(QQuickLayoutItemProxy *proxy)
{
    if (controllingProxy != proxy)
        return;

    qCDebug(lcLayouts) << proxy
                       << "no longer controls"
                       << parent();

    controllingProxy = nullptr;
    emit controlReleased();
    emit controllingProxyChanged();

    for (auto &otherProxy: std::as_const(proxies)) {
        if (proxy != otherProxy)
            otherProxy->maybeTakeControl();
    }
}

/*! \internal
    \brief QQuickLayoutItemProxyAttachedData::getControllingProxy
    \return the proxy that currently controls the item this data is attached to.
    Returns \c null if no proxy controls the item.
*/
QQuickLayoutItemProxy *QQuickLayoutItemProxyAttachedData::getControllingProxy() const
{
    return controllingProxy;
}

/*! \internal
    \brief QQuickLayoutItemProxyAttachedData::getProxies
    \return a list of all proxies that target the item this data is attached to.
*/
QQmlListProperty<QQuickLayoutItemProxy> QQuickLayoutItemProxyAttachedData::getProxies()
{
    using Type = QQuickLayoutItemProxy;
    using Property = QQmlListProperty<Type>;

    return Property(
        this, &proxies,
        [](Property *p) { return static_cast<QList<Type *> *>(p->data)->size(); },
        [](Property *p, qsizetype i) { return static_cast<QList<Type *> *>(p->data)->at(i); }
    );
}

/*! \internal
    \brief QQuickLayoutItemProxyAttachedData::proxyHasControl
    \return \c true if a proxy is controlling the item, \c false otherwise.
*/
bool QQuickLayoutItemProxyAttachedData::proxyHasControl() const
{
    return controllingProxy != nullptr;
}
