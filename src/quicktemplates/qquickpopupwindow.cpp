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

Q_STATIC_LOGGING_CATEGORY(lcPopupWindow, "qt.quick.controls.popup.window")

static bool s_popupGrabOk = false;
static QWindow *s_grabbedWindow = nullptr;

class QQuickPopupWindowPrivate : public QQuickWindowQmlImplPrivate
{
    Q_DECLARE_PUBLIC(QQuickPopupWindow)

public:
    QPointer<QQuickItem> m_popupItem;
    QPointer<QQuickPopup> m_popup;
    QPointer<QWindow> m_popupParentItemWindow;
    bool m_inHideEvent = false;

protected:
    void setVisible(bool visible) override;

private:
    bool filterPopupSpecialCases(QEvent *event);
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
    if (QQuickWindow *nearestParentItemWindow = d->m_popup->window()) {
        d->m_popupParentItemWindow = nearestParentItemWindow;
        connect(d->m_popupParentItemWindow, &QWindow::xChanged, this, &QQuickPopupWindow::parentWindowXChanged);
        connect(d->m_popupParentItemWindow, &QWindow::yChanged, this, &QQuickPopupWindow::parentWindowYChanged);
    }
    setWidth(d->m_popupItem->implicitWidth());
    setHeight(d->m_popupItem->implicitHeight());

    const auto flags = QQuickPopupPrivate::get(popup)->popupWindowType();

    // For popup windows, we'll need to draw everything, in order to have enough control over the styling.
    if (flags & Qt::Popup)
        setColor(QColorConstants::Transparent);

    setFlags(flags);

    qCDebug(lcPopupWindow) << "Created popup window with parent:" << parent << "flags:" << flags;
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
    QQuickPopupPrivate *popupPrivate = QQuickPopupPrivate::get(d->m_popup);

    const auto topLeftFromSystem = global2Local(d->geometry.topLeft());
    // We need to use the current topLeft position here, so that reposition()
    // does not move the window
    const auto oldX = popupPrivate->x;
    const auto oldY = popupPrivate->y;

    if (Q_LIKELY(topLeftFromSystem)) {
        popupPrivate->x = topLeftFromSystem->x();
        popupPrivate->y = topLeftFromSystem->y();
    }

    const QMarginsF windowInsets = popupPrivate->windowInsets();
    d->m_popupItem->setWidth(e->size().width() - windowInsets.left() - windowInsets.right());
    d->m_popupItem->setHeight(e->size().height() - windowInsets.top() - windowInsets.bottom());

    // and restore the actual x and y afterwards
    popupPrivate->x = oldX;
    popupPrivate->y = oldY;
}

void QQuickPopupWindowPrivate::setVisible(bool visible)
{
    if (m_inHideEvent)
        return;

    const bool visibleChanged = QWindowPrivate::visible != visible;

    // Check if we're about to close the last popup, in which case, ungrab.
    if (!visible && visibleChanged && QGuiApplicationPrivate::popupCount() == 1 && s_grabbedWindow) {
        s_grabbedWindow->setMouseGrabEnabled(false);
        s_grabbedWindow->setKeyboardGrabEnabled(false);
        s_popupGrabOk = false;
        qCDebug(lcPopupWindow) << "The window " << s_grabbedWindow << "has disabled global mouse and keyboard grabs.";
        s_grabbedWindow = nullptr;
    }

    QQuickWindowQmlImplPrivate::setVisible(visible);

    // Similar logic to grabForPopup(QWidget *popup)
    // If the user clicks outside, popups with CloseOnPressOutside*/CloseOnReleaseOutside* need to be able to react,
    // in order to determine if they should close.
    // Pointer press and release events should also be filtered by the top-most popup window, and only be delivered to other windows in rare cases.
    if (visible && visibleChanged && QGuiApplicationPrivate::popupCount() == 1 && !s_popupGrabOk) {
        QWindow *win = m_popup->window();
        if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
            return; // workaround for QTBUG-134009
        s_popupGrabOk = win->setKeyboardGrabEnabled(true);
        if (s_popupGrabOk) {
            s_popupGrabOk = win->setMouseGrabEnabled(true);
            if (!s_popupGrabOk)
                win->setKeyboardGrabEnabled(false);
            s_grabbedWindow = win;
            qCDebug(lcPopupWindow) << "The window" << win << "has enabled global mouse" << (s_popupGrabOk ? "and keyboard" : "") << "grabs.";
        }
    }
}

