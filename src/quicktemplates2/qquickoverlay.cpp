/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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
#include "qquickpopup_p_p.h"
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

    void popupAboutToShow();
    void popupAboutToHide();
    void drawerPositionChange();
    void resizeOverlay(QQuickItem *overlay);
    void restackOverlay();

    QQuickItem *modal;
    QQuickItem *modeless;
    QVector<QQuickDrawer *> drawers;
    QVector<QQuickPopup *> popups;
    QPointer<QQuickPopup> mouseGrabberPopup;
    int modalPopups;
};

void QQuickOverlayPrivate::popupAboutToShow()
{
    Q_Q(QQuickOverlay);
    QQuickPopup *popup = qobject_cast<QQuickPopup *>(q->sender());
    if (!popup)
        return;

    // use QQmlProperty instead of QQuickItem::setOpacity() to trigger QML Behaviors
    if (popup->isModal()) {
        if (modal)
            QQmlProperty::write(modal, QStringLiteral("opacity"), 1.0);
    } else if (popup->dim()) {
        if (modeless)
            QQmlProperty::write(modeless, QStringLiteral("opacity"), 1.0);
    }
}

void QQuickOverlayPrivate::popupAboutToHide()
{
    Q_Q(QQuickOverlay);
    QQuickPopup *popup = qobject_cast<QQuickPopup *>(q->sender());
    if (!popup)
        return;

    // use QQmlProperty instead of QQuickItem::setOpacity() to trigger QML Behaviors
    if (popup->isModal()) {
        if (modal && modalPopups <= 1)
            QQmlProperty::write(modal, QStringLiteral("opacity"), 0.0);
    } else if (popup->dim()) {
        if (modeless)
            QQmlProperty::write(modeless, QStringLiteral("opacity"), 0.0);
    }
}

void QQuickOverlayPrivate::drawerPositionChange()
{
    Q_Q(QQuickOverlay);
    QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(q->sender());
    if (!drawer)
        return;

    // call QQuickItem::setOpacity() directly to avoid triggering QML Behaviors
    // which would make the fading feel laggy compared to the drawer movement
    if (drawer->isModal()) {
        if (modal)
            modal->setOpacity(drawer->position());
    } else if (drawer->dim()) {
        if (modeless)
            modeless->setOpacity(drawer->position());
    }
}

void QQuickOverlayPrivate::resizeOverlay(QQuickItem *overlay)
{
    Q_Q(QQuickOverlay);
    overlay->setWidth(q->width());
    overlay->setHeight(q->height());
}

void QQuickOverlayPrivate::restackOverlay()
{
    if (!modal && !modeless)
        return;

    // find the bottom-most modal and top-most modeless dimming popups
    QQuickPopup *modalPopup = nullptr;
    QQuickPopup *modelessPopup = nullptr;
    for (auto it = popups.crbegin(), end = popups.crend(); it != end; ++it) {
        QQuickPopup *popup = (*it);
        if (popup->isModal()) {
            if (!modalPopup || modalPopup->z() >= popup->z())
                modalPopup = popup;
        } else if (popup->dim()) {
            if (!modelessPopup)
                modelessPopup = popup;
        }
    }

    if (modal) {
        modal->setZ(modalPopup ? modalPopup->z() : 0.0);
        if (modalPopup)
            modal->stackBefore(modalPopup->popupItem());
    }
    if (modeless) {
        modeless->setZ(modelessPopup ? modelessPopup->z() : 0.0);
        if (modelessPopup)
            modeless->stackBefore(modelessPopup->popupItem());
    }
}

QQuickOverlayPrivate::QQuickOverlayPrivate() :
    modal(nullptr),
    modeless(nullptr),
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

QQuickItem *QQuickOverlay::modal() const
{
    Q_D(const QQuickOverlay);
    return d->modal;
}

void QQuickOverlay::setModal(QQuickItem *modal)
{
    Q_D(QQuickOverlay);
    if (d->modal == modal)
        return;

    delete d->modal;
    d->modal = modal;
    if (modal) {
        modal->setOpacity(0.0);
        modal->setParentItem(this);
        if (isComponentComplete())
            d->resizeOverlay(modal);
    }
    emit modalChanged();
}

QQuickItem *QQuickOverlay::modeless() const
{
    Q_D(const QQuickOverlay);
    return d->modeless;
}

