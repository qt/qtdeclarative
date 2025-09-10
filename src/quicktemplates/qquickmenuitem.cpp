// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmenuitem_p.h"
#include "qquickmenuitem_p_p.h"
#include "qquickmenu_p_p.h"
#include "qquickdeferredexecute_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>
#include <QtQuick/private/qquickevents_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuItem
    \inherits AbstractButton
//!     \nativetype QQuickMenuItem
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-menus
    \brief Presents an item within a Menu.

    MenuItem is a convenience type that implements the AbstractButton API,
    providing a familiar way to respond to menu items being \l triggered, for
    example.

    MenuItem inherits its API from AbstractButton. For instance, you can set
    \l {AbstractButton::text}{text} and \l {Icons in Qt Quick Controls}{icon}
    using the AbstractButton API.

    \code
    Button {
        id: fileButton
        text: "File"
        onClicked: menu.open()

        Menu {
            id: menu

            MenuItem {
                text: "New..."
                onTriggered: document.reset()
            }
            MenuItem {
                text: "Open..."
                onTriggered: openDialog.open()
            }
            MenuItem {
                text: "Save"
                onTriggered: saveDialog.open()
            }
        }
    }
    \endcode

    \sa {Customizing Menu}, Menu, {Menu Controls}
*/

/*!
    \qmlproperty real QtQuick.Controls::MenuItem::textPadding
    \readonly
    \since 6.8

    This property holds the maximum \l implicitTextPadding found
    among all the menu items inside the same \l menu.

    This property can be used by the style to ensure that all MenuItems
    inside the same Menu end up aligned with respect to the
    \l {AbstractButton::} {text}.

    A \l Menu can consist of meny different MenuItems, some can be checkable,
    some can have an icon, and some will just contain text. And very often,
    a style wants to make sure that the text inside all of them ends up
    left-aligned (or right-aligned for \l {Control::} {mirrored} items).
    By letting each MenuItem assign its own minimum text padding to
    \l implicitTextPadding (taking icons and checkmarks into account), but
    using \l textPadding to actually position the
    \l {AbstractButton::} {text}, all MenuItems should end up being aligned

    In order for this to work, all MenuItems should set \l implicitTextPadding
    to be the minimum space needed from the left edge of the
    \l {Control::} {contentItem} to the text.

    \sa implicitTextPadding
*/

/*!
    \qmlproperty real QtQuick.Controls::MenuItem::implicitTextPadding
    \since 6.8

    This property holds the minimum space needed from the left edge of the
    \l {Control::} {contentItem} to the text. It's used to calculate a common
    \l textPadding among all the MenuItems inside a \l Menu.

    \sa textPadding
*/

void QQuickMenuItemPrivate::setMenu(QQuickMenu *newMenu)
{
    Q_Q(QQuickMenuItem);
    if (menu == newMenu)
        return;

    menu = newMenu;
    emit q->menuChanged();
}

void QQuickMenuItemPrivate::setSubMenu(QQuickMenu *newSubMenu)
{
    Q_Q(QQuickMenuItem);
    if (subMenu == newSubMenu)
        return;

    if (subMenu) {
        QObject::disconnect(subMenu, &QQuickMenu::titleChanged, q, &QQuickAbstractButton::setText);
        QObject::disconnect(subMenu, &QQuickMenu::iconChanged, q, &QQuickAbstractButton::setIcon);
        QObjectPrivate::disconnect(subMenu, &QQuickPopup::enabledChanged, this, &QQuickMenuItemPrivate::updateEnabled);
    }

    if (newSubMenu) {
        QObject::connect(newSubMenu, &QQuickMenu::titleChanged, q, &QQuickAbstractButton::setText);
        QObject::connect(newSubMenu, &QQuickMenu::iconChanged, q, &QQuickAbstractButton::setIcon);
        QObjectPrivate::connect(newSubMenu, &QQuickPopup::enabledChanged, this, &QQuickMenuItemPrivate::updateEnabled);
        q->setText(newSubMenu->title());
        q->setIcon(newSubMenu->icon());
    }

    subMenu = newSubMenu;
    updateEnabled();
    emit q->subMenuChanged();
}

void QQuickMenuItemPrivate::updateEnabled()
{
    Q_Q(QQuickMenuItem);
    q->setEnabled(subMenu && subMenu->isEnabled());
}

static inline QString arrowName() { return QStringLiteral("arrow"); }

void QQuickMenuItemPrivate::cancelArrow()
{
    Q_Q(QQuickAbstractButton);
    quickCancelDeferred(q, arrowName());
}

