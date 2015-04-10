/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Extras module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktumbler_p.h"

#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuickControls/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Tumbler
    \inherits Container
    \instantiates QQuickTumbler
    \inqmlmodule QtQuick.Extras
    \ingroup containers
    \brief A spinnable wheel of items that can be selected.

    TODO
*/

class QQuickTumblerPrivate : public QQuickControlPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickTumbler)

public:
    QQuickTumblerPrivate() :
        delegate(Q_NULLPTR),
        visibleItemCount(3)
    {
    }

    QVariant model;
    QQmlComponent *delegate;
    int visibleItemCount;

    void updateItemHeights();
    void updateItemWidths();

    void itemChildAdded(QQuickItem *, QQuickItem *);
    void itemChildRemoved(QQuickItem *, QQuickItem *);
};

static QList<QQuickItem *> contentItemChildItems(QQuickItem *contentItem)
{
    if (!contentItem)
        return QList<QQuickItem *>();

    // PathView has no contentItem property, but ListView does.
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(contentItem);
    return flickable ? flickable->contentItem()->childItems() : contentItem->childItems();
}

void QQuickTumblerPrivate::updateItemHeights()
{
    // TODO: can we/do we want to support spacing?
    const qreal itemHeight = (contentItem->height()/* - qMax(0, itemCount - 1) * spacing*/
        - topPadding - bottomPadding) / visibleItemCount;
    foreach (QQuickItem *childItem, contentItemChildItems(contentItem))
        childItem->setHeight(itemHeight);
}

void QQuickTumblerPrivate::updateItemWidths()
{
    foreach (QQuickItem *childItem, contentItemChildItems(contentItem))
        childItem->setWidth(width);
}

void QQuickTumblerPrivate::itemChildAdded(QQuickItem *, QQuickItem *)
{
    updateItemWidths();
    updateItemHeights();
}

void QQuickTumblerPrivate::itemChildRemoved(QQuickItem *, QQuickItem *)
{
    updateItemWidths();
    updateItemHeights();
}

QQuickTumbler::QQuickTumbler(QQuickItem *parent) :
    QQuickControl(*(new QQuickTumblerPrivate), parent)
{
}

/*!
    \qmlproperty variant QtQuickExtras2::Tumbler::model

    This property holds the model that provides data for this tumbler.
*/
QVariant QQuickTumbler::model() const
{
    Q_D(const QQuickTumbler);
    return d->model;
}

void QQuickTumbler::setModel(const QVariant &model)
{
    Q_D(QQuickTumbler);
    if (model != d->model) {
        d->model = model;
        emit modelChanged();
    }
}

/*!
    \qmlproperty int QtQuickExtras2::Tumbler::count

    This property holds the number of items in the model.
*/
int QQuickTumbler::count() const
{
    Q_D(const QQuickTumbler);
    return d->contentItem->property("count").toInt();
}

/*!
    \qmlproperty int QtQuickExtras2::Tumbler::currentIndex

    This property holds the index of the current item.
*/
int QQuickTumbler::currentIndex() const
{
    Q_D(const QQuickTumbler);
    return d->contentItem ? d->contentItem->property("currentIndex").toInt() : -1;
}

void QQuickTumbler::setCurrentIndex(int currentIndex)
{
    Q_D(QQuickTumbler);
    d->contentItem->setProperty("currentIndex", currentIndex);
}

/*!
    \qmlproperty Item QtQuickExtras2::Tumbler::currentItem

    This property holds the item at the current index.
*/
QQuickItem *QQuickTumbler::currentItem() const
{
    Q_D(const QQuickTumbler);
    return d->contentItem ? d->contentItem->property("currentItem").value<QQuickItem*>() : Q_NULLPTR;
}

/*!
    \qmlproperty component QtQuickExtras2::Tumbler::delegate

    This property holds the delegate used to display each item.
*/
QQmlComponent *QQuickTumbler::delegate() const
{
    Q_D(const QQuickTumbler);
    return d->delegate;
}

