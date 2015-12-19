/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include "qquickoverlay_p.h"
#include "qquickpopup_p.h"
#include "qquickdrawer_p.h"
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

class QQuickOverlayPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickOverlay)

public:
    QQuickOverlayPrivate();

    void updateBackground();
    void resizeBackground();

    QQuickItem *background;
    QVector<QQuickDrawer *> drawers;
    QHash<QQuickItem *, QQuickPopup *> popups;
    int modalPopups;
};

void QQuickOverlayPrivate::updateBackground()
{
    if (!background)
        return;

    bool anim = true;
    qreal level = 0.0;
    if (modalPopups > 0) {
        level = 1.0;
    } else {
        foreach (QQuickDrawer *drawer, drawers) {
            qreal pos = drawer->position();
            if (pos > 0.0 && pos < 1.0)
                anim = false;
            level = qMax(level, pos);
        }
    }

    if (anim) {
        // use QQmlProperty instead of QQuickItem::setOpacity() to trigger QML Behaviors
        QQmlProperty::write(background, QStringLiteral("opacity"), level);
    } else {
        // except while a drawer is opening/closing, or else the background
        // fading feels laggy compared to the drawer movement
        background->setOpacity(level);
    }
}

void QQuickOverlayPrivate::resizeBackground()
{
    Q_Q(QQuickOverlay);
    background->setWidth(q->width());
    background->setHeight(q->height());
}

QQuickOverlayPrivate::QQuickOverlayPrivate() :
    background(Q_NULLPTR),
    modalPopups(0)
{
}

QQuickOverlay::QQuickOverlay(QQuickItem *parent)
    : QQuickItem(*(new QQuickOverlayPrivate), parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setFiltersChildMouseEvents(true);
    setVisible(false);
}


QQuickItem *QQuickOverlay::background() const
{
    Q_D(const QQuickOverlay);
    return d->background;
}

void QQuickOverlay::setBackground(QQuickItem *background)
{
    Q_D(QQuickOverlay);
    if (d->background != background) {
        delete d->background;
        d->background = background;
        if (background) {
            background->setOpacity(0.0);
            background->setParentItem(this);
            if (qFuzzyIsNull(background->z()))
                background->setZ(-1);
            if (isComponentComplete())
                d->resizeBackground();
        }
        emit backgroundChanged();
    }
}

void QQuickOverlay::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickOverlay);
    QQuickItem::itemChange(change, data);

    QQuickPopup *popup = Q_NULLPTR;
    if (change == ItemChildAddedChange || change == ItemChildRemovedChange) {
        QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(data.item);
        if (drawer) {
            if (change == ItemChildAddedChange) {
                QObjectPrivate::connect(drawer, &QQuickDrawer::positionChanged, d, &QQuickOverlayPrivate::updateBackground);
                d->drawers.append(drawer);
            } else {
                QObjectPrivate::disconnect(drawer, &QQuickDrawer::positionChanged, d, &QQuickOverlayPrivate::updateBackground);
                d->drawers.removeOne(drawer);
            }
            d->updateBackground();
        } else {
            popup = qobject_cast<QQuickPopup *>(data.item->parent());
        }
        setVisible(!childItems().isEmpty());
    }
    if (!popup)
        return;

    if (change == ItemChildAddedChange) {
        if (QQuickPopup *prevPopup = d->popups.value(data.item)) {
            qmlInfo(popup).nospace() << "Popup is sharing item " << data.item << " with " << prevPopup
                                     << ". This is not supported and strange things are about to happen.";
            return;
        }

        d->popups.insert(data.item, popup);
        if (popup->isModal())
            ++d->modalPopups;

        connect(this, &QQuickOverlay::pressed, popup, &QQuickPopup::pressedOutside);
        connect(this, &QQuickOverlay::released, popup, &QQuickPopup::releasedOutside);
    } else if (change == ItemChildRemovedChange) {
        Q_ASSERT(popup == d->popups.value(data.item));

        disconnect(this, &QQuickOverlay::pressed, popup, &QQuickPopup::pressedOutside);
        disconnect(this, &QQuickOverlay::released, popup, &QQuickPopup::releasedOutside);

        if (popup->isModal())
            --d->modalPopups;
        d->popups.remove(data.item);
    }

    d->updateBackground();
}

void QQuickOverlay::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickOverlay);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    if (d->background)
        d->resizeBackground();
}

void QQuickOverlay::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickOverlay);
    event->setAccepted(d->modalPopups > 0);
}

void QQuickOverlay::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickOverlay);
    event->setAccepted(d->modalPopups > 0);
}

void QQuickOverlay::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickOverlay);
    event->setAccepted(d->modalPopups > 0);
    emit pressed();
}

void QQuickOverlay::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickOverlay);
    event->setAccepted(d->modalPopups > 0);
}

void QQuickOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickOverlay);
    event->setAccepted(d->modalPopups > 0);
    emit released();
}

bool QQuickOverlay::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    Q_D(QQuickOverlay);
    if (d->modalPopups == 0)
        return false;
    // TODO Filter touch events
    if (event->type() != QEvent::MouseButtonPress)
        return false;
    while (item->parentItem() != this)
        item = item->parentItem();

    bool modalBlocked = false;
    const QQuickItemPrivate *priv = QQuickItemPrivate::get(this);
    const QList<QQuickItem *> &sortedChildren = priv->paintOrderChildItems();
    for (int i = sortedChildren.count() - 1; i >= 0; --i) {
        QQuickItem *contentItem = sortedChildren[i];
        if (contentItem == item)
            break;

        QQuickPopup *popup = d->popups.value(contentItem);
        if (popup) {
            emit popup->pressedOutside();

            if (!modalBlocked && popup->isModal())
                modalBlocked = true;
        }
    }

    return modalBlocked;
}

QT_END_NAMESPACE