void QQuickOverlay::setModeless(QQuickItem *modeless)
{
    Q_D(QQuickOverlay);
    if (d->modeless == modeless)
        return;

    delete d->modeless;
    d->modeless = modeless;
    if (modeless) {
        modeless->setOpacity(0.0);
        modeless->setParentItem(this);
        if (isComponentComplete())
            d->resizeOverlay(modeless);
    }
    emit modelessChanged();
}

void QQuickOverlay::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickOverlay);
    QQuickItem::itemChange(change, data);

    QQuickPopup *popup = nullptr;
    if (change == ItemChildAddedChange || change == ItemChildRemovedChange) {
        popup = qobject_cast<QQuickPopup *>(data.item->parent());
        setVisible(!childItems().isEmpty());
    }
    if (!popup)
        return;

    if (change == ItemChildAddedChange) {
        d->popups.append(popup);
        d->restackOverlay();

        QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(popup);
        if (drawer) {
            QObjectPrivate::connect(drawer, &QQuickDrawer::positionChanged, d, &QQuickOverlayPrivate::drawerPositionChange);
            d->drawers.append(drawer);
        } else {
            if (popup->isModal())
                ++d->modalPopups;

            QObjectPrivate::connect(popup, &QQuickPopup::aboutToShow, d, &QQuickOverlayPrivate::popupAboutToShow);
            QObjectPrivate::connect(popup, &QQuickPopup::aboutToHide, d, &QQuickOverlayPrivate::popupAboutToHide);
        }
    } else if (change == ItemChildRemovedChange) {
        d->popups.removeOne(popup);
        d->restackOverlay();

        QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(popup);
        if (drawer) {
            QObjectPrivate::disconnect(drawer, &QQuickDrawer::positionChanged, d, &QQuickOverlayPrivate::drawerPositionChange);
            d->drawers.removeOne(drawer);
        } else {
            if (popup->isModal())
                --d->modalPopups;

            QObjectPrivate::disconnect(popup, &QQuickPopup::aboutToShow, d, &QQuickOverlayPrivate::popupAboutToShow);
            QObjectPrivate::disconnect(popup, &QQuickPopup::aboutToHide, d, &QQuickOverlayPrivate::popupAboutToHide);
        }
    }
}

void QQuickOverlay::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickOverlay);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    if (d->modal)
        d->resizeOverlay(d->modal);
    if (d->modeless)
        d->resizeOverlay(d->modeless);
}

bool QQuickOverlay::event(QEvent *event)
{
    Q_D(QQuickOverlay);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        emit pressed();
        for (auto it = d->popups.crbegin(), end = d->popups.crend(); it != end; ++it) {
            if ((*it)->overlayEvent(this, event)) {
                d->mouseGrabberPopup = *it;
                return true;
            }
        }
        break;
    case QEvent::MouseMove:
        if (d->mouseGrabberPopup) {
            if (d->mouseGrabberPopup->overlayEvent(this, event))
                return true;
        } else {
            for (auto it = d->popups.crbegin(), end = d->popups.crend(); it != end; ++it) {
                if ((*it)->overlayEvent(this, event))
                    return true;
            }
        }
        break;
    case QEvent::MouseButtonRelease:
        emit released();
        if (d->mouseGrabberPopup) {
            QQuickPopup *grabber = d->mouseGrabberPopup;
            d->mouseGrabberPopup = nullptr;
            if (grabber->overlayEvent(this, event))
                return true;
        } else {
            for (auto it = d->popups.crbegin(), end = d->popups.crend(); it != end; ++it) {
                if ((*it)->overlayEvent(this, event))
                    return true;
            }
        }
        break;
    default:
        break;
    }

    return QQuickItem::event(event);
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

    const QList<QQuickItem *> sortedChildren = d->paintOrderChildItems();
    for (auto it = sortedChildren.rbegin(), end = sortedChildren.rend(); it != end; ++it) {
        QQuickItem *popupItem = *it;
        if (popupItem == item)
            break;

        QQuickPopup *popup = qobject_cast<QQuickPopup *>(popupItem->parent());
        if (popup && popup->overlayEvent(item, event))
            return true;
    }

    return false;
}

QT_END_NAMESPACE