void QQuickTumbler::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickTumbler);
    if (delegate != d->delegate) {
        d->delegate = delegate;
        emit delegateChanged();
    }
}

/*!
    \qmlproperty int QtQuickExtras2::Tumbler::visibleItemCount

    This property holds the number of items visible in the tumbler. It must be
    an odd number.
*/
int QQuickTumbler::visibleItemCount() const
{
    Q_D(const QQuickTumbler);
    return d->visibleItemCount;
}

void QQuickTumbler::setVisibleItemCount(int visibleItemCount)
{
    Q_D(QQuickTumbler);
    if (visibleItemCount != d->visibleItemCount) {
        d->visibleItemCount = visibleItemCount;
        d->updateItemHeights();
        emit visibleItemCountChanged();
    }
}

QQuickTumblerAttached *QQuickTumbler::qmlAttachedProperties(QObject *object)
{
    QQuickItem *delegateItem = qobject_cast<QQuickItem *>(object);
    if (!delegateItem) {
        qWarning() << "Attached properties of Tumbler must be accessed from within a delegate item";
        return Q_NULLPTR;
    }

    return new QQuickTumblerAttached(delegateItem);
}

void QQuickTumbler::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTumbler);

    QQuickControl::geometryChanged(newGeometry, oldGeometry);

    d->updateItemHeights();

    if (newGeometry.width() != oldGeometry.width())
        d->updateItemWidths();
}

void QQuickTumbler::componentComplete()
{
    Q_D(QQuickTumbler);
    QQuickControl::componentComplete();
    d->updateItemHeights();
    d->updateItemWidths();
}

void QQuickTumbler::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickTumbler);

    QQuickControl::contentItemChange(newItem, oldItem);

    // Since we use the currentIndex of the contentItem directly, we must
    // ensure that we keep track of the currentIndex so it doesn't get lost
    // between contentItem changes.
    const int previousCurrentIndex = currentIndex();

    if (oldItem) {
        disconnect(oldItem, SIGNAL(currentIndexChanged()), this, SIGNAL(currentIndexChanged()));
        disconnect(oldItem, SIGNAL(currentItemChanged()), this, SIGNAL(currentItemChanged()));
        disconnect(oldItem, SIGNAL(countChanged()), this, SIGNAL(countChanged()));

        QQuickItemPrivate *oldItemPrivate = QQuickItemPrivate::get(oldItem);
        oldItemPrivate->removeItemChangeListener(d, QQuickItemPrivate::Children);
    }

    if (newItem) {
        connect(newItem, SIGNAL(currentIndexChanged()), this, SIGNAL(currentIndexChanged()));
        connect(newItem, SIGNAL(currentItemChanged()), this, SIGNAL(currentItemChanged()));
        connect(newItem, SIGNAL(countChanged()), this, SIGNAL(countChanged()));

        QQuickItemPrivate *newItemPrivate = QQuickItemPrivate::get(newItem);
        newItemPrivate->addItemChangeListener(d, QQuickItemPrivate::Children);

        // If the previous currentIndex is -1, it means we had no contentItem previously.
        if (previousCurrentIndex != -1) {
            // Can't call setCurrentIndex here, as contentItemChange() is
            // called *before* the contentItem is set.
            newItem->setProperty("currentIndex", previousCurrentIndex);
        }
    }
}

void QQuickTumbler::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickTumbler);

    QQuickControl::keyPressEvent(event);

    if (event->isAutoRepeat())
        return;

    if (event->key() == Qt::Key_Up) {
        QMetaObject::invokeMethod(d->contentItem, "decrementCurrentIndex");
    } else if (event->key() == Qt::Key_Down) {
        QMetaObject::invokeMethod(d->contentItem, "incrementCurrentIndex");
    }
}

class QQuickTumblerAttachedPrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickTumblerAttached)
public:
    QQuickTumblerAttachedPrivate(QQuickItem *delegateItem) :
        tumbler(Q_NULLPTR),
        index(-1),
        displacement(1)
    {
        if (!delegateItem->parentItem()) {
            qWarning() << "Attached properties of Tumbler must be accessed from within a delegate item that has a parent";
            return;
        }

        QVariant indexContextProperty = qmlContext(delegateItem)->contextProperty(QStringLiteral("index"));
        if (!indexContextProperty.isValid()) {
            qWarning() << "Attempting to access attached property on item without an \"index\" property";
            return;
        }

        index = indexContextProperty.toInt();
        if (!delegateItem->parentItem()->inherits("QQuickPathView")) {
            qWarning() << "contentItems other than PathView are not currently supported";
            return;
        }

        tumbler = qobject_cast<QQuickTumbler* >(delegateItem->parentItem()->parentItem());
    }

    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;
    void itemChildAdded(QQuickItem *, QQuickItem *);
    void itemChildRemoved(QQuickItem *, QQuickItem *);

    void _q_calculateDisplacement();

    // The Tumbler that contains the delegate. Required to calculated the displacement.
    QQuickTumbler *tumbler;
    // The index of the delegate. Used to calculate the displacement.
    int index;
    // The displacement for our delegate.
    qreal displacement;
};

void QQuickTumblerAttachedPrivate::itemGeometryChanged(QQuickItem *, const QRectF &, const QRectF &)
{
    _q_calculateDisplacement();
}

void QQuickTumblerAttachedPrivate::itemChildAdded(QQuickItem *, QQuickItem *)
{
    _q_calculateDisplacement();
}

void QQuickTumblerAttachedPrivate::itemChildRemoved(QQuickItem *, QQuickItem *child)
{
    _q_calculateDisplacement();

    if (parent == child) {
        // The child that was removed from the contentItem was the delegate
        // that our properties are attached to. If we don't remove the change
        // listener, the contentItem will attempt to notify a destroyed
        // listener, causing a crash.
        QQuickItemPrivate *p = QQuickItemPrivate::get(tumbler->contentItem());
        p->removeItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Children);
    }
}

void QQuickTumblerAttachedPrivate::_q_calculateDisplacement()
{
    const int previousDisplacement = displacement;

    displacement = 1;

    // TODO: ListView has no offset property, need to try using contentY instead.
    if (tumbler && tumbler->contentItem()->inherits("QQuickListView"))
        return;

    qreal offset = tumbler->contentItem()->property("offset").toReal();
    displacement = tumbler->count() - index - offset;
    int halfVisibleItems = tumbler->visibleItemCount() / 2 + 1;
    if (displacement > halfVisibleItems)
        displacement -= tumbler->count();
    else if (displacement < -halfVisibleItems)
        displacement += tumbler->count();

    Q_Q(QQuickTumblerAttached);
    if (displacement != previousDisplacement)
        emit q->displacementChanged();
}

QQuickTumblerAttached::QQuickTumblerAttached(QQuickItem *delegateItem) :
    QObject(*(new QQuickTumblerAttachedPrivate(delegateItem)), delegateItem)
{
    Q_D(QQuickTumblerAttached);
    if (d->tumbler) {
        // TODO: in case of listview, listen to contentItem of contentItem
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->tumbler->contentItem());
        p->addItemChangeListener(d, QQuickItemPrivate::Geometry | QQuickItemPrivate::Children);

        // TODO: in case of ListView, listen to contentY changed
        connect(d->tumbler->contentItem(), SIGNAL(offsetChanged()), this, SLOT(_q_calculateDisplacement()));
    }
}

qreal QQuickTumblerAttached::displacement() const
{
    Q_D(const QQuickTumblerAttached);
    return d->displacement;
}

QT_END_NAMESPACE

#include "moc_qquicktumbler_p.cpp"