void QQuickMenuItemPrivate::executeArrow(bool complete)
{
    Q_Q(QQuickMenuItem);
    if (arrow.wasExecuted())
        return;

    if (!arrow || complete)
        quickBeginDeferred(q, arrowName(), arrow);
    if (complete)
        quickCompleteDeferred(q, arrowName(), arrow);
}

bool QQuickMenuItemPrivate::acceptKeyClick(Qt::Key key) const
{
    return key == Qt::Key_Return || key == Qt::Key_Enter
            || QQuickAbstractButtonPrivate::acceptKeyClick(key);
}

QPalette QQuickMenuItemPrivate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::Menu);
}

/*!
    \qmlsignal void QtQuick.Controls::MenuItem::triggered()

    This signal is emitted when the menu item is triggered by the user.
*/

QQuickMenuItem::QQuickMenuItem(QQuickItem *parent)
    : QQuickAbstractButton(*(new QQuickMenuItemPrivate), parent)
{
    connect(this, &QQuickAbstractButton::clicked, this, &QQuickMenuItem::triggered);
}

/*!
    \qmlproperty bool QtQuick.Controls::MenuItem::highlighted

    This property holds whether the menu item is highlighted by the user.

    A menu item can be highlighted by mouse hover or keyboard navigation.

    The default value is \c false.

    \sa Menu::currentIndex
*/
bool QQuickMenuItem::isHighlighted() const
{
    Q_D(const QQuickMenuItem);
    return d->highlighted;
}

void QQuickMenuItem::setHighlighted(bool highlighted)
{
    Q_D(QQuickMenuItem);
    if (highlighted == d->highlighted)
        return;

    d->highlighted = highlighted;
    emit highlightedChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty Item QtQuick.Controls::MenuItem::arrow

    This property holds the sub-menu arrow item.

    \sa {Customizing Menu}
*/
QQuickItem *QQuickMenuItem::arrow() const
{
    QQuickMenuItemPrivate *d = const_cast<QQuickMenuItemPrivate *>(d_func());
    if (!d->arrow)
        d->executeArrow();
    return d->arrow;
}

void QQuickMenuItem::setArrow(QQuickItem *arrow)
{
    Q_D(QQuickMenuItem);
    if (d->arrow == arrow)
        return;

    if (!d->arrow.isExecuting())
        d->cancelArrow();

    QQuickControlPrivate::hideOldItem(d->arrow);
    d->arrow = arrow;
    if (arrow && !arrow->parentItem())
        arrow->setParentItem(this);
    if (!d->arrow.isExecuting())
        emit arrowChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty Menu QtQuick.Controls::MenuItem::menu
    \readonly

    This property holds the menu that contains this menu item,
    or \c null if the item is not in a menu.
*/
QQuickMenu *QQuickMenuItem::menu() const
{
    Q_D(const QQuickMenuItem);
    return d->menu;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty Menu QtQuick.Controls::MenuItem::subMenu
    \readonly

    This property holds the sub-menu that this item presents in
    the parent menu, or \c null if this item is not a sub-menu item.
*/
QQuickMenu *QQuickMenuItem::subMenu() const
{
    Q_D(const QQuickMenuItem);
    return d->subMenu;
}

void QQuickMenuItem::componentComplete()
{
    Q_D(QQuickMenuItem);
    d->executeArrow(true);
    QQuickAbstractButton::componentComplete();
}

QFont QQuickMenuItem::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::Menu);
}

qreal QQuickMenuItem::implicitTextPadding() const
{
    return d_func()->implicitTextPadding;
}

void QQuickMenuItem::setImplicitTextPadding(qreal newImplicitTextPadding)
{
    Q_D(QQuickMenuItem);
    if (qFuzzyCompare(d->implicitTextPadding, newImplicitTextPadding))
        return;
    d->implicitTextPadding = newImplicitTextPadding;
    emit implicitTextPaddingChanged();
}

qreal QQuickMenuItem::textPadding() const
{
    Q_D(const QQuickMenuItem);
    return d->menu ? QQuickMenuPrivate::get(d->menu)->textPadding : 0;
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickMenuItem::accessibleRole() const
{
    return QAccessible::MenuItem;
}
#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QQuickMenuItem *menuItem)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    if (!menuItem) {
        debug << "QQuickMenuItem(nullptr)";
        return debug;
    }

    debug << menuItem->metaObject()->className() << '(' << static_cast<const void *>(menuItem);
    if (!menuItem->objectName().isEmpty())
        debug << ", name=" << menuItem->objectName();
    debug << ", text=" << menuItem->text();
    debug << ')';
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#include "moc_qquickmenuitem_p.cpp"