/*! \internal
    Even if all pointer events are sent to the active popup, there are cases
    where we need to take several popups, or even the menu bar, into account
    to figure out what the event should do.

    - When clicking outside a popup, the closePolicy should determine whether the
      popup should close or not. When closing a menu this way, all other menus
      that are grouped together should also close.

    - We want all open menus and sub menus that belong together to almost act as
      a single popup WRT hover event delivery. This will allow the user to hover
      and highlight MenuItems inside all of them, not just this menu. This function
      will therefore find the menu, or menu bar, under the event's position, and
      forward hover events to it.

    Note that we for most cases want to return false from this function, even if
    the event was actually handled. That way it will be also sent to the DA, to
    let normal event delivery to any potential grabbers happen the usual way. It
    will also allow QGuiApplication to forward the event to the window under the
    pointer if the event was outside of any popups (if supported by e.g
    QPlatformIntegration::ReplayMousePressOutsidePopup).
 */
bool QQuickPopupWindowPrivate::filterPopupSpecialCases(QEvent *event)
{
    Q_Q(QQuickPopupWindow);

    if (!event->isPointerEvent())
        return false;

    QQuickPopup *popup = m_popup;
    if (!popup)
        return false;

    auto *pe = static_cast<QPointerEvent *>(event);
    const QPointF globalPos = pe->points().first().globalPosition();
    const QQuickPopup::ClosePolicy closePolicy = popup->closePolicy();
    QQuickPopup *targetPopup = QQuickPopupPrivate::get(popup)->contains(contentItem->mapFromGlobal(globalPos)) ? popup : nullptr;

    // Resolve the Menu or MenuBar under the mouse, if any
    QQuickMenu *menu = qobject_cast<QQuickMenu *>(popup);
    QQuickMenuBar *targetMenuBar = nullptr;
    QObject *menuParent = menu;
    while (menuParent) {
        if (auto *parentMenu = qobject_cast<QQuickMenu *>(menuParent)) {
            QQuickPopupWindow *popupWindow = QQuickMenuPrivate::get(parentMenu)->popupWindow;
            auto *popup_d = QQuickPopupPrivate::get(popupWindow->popup());
            QPointF scenePos = popupWindow->contentItem()->mapFromGlobal(globalPos);
            if (popup_d->contains(scenePos)) {
                targetPopup = parentMenu;
                break;
            }
        } else if (auto *menuBar = qobject_cast<QQuickMenuBar *>(menuParent)) {
            const QPointF menuBarPos = menuBar->mapFromGlobal(globalPos);
            if (menuBar->contains(menuBarPos))
                targetMenuBar = menuBar;
            break;
        }

        menuParent = menuParent->parent();
    }

    auto closePopupAndParentMenus = [q]() {
        QQuickPopup *current = q->popup();
        do {
            qCDebug(lcPopupWindow) << "Closing" << current << "from an outside pointer press or release event";
            current->close();
            current = qobject_cast<QQuickMenu *>(current->parent());
        } while (current);
    };

    if (pe->isBeginEvent()) {
        if (targetMenuBar) {
            // If the press was on top of the menu bar, we close all menus and return
            // true. The latter will stop QGuiApplication from propagating the event
            // to the window under the pointer, and therefore also to the MenuBar.
            // The latter would otherwise cause a menu to reopen again immediately, and
            // undermine that we want to close all popups.
            closePopupAndParentMenus();
            return true;
        } else if (!targetPopup && closePolicy.testAnyFlags(QQuickPopup::CloseOnPressOutside | QQuickPopup::CloseOnPressOutsideParent)) {
            // Pressed outside either a popup window, or a menu or menubar that owns a menu using popup windows.
            // Note that A QQuickPopupWindow can be bigger than the
            // menu itself, to make room for a drop-shadow. But if the press was on top
            // of the shadow, targetMenu will still be nullptr.
            closePopupAndParentMenus();
            return false;
        }
    } else if (pe->isUpdateEvent()){
        QQuickWindow *targetWindow = nullptr;
        if (targetPopup)
            targetWindow = QQuickPopupPrivate::get(targetPopup)->popupWindow;
        else if (targetMenuBar)
            targetWindow = targetMenuBar->window();
        else
            return false;

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
        if (!targetPopup && !targetMenuBar && closePolicy.testAnyFlags(QQuickPopup::CloseOnReleaseOutside | QQuickPopup::CloseOnReleaseOutsideParent)) {
            // Released outside either a popup window, or a menu or menubar that owns a menu using popup windows.
            closePopupAndParentMenus();
            return false;
        }

       // To support opening a Menu on press (e.g on a MenuBarItem), followed by
       // a drag and release on a MenuItem inside the Menu, we ask the Menu to
       // perform a click on the active MenuItem, if any.
        if (QQuickMenu *targetMenu = qobject_cast<QQuickMenu *>(targetPopup)) {
            qCDebug(lcPopupWindow) << "forwarding" << pe << "to popup menu:" << targetMenu;
            QQuickMenuPrivate::get(targetMenu)->handleReleaseWithoutGrab(pe->point(0));
        }
    }

    return false;
}

