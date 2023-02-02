// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmenubaritem_p.h"
#include "qquickmenubaritem_p_p.h"
#include "qquickmenubar_p.h"
#include "qquickmenu_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuBarItem
    \inherits AbstractButton
//!     \instantiates QQuickMenuBarItem
    \inqmlmodule QtQuick.Controls
    \since 5.10
    \ingroup qtquickcontrols-menus
    \brief Presents a drop-down menu within a MenuBar.

    MenuBarItem presents a Menu within a MenuBar. The respective drop-down menu
    is shown when a MenuBarItem is \l triggered via keyboard, mouse, or touch.

    \image qtquickcontrols-menubar.png

    MenuBarItem is used as a default \l {MenuBar::}{delegate} type for MenuBar.
    Notice that it is not necessary to declare MenuBarItem instances by hand when
    using MenuBar. It is sufficient to declare Menu instances as children of the
    MenuBar and the respective items are created automatically.

    \sa {Customizing MenuBar}, MenuBar, {Menu Controls}
*/

/*!
    \qmlsignal void QtQuick.Controls::MenuBarItem::triggered()

    This signal is emitted when the menu bar item is triggered by the user.
*/

void QQuickMenuBarItemPrivate::setMenuBar(QQuickMenuBar *newMenuBar)
{
    Q_Q(QQuickMenuBarItem);
    if (menuBar == newMenuBar)
        return;

    menuBar = newMenuBar;
    emit q->menuBarChanged();
}

bool QQuickMenuBarItemPrivate::handlePress(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickMenuBarItem);
    const bool handled = QQuickAbstractButtonPrivate::handlePress(point, timestamp);
    if (!handled)
        return false;

    const bool wasTouchPress = touchId != -1;
    if (!wasTouchPress) {
        // Open the menu when it's a mouse press.
        emit q->triggered();
    }

    return true;
}

bool QQuickMenuBarItemPrivate::handleRelease(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickMenuBarItem);
    const bool wasTouchPress = touchId != -1;
    const bool handled = QQuickAbstractButtonPrivate::handleRelease(point, timestamp);
    if (!handled)
        return false;

    if (wasDoubleClick || !wasTouchPress) {
        // Don't open the menu on mouse release, as it should be done on press.
        return handled;
    }

    if (wasTouchPress) {
        // Open the menu.
        emit q->triggered();
    }

    return true;
}

QPalette QQuickMenuBarItemPrivate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::MenuBar);
}

QQuickMenuBarItem::QQuickMenuBarItem(QQuickItem *parent)
    : QQuickAbstractButton(*(new QQuickMenuBarItemPrivate), parent)
{
    setFocusPolicy(Qt::NoFocus);
}

/*!
    \qmlproperty Menu QtQuick.Controls::MenuBarItem::menuBar
    \readonly

    This property holds the menu bar that contains this item,
    or \c null if the item is not in a menu bar.
*/
QQuickMenuBar *QQuickMenuBarItem::menuBar() const
{
    Q_D(const QQuickMenuBarItem);
    return d->menuBar;
}

/*!
    \qmlproperty Menu QtQuick.Controls::MenuBarItem::menu

    This property holds the menu that this item presents in a
    menu bar, or \c null if this item does not have a menu.
*/
QQuickMenu *QQuickMenuBarItem::menu() const
{
    Q_D(const QQuickMenuBarItem);
    return d->menu;
}

void QQuickMenuBarItem::setMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenuBarItem);
    if (d->menu == menu)
        return;

    if (d->menu)
        disconnect(d->menu, &QQuickMenu::titleChanged, this, &QQuickAbstractButton::setText);

    if (menu) {
        setText(menu->title());
        menu->setY(height());
        menu->setParentItem(this);
        menu->setClosePolicy(QQuickPopup::CloseOnEscape | QQuickPopup::CloseOnPressOutsideParent | QQuickPopup::CloseOnReleaseOutsideParent);
        connect(menu, &QQuickMenu::titleChanged, this, &QQuickAbstractButton::setText);
    }

    d->menu = menu;
    emit menuChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::MenuBarItem::highlighted

    This property holds whether the menu bar item is highlighted by the user.

    A menu bar item can be highlighted by mouse hover or keyboard navigation.

    The default value is \c false.
*/
bool QQuickMenuBarItem::isHighlighted() const
{
    Q_D(const QQuickMenuBarItem);
    return d->highlighted;
}

void QQuickMenuBarItem::setHighlighted(bool highlighted)
{
    Q_D(QQuickMenuBarItem);
    if (highlighted == d->highlighted)
        return;

    d->highlighted = highlighted;
    emit highlightedChanged();
}

bool QQuickMenuBarItem::event(QEvent *event)
{
#if QT_CONFIG(shortcut)
    Q_D(QQuickMenuBarItem);
    if (event->type() == QEvent::Shortcut) {
        auto *shortcutEvent = static_cast<QShortcutEvent *>(event);
        if (shortcutEvent->shortcutId() == d->shortcutId) {
            d->trigger();
            emit triggered();
            return true;
        }
    }
#endif
    return QQuickControl::event(event);
}

void QQuickMenuBarItem::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickMenuBarItem);
    QQuickControl::keyPressEvent(event);
    if (d->acceptKeyClick(static_cast<Qt::Key>(event->key()))) {
        d->setPressPoint(d->centerPressPoint());
        setPressed(true);
        emit pressed();
        event->accept();
    }
}

void QQuickMenuBarItem::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickMenuBarItem);
    QQuickControl::keyReleaseEvent(event);
    if (d->pressed && d->acceptKeyClick(static_cast<Qt::Key>(event->key()))) {
        setPressed(false);
        emit released();
        d->trigger();
        // We override these event functions so that we can emit triggered here.
        // We can't just connect clicked to triggered, because that would cause mouse clicks
        // to open the menu, when only presses should.
        emit triggered();
        event->accept();
    }
}

void QQuickMenuBarItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickMenuBarItem);
    QQuickAbstractButton::geometryChange(newGeometry, oldGeometry);
    if (d->menu)
        d->menu->setY(newGeometry.height());
}

QFont QQuickMenuBarItem::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::MenuBar);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickMenuBarItem::accessibleRole() const
{
    return QAccessible::MenuBar;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickmenubaritem_p.cpp"
