// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshortcutcontext_p_p.h"
#include "qquickoverlay_p_p.h"
#include "qquicktooltip_p.h"
#include "qquickmenu_p.h"
#include "qquickmenu_p_p.h"
#include "qquickpopup_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qguiapplication.h>
#include <QtQuick/qquickrendercontrol.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcContextMatcher, "qt.quick.controls.shortcutcontext.matcher")

static bool isBlockedByPopup(QQuickItem *item)
{
    if (!item || !item->window())
        return false;

    QQuickOverlay *overlay = QQuickOverlay::overlay(item->window());
    const auto popups = QQuickOverlayPrivate::get(overlay)->stackingOrderPopups();
    for (QQuickPopup *popup : popups) {
        if (qobject_cast<QQuickToolTip *>(popup))
            continue; // ignore tooltips (QTBUG-60492)
        if (popup->isModal() || popup->closePolicy() & QQuickPopup::CloseOnEscape) {
            qCDebug(lcContextMatcher) << popup << "is modal or has a CloseOnEscape policy;"
                << "if the following are both true," << item << "will be blocked by it:"
                << (item != popup->popupItem()) << !popup->popupItem()->isAncestorOf(item);
            return item != popup->popupItem() && !popup->popupItem()->isAncestorOf(item);
        }
    }

    return false;
}

bool QQuickShortcutContext::matcher(QObject *obj, Qt::ShortcutContext context)
{
    QQuickItem *item = nullptr;
    switch (context) {
    case Qt::ApplicationShortcut:
        return true;
    case Qt::WindowShortcut:
        while (obj && !obj->isWindowType()) {
            item = qobject_cast<QQuickItem *>(obj);
            if (item && item->window()) {
                obj = item->window();
                break;
            } else if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(obj)) {
                obj = popup->window();
                item = popup->popupItem();

                if (!obj) {
                    // The popup has no associated window (yet). However, sub-menus,
                    // unlike top-level menus, will not have an associated window
                    // until their parent menu is opened. So, check if this is a sub-menu
                    // so that actions within it can grab shortcuts.
                    if (auto *menu = qobject_cast<QQuickMenu *>(popup)) {
                        auto parentMenu = QQuickMenuPrivate::get(menu)->parentMenu;
                        while (!obj && parentMenu)
                            obj = parentMenu->window();
                    }
                }
                break;
            }
            obj = obj->parent();
        }
        if (QWindow *renderWindow = QQuickRenderControl::renderWindowFor(qobject_cast<QQuickWindow *>(obj)))
            obj = renderWindow;
        qCDebug(lcContextMatcher) << "obj" << obj << "item" << item << "focusWindow" << QGuiApplication::focusWindow()
            << "!isBlockedByPopup(item)" << !isBlockedByPopup(item);
        return obj && obj == QGuiApplication::focusWindow() && !isBlockedByPopup(item);
    default:
        return false;
    }
}

QT_END_NAMESPACE
