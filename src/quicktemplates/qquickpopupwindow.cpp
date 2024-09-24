// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpopupwindow_p_p.h"
#include "qquickcombobox_p.h"
#include "qquickdialog_p.h"
#include "qquickpopup_p.h"
#include "qquickpopup_p_p.h"
#include "qquickmenu_p_p.h"
#include "qquickmenubar_p_p.h"
#include "qquickpopupitem_p_p.h"
#include <QtGui/private/qguiapplication_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtGui/private/qeventpoint_p.h>
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
    bool m_inHideEvent = false;

    void setVisible(bool visible) override;

    void forwardEventToParentMenuOrMenuBar(QEvent *event);
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

    qCDebug(lcPopupWindow) << "Created popup window with parent:" << parent << "flags:" << flags;
}

QQuickPopupWindow::~QQuickPopupWindow()
{
    Q_D(QQuickPopupWindow);
    disconnect(d->m_popup, &QQuickPopup::windowChanged, this, &QQuickPopupWindow::windowChanged);
    disconnect(d->m_popup, &QQuickPopup::implicitWidthChanged, this, &QQuickPopupWindow::implicitWidthChanged);
    disconnect(d->m_popup, &QQuickPopup::implicitHeightChanged, this, &QQuickPopupWindow::implicitHeightChanged);
    if (d->m_popup->window()) {
        disconnect(d->m_popup->window(), &QWindow::xChanged, this, &QQuickPopupWindow::parentWindowXChanged);
        disconnect(d->m_popup->window(), &QWindow::yChanged, this, &QQuickPopupWindow::parentWindowYChanged);
    }
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
    // Avoid potential infinite recursion, between QWindowPrivate::setVisible(false) and this function.
    QScopedValueRollback<bool>inHideEventRollback(d->m_inHideEvent, true);
    if (QQuickPopup *popup = d->m_popup) {
        QQuickDialog *dialog = qobject_cast<QQuickDialog *>(popup);
        if (dialog && QQuickPopupPrivate::get(dialog)->visible)
            dialog->reject();
        else
            popup->setVisible(false);
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
    const QMarginsF windowInsets = QQuickPopupPrivate::get(d->m_popup)->windowInsets();
    d->m_popupItem->setWidth(e->size().width() - windowInsets.left() - windowInsets.right());
    d->m_popupItem->setHeight(e->size().height() - windowInsets.top() - windowInsets.bottom());
}

void QQuickPopupWindowPrivate::setVisible(bool visible)
{
    if (m_inHideEvent)
        return;

    QQuickWindowQmlImplPrivate::setVisible(visible);
}

/*! \internal
    We want to handle menus specially, compared to other popups. For menus, we
    want all open parent menus and sub menus that belong together to almost
    act as a single popup WRT hover event delivery. This will allow the user to
    hover and highlight MenuItems inside all of them, not just the leaf menu.
    This function will therefore find the menu, or menu bar, under the event's
    position, and forward the event to it. But note that this forwarding will
    happen in parallel with normal event delivery (we don't eat the events), as
    we don't want to break the delivery to e.g grabbers.
 */
void QQuickPopupWindowPrivate::forwardEventToParentMenuOrMenuBar(QEvent *event)
{
    Q_Q(QQuickPopupWindow);

    if (!event->isPointerEvent())
        return;
    auto menu = qobject_cast<QQuickMenu *>(q->popup());
    if (!menu)
        return;

    auto *pe = static_cast<QPointerEvent *>(event);
    const QPointF globalPos = pe->points().first().globalPosition();

    // Resolve the Menu or MenuBar under the mouse, if any
    QQuickMenu *targetMenu = nullptr;
    QQuickMenuBar *targetMenuBar = nullptr;
    QObject *menuParent = menu;
    while (menuParent) {
        if (auto parentMenu = qobject_cast<QQuickMenu *>(menuParent)) {
            QQuickPopupWindow *popupWindow = QQuickMenuPrivate::get(parentMenu)->popupWindow;
            auto popup_d = QQuickPopupPrivate::get(popupWindow->popup());
            QPointF scenePos = popupWindow->contentItem()->mapFromGlobal(globalPos);
            if (popup_d->contains(scenePos)) {
                targetMenu = parentMenu;
                break;
            }
        } else if (auto menuBar = qobject_cast<QQuickMenuBar *>(menuParent)) {
            const QPointF menuBarPos = menuBar->mapFromGlobal(globalPos);
            if (menuBar->contains(menuBarPos))
                targetMenuBar = menuBar;
            break;
        }

        menuParent = menuParent->parent();
    }

    if (pe->isBeginEvent()) {
        if (!targetMenu) {
            // A QQuickPopupWindow can be bigger than the menu itself, to make room
            // for a drop-shadow. Close all menus if the user do a press, either on
            // the shadow, or outside the menu.
            QGuiApplicationPrivate::closeAllPopups();
            return;
        }
    } else if (pe->isUpdateEvent()){
        QQuickWindow *targetWindow = nullptr;
        if (targetMenu)
            targetWindow = QQuickPopupPrivate::get(targetMenu)->popupWindow;
        else if (targetMenuBar)
            targetWindow = targetMenuBar->window();
        else
            return;

        // Forward move events to the target window
        const auto scenePos = pe->point(0).scenePosition();
        const auto translatedScenePos = targetWindow->mapFromGlobal(globalPos);
        QMutableEventPoint::setScenePosition(pe->point(0), translatedScenePos);
        auto *grabber = pe->exclusiveGrabber(pe->point(0));

        if (grabber) {
            // Temporarily disable the grabber, to stop the delivery agent inside
            // targetWindow from forwarding the event to an item outside the menu
            // or menubar. This is especially important to support a press on e.g
            // a MenuBarItem, followed by a drag-and-release on top of a MenuItem.
            pe->setExclusiveGrabber(pe->point(0), nullptr);
        }

        qCDebug(lcPopupWindow) << "forwarding" << pe << "to popup menu:" << targetWindow;
        QQuickWindowPrivate::get(targetWindow)->deliveryAgent->event(pe);

        // Restore the event before we return
        QMutableEventPoint::setScenePosition(pe->point(0), scenePos);
        if (grabber)
            pe->setExclusiveGrabber(pe->point(0), grabber);
    } else if (pe->isEndEvent()) {
        if (!targetMenu) {
            // Close all menus if the pressAndHold didn't end up on top of a menu
            int pressDuration = pe->point(0).timestamp() - pe->point(0).pressTimestamp();
            if (pressDuration >= QGuiApplication::styleHints()->mousePressAndHoldInterval())
                QGuiApplicationPrivate::closeAllPopups();
            return;
        }

        // To support opening a Menu on press (e.g on a MenuBarItem), followed by
        // a drag and release on a MenuItem inside the Menu, we ask the Menu to
        // perform a click on the active MenuItem, if any.
        QQuickMenuPrivate::get(targetMenu)->handleReleaseWithoutGrab(pe->point(0));
    }
}

bool QQuickPopupWindow::event(QEvent *e)
{
    Q_D(QQuickPopupWindow);
    d->forwardEventToParentMenuOrMenuBar(e);

    if (d->m_popup && !d->m_popup->hasFocus() && (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease)
#if QT_CONFIG(shortcut)
        && (!static_cast<QKeyEvent *>(e)->matches(QKeySequence::Cancel)
#if defined(Q_OS_ANDROID)
        || static_cast<QKeyEvent *>(e)->key() != Qt::Key_Back
#endif
        )
#endif
    ) return false;

    return QQuickWindowQmlImpl::event(e);
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
    if (!popup || !popup->window())
        return;
    QQuickPopupPrivate *popupPrivate = QQuickPopupPrivate::get(popup);

    const auto windowPos = global2Local(pos);
    qCDebug(lcPopupWindow) << "A window system event changed the popup's position to be " << windowPos;
    popupPrivate->setEffectivePosFromWindowPos(windowPos);
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