bool QQuickPopupWindow::event(QEvent *e)
{
    Q_D(QQuickPopupWindow);
    if (d->filterPopupSpecialCases(e))
        return true;

    if (QQuickPopup *popup = d->m_popup) {
        // Popups without focus should not consume keyboard events.
        if (!popup->hasFocus() && (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease)
#if QT_CONFIG(shortcut)
            && (!static_cast<QKeyEvent *>(e)->matches(QKeySequence::Cancel)
#if defined(Q_OS_ANDROID)
                || static_cast<QKeyEvent *>(e)->key() != Qt::Key_Back
#endif
            )
#endif
        ) return false;
    }

    return QQuickWindowQmlImpl::event(e);
}

void QQuickPopupWindow::windowChanged(QWindow *window)
{
    Q_D(QQuickPopupWindow);
    if (!d->m_popupParentItemWindow.isNull()) {
        disconnect(d->m_popupParentItemWindow, &QWindow::xChanged, this, &QQuickPopupWindow::parentWindowXChanged);
        disconnect(d->m_popupParentItemWindow, &QWindow::yChanged, this, &QQuickPopupWindow::parentWindowYChanged);
    }
    if (window) {
        d->m_popupParentItemWindow = window;
        connect(window, &QWindow::xChanged, this, &QQuickPopupWindow::parentWindowXChanged);
        connect(window, &QWindow::yChanged, this, &QQuickPopupWindow::parentWindowYChanged);
    } else {
        d->m_popupParentItemWindow.clear();
    }
}

std::optional<QPoint> QQuickPopupWindow::global2Local(const QPoint &pos) const
{
    Q_D(const QQuickPopupWindow);
    QQuickPopup *popup = d->m_popup;
    Q_ASSERT(popup);
    QWindow *mainWindow = d->m_popupParentItemWindow;
    if (!mainWindow)
        mainWindow = transientParent();
    if (Q_UNLIKELY((!mainWindow || mainWindow != popup->window())))
        return std::nullopt;

    const QPoint scenePos = mainWindow->mapFromGlobal(pos);
    // Popup's coordinates are relative to the nearest parent item.
    return popup->parentItem() ? popup->parentItem()->mapFromScene(scenePos).toPoint() : scenePos;
}

void QQuickPopupWindow::parentWindowXChanged(int newX)
{
    const auto popupLocalPos = global2Local({x(), y()});
    if (Q_UNLIKELY(!popupLocalPos))
        return;
    handlePopupPositionChangeFromWindowSystem({ newX + popupLocalPos->x(), y() });
}

void QQuickPopupWindow::parentWindowYChanged(int newY)
{
    const auto popupLocalPos = global2Local({x(), y()});
    if (Q_UNLIKELY(!popupLocalPos))
        return;
    handlePopupPositionChangeFromWindowSystem({ x(), newY + popupLocalPos->y() });
}

void QQuickPopupWindow::handlePopupPositionChangeFromWindowSystem(const QPoint &pos)
{
    Q_D(QQuickPopupWindow);
    QQuickPopup *popup = d->m_popup;
    if (!popup)
        return;

    const auto windowPos = global2Local(pos);
    if (Q_LIKELY(windowPos)) {
        qCDebug(lcPopupWindow) << "A window system event changed the popup's position to be " << *windowPos;
        QQuickPopupPrivate::get(popup)->setEffectivePosFromWindowPos(*windowPos);
    }
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

