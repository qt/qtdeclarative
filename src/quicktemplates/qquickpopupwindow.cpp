// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpopupwindow_p_p.h"
#include "qquickcombobox_p.h"
#include "qquickpopup_p.h"
#include "qquickpopup_p_p.h"
#include "qquickpopupitem_p_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickwindowmodule_p.h>
#include <QtQuick/private/qquickwindowmodule_p_p.h>
#include <qpa/qplatformwindow_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPopupWindow, "qt.quick.controls.popup.window")

class QQuickPopupWindowPrivate : public QQuickWindowQmlImplPrivate
{
    Q_DECLARE_PUBLIC(QQuickPopupWindow)

public:
    QPointer<QQuickItem> m_popupItem;
    QPointer<QQuickPopup> m_popup;
};

QQuickPopupWindow::QQuickPopupWindow(QQuickPopup *popup, QWindow *parent)
    : QQuickWindowQmlImpl(*(new QQuickPopupWindowPrivate), nullptr)
{
    Q_D(QQuickPopupWindow);

    d->m_popup = popup;
    d->m_popupItem = popup->popupItem();
    setTransientParent(parent);

    connect(d->m_popup, &QQuickPopup::windowChanged, this, &QQuickPopupWindow::windowChanged);
    connect(d->m_popup, &QQuickPopup::implicitWidthChanged, this, &QQuickPopupWindow::implicitWidthChanged);
    connect(d->m_popup, &QQuickPopup::implicitHeightChanged, this, &QQuickPopupWindow::implicitHeightChanged);
    connect(d->m_popup->window(), &QWindow::xChanged, this, &QQuickPopupWindow::parentWindowXChanged);
    connect(d->m_popup->window(), &QWindow::yChanged, this, &QQuickPopupWindow::parentWindowYChanged);

    setWidth(d->m_popupItem->implicitWidth());
    setHeight(d->m_popupItem->implicitHeight());

    const auto flags = QQuickPopupPrivate::get(popup)->popupWindowType();

    // For popup windows, we'll need to draw everything, in order to have enough control over the styling.
    if (flags & Qt::Popup)
        setColor(QColorConstants::Transparent);

    setFlags(flags);

    qCDebug(lcPopupWindow) << "Created popup window with flags: " << flags;
}

QQuickPopupWindow::~QQuickPopupWindow()
{
    Q_D(QQuickPopupWindow);
    disconnect(d->m_popup, &QQuickPopup::windowChanged, this, &QQuickPopupWindow::windowChanged);
    disconnect(d->m_popup, &QQuickPopup::implicitWidthChanged, this, &QQuickPopupWindow::implicitWidthChanged);
    disconnect(d->m_popup, &QQuickPopup::implicitHeightChanged, this, &QQuickPopupWindow::implicitHeightChanged);
    disconnect(d->m_popup->window(), &QWindow::xChanged, this, &QQuickPopupWindow::parentWindowXChanged);
    disconnect(d->m_popup->window(), &QWindow::yChanged, this, &QQuickPopupWindow::parentWindowYChanged);
}

QQuickPopup *QQuickPopupWindow::popup() const
{
    Q_D(const QQuickPopupWindow);
    return d->m_popup;
}

void QQuickPopupWindow::hideEvent(QHideEvent *e)
{
    Q_D(QQuickPopupWindow);
    QQuickWindow::hideEvent(e);
    if (QQuickPopup *popup = d->m_popup) {
        QQuickPopupPrivate::get(popup)->visible = false;
        emit popup->visibleChanged();
    }
}

void QQuickPopupWindow::moveEvent(QMoveEvent *e)
{
    handlePopupPositionChangeFromWindowSystem(e->pos());
}

void QQuickPopupWindow::resizeEvent(QResizeEvent *e)
{
    Q_D(QQuickPopupWindow);
    QQuickWindowQmlImpl::resizeEvent(e);

    if (!d->m_popupItem)
        return;

    qCDebug(lcPopupWindow) << "A window system event changed the popup's size to be " << e->size();
    d->m_popupItem->setWidth(e->size().width());
    d->m_popupItem->setHeight(e->size().height());
}

void QQuickPopupWindow::windowChanged(QWindow *window)
{
    if (window) {
        connect(window, &QWindow::xChanged, this, &QQuickPopupWindow::parentWindowXChanged);
        connect(window, &QWindow::yChanged, this, &QQuickPopupWindow::parentWindowYChanged);
    }
}

QPoint QQuickPopupWindow::global2Local(const QPoint &pos) const
{
    Q_D(const QQuickPopupWindow);
    QQuickPopup *popup = d->m_popup;
    Q_ASSERT(popup);
    const QPoint scenePos = popup->window()->mapFromGlobal(pos);
    // Popup's coordinates are relative to the nearest parent item.
    return popup->parentItem() ? popup->parentItem()->mapFromScene(scenePos).toPoint() : scenePos;
}

void QQuickPopupWindow::parentWindowXChanged(int newX)
{
    const auto popupLocalPos = global2Local({x(), y()});
    handlePopupPositionChangeFromWindowSystem({newX + popupLocalPos.x(), y()});
}

void QQuickPopupWindow::parentWindowYChanged(int newY)
{
    const auto popupLocalPos = global2Local({x(), y()});
    handlePopupPositionChangeFromWindowSystem({x(), newY + popupLocalPos.y()});
}

void QQuickPopupWindow::handlePopupPositionChangeFromWindowSystem(const QPoint &pos)
{
    Q_D(QQuickPopupWindow);
    QQuickPopup *popup = d->m_popup;
    if (!popup)
        return;
    QQuickPopupPrivate *popupPrivate = QQuickPopupPrivate::get(popup);

    const auto localPos = global2Local(pos);

    const qreal oldX = popup->x();
    const qreal oldY = popup->y();

    qCDebug(lcPopupWindow) << "A window system event changed the popup's position to be " << localPos;

    popupPrivate->x = popupPrivate->effectiveX = localPos.x();
    popupPrivate->y = popupPrivate->effectiveY = localPos.y();

    if (!qFuzzyCompare(oldX, localPos.x()))
        emit popup->xChanged();
    if (!qFuzzyCompare(oldY, localPos.y()))
        emit popup->yChanged();
}

void QQuickPopupWindow::implicitWidthChanged()
{
    Q_D(const QQuickPopupWindow);
    if (auto popup = d->m_popup)
        setWidth(popup->implicitWidth());
}

void QQuickPopupWindow::implicitHeightChanged()
{
    Q_D(const QQuickPopupWindow);
    if (auto popup = d->m_popup)
        setHeight(popup->implicitHeight());
}

QT_END_NAMESPACE

