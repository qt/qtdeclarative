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
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

class QQuickOverlayPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickOverlay)

public:
    QQuickOverlayPrivate();

    void popupAboutToShow();
    void popupAboutToHide();
    void popupClosed();

    void createOverlay(QQuickPopup *popup);
    void destroyOverlay(QQuickPopup *popup);
    void resizeOverlay(QQuickPopup *popup);

    QQmlComponent *modal;
    QQmlComponent *modeless;
    QVector<QQuickDrawer *> drawers;
    QVector<QQuickPopup *> popups;
    QPointer<QQuickPopup> mouseGrabberPopup;
    int modalPopups;
};

void QQuickOverlayPrivate::popupAboutToShow()
{
    Q_Q(QQuickOverlay);
    QQuickPopup *popup = qobject_cast<QQuickPopup *>(q->sender());
    if (!popup || !popup->dim())
        return;

    createOverlay(popup);

    // use QQmlProperty instead of QQuickItem::setOpacity() to trigger QML Behaviors
    QQuickPopupPrivate *p = QQuickPopupPrivate::get(popup);
    if (p->dimmer)
        QQmlProperty::write(p->dimmer, QStringLiteral("opacity"), 1.0);
}

void QQuickOverlayPrivate::popupAboutToHide()
{
    Q_Q(QQuickOverlay);
    QQuickPopup *popup = qobject_cast<QQuickPopup *>(q->sender());
    if (!popup || !popup->dim())
        return;

    // use QQmlProperty instead of QQuickItem::setOpacity() to trigger QML Behaviors
    QQuickPopupPrivate *p = QQuickPopupPrivate::get(popup);
    if (p->dimmer)
        QQmlProperty::write(p->dimmer, QStringLiteral("opacity"), 0.0);
}

void QQuickOverlayPrivate::popupClosed()
{
    Q_Q(QQuickOverlay);
    QQuickPopup *popup = qobject_cast<QQuickPopup *>(q->sender());
    if (!popup || !popup->dim())
        return;

    destroyOverlay(popup);
}

static QQuickItem *createDimmer(QQmlComponent *component, QQuickPopup *popup, QQuickItem *parent)
{
    if (!component)
        return nullptr;

    QQmlContext *creationContext = component->creationContext();
    if (!creationContext)
        creationContext = qmlContext(parent);
    QQmlContext *context = new QQmlContext(creationContext);
    context->setContextObject(popup);
    QQuickItem *item = qobject_cast<QQuickItem*>(component->beginCreate(context));
    if (item) {
        item->setOpacity(0.0);
        item->setParentItem(parent);
        item->stackBefore(popup->popupItem());
        item->setZ(popup->z());
        component->completeCreate();
    }
    return item;
}

void QQuickOverlayPrivate::createOverlay(QQuickPopup *popup)
{
    Q_Q(QQuickOverlay);
    QQuickPopupPrivate *p = QQuickPopupPrivate::get(popup);
    if (!p->dimmer)
        p->dimmer = createDimmer(popup->isModal() ? modal : modeless, popup, q);
    resizeOverlay(popup);
}

void QQuickOverlayPrivate::destroyOverlay(QQuickPopup *popup)
{
    QQuickPopupPrivate *p = QQuickPopupPrivate::get(popup);
    if (p->dimmer) {
        p->dimmer->deleteLater();
        p->dimmer = nullptr;
    }
}

void QQuickOverlayPrivate::resizeOverlay(QQuickPopup *popup)
{
    Q_Q(QQuickOverlay);
    QQuickPopupPrivate *p = QQuickPopupPrivate::get(popup);
    if (p->dimmer) {
        p->dimmer->setWidth(q->width());
        p->dimmer->setHeight(q->height());
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

QQmlComponent *QQuickOverlay::modal() const
{
    Q_D(const QQuickOverlay);
    return d->modal;
}

void QQuickOverlay::setModal(QQmlComponent *modal)
{
    Q_D(QQuickOverlay);
    if (d->modal == modal)
        return;

    delete d->modal;
    d->modal = modal;
    emit modalChanged();
}

QQmlComponent *QQuickOverlay::modeless() const
{
    Q_D(const QQuickOverlay);
    return d->modeless;
}

void QQuickOverlay::setModeless(QQmlComponent *modeless)
{
    Q_D(QQuickOverlay);
    if (d->modeless == modeless)
        return;

    delete d->modeless;
    d->modeless = modeless;
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

        QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(popup);
        if (drawer) {
            d->drawers.append(drawer);
            d->createOverlay(drawer);
        } else {
            if (popup->isModal())
                ++d->modalPopups;

            QObjectPrivate::connect(popup, &QQuickPopup::aboutToShow, d, &QQuickOverlayPrivate::popupAboutToShow);
            QObjectPrivate::connect(popup, &QQuickPopup::aboutToHide, d, &QQuickOverlayPrivate::popupAboutToHide);
            QObjectPrivate::connect(popup, &QQuickPopup::closed, d, &QQuickOverlayPrivate::popupClosed);
        }
    } else if (change == ItemChildRemovedChange) {
        d->popups.removeOne(popup);

        QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(popup);
        if (drawer) {
            d->drawers.removeOne(drawer);
            d->destroyOverlay(drawer);
        } else {
            if (popup->isModal())
                --d->modalPopups;

            QObjectPrivate::disconnect(popup, &QQuickPopup::aboutToShow, d, &QQuickOverlayPrivate::popupAboutToShow);
            QObjectPrivate::disconnect(popup, &QQuickPopup::aboutToHide, d, &QQuickOverlayPrivate::popupAboutToHide);
            QObjectPrivate::disconnect(popup, &QQuickPopup::closed, d, &QQuickOverlayPrivate::popupClosed);
        }
    }
}

void QQuickOverlay::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickOverlay);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    for (QQuickPopup *popup : d->popups)
        d->resizeOverlay(popup);
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
