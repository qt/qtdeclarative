// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmenuitem_p.h"

#include <QtCore/qloggingcategory.h>
//#include <QtGui/qicon.h>
//#if QT_CONFIG(shortcut)
//#include <QtGui/qkeysequence.h>
//#endif
#include <QtGui/qpa/qplatformmenu.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
//#include <QtQuickTemplates2/private/qquickshortcutcontext_p_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p_p.h>
#include <QtQuickTemplates2/private/qquickaction_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p_p.h>
#include <QtQuickTemplates2/private/qquickmenuseparator_p.h>
#include <QtQuickTemplates2/private/qquicknativeiconloader_p.h>
#include <QtQuickTemplates2/private/qquicknativemenuitem_p.h>
#include <QtQuickTemplates2/private/qquickshortcutcontext_p_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcNativeMenuItem, "qt.quick.controls.nativemenuitem")

/*!
    \class QQuickNativeMenuItem
    \brief A native menu item.
    \since 6.7
    \internal

    Creates a native menu item from an Action/MenuItem/Menu,
    and syncs the properties and signals. It can also represent a
    MenuSeparator.

    \sa Menu, Action
*/

QQuickNativeMenuItem *QQuickNativeMenuItem::createFromNonNativeItem(
    QQuickMenu *parentMenu, QQuickItem *nonNativeItem)
{
    auto *menuItem = qobject_cast<QQuickMenuItem *>(nonNativeItem);
    Type type = Type::Unknown;
    if (menuItem) {
        if (menuItem->action()) {
            type = Type::Action;
        } else if (menuItem->subMenu()) {
            type = Type::SubMenu;
        } else {
            // It's a plain MenuItem, rather than a MenuItem created by us for an Action or Menu.
            type = Type::MenuItem;
        }
    } else if (qobject_cast<QQuickMenuSeparator *>(nonNativeItem)) {
        type = Type::Separator;
    }

    std::unique_ptr<QQuickNativeMenuItem> nativeMenuItemPtr(new QQuickNativeMenuItem(
        parentMenu, nonNativeItem, type));
    if (type == Type::Unknown) {
        // It's not a Menu/Action/MenuSeparator that we're dealing with, but we still need
        // to create the QQuickNativeMenu item for it so that our container has the same
        // indices as the menu's contentModel.
        return nativeMenuItemPtr.release();
    }

    qCDebug(lcNativeMenuItem) << "attemping to create native menu item for"
        << nativeMenuItemPtr->debugText();
    auto *parentMenuPrivate = QQuickMenuPrivate::get(parentMenu);
    nativeMenuItemPtr->m_handle.reset(parentMenuPrivate->handle->createMenuItem());
    if (!nativeMenuItemPtr->m_handle)
        nativeMenuItemPtr->m_handle.reset(QGuiApplicationPrivate::platformTheme()->createPlatformMenuItem());
    if (!nativeMenuItemPtr->m_handle)
        return nullptr;

    auto *nativeMenuItem = nativeMenuItemPtr.release();
    switch (type) {
    case Type::Action:
        // Ensure that the action is triggered when the user clicks on a native menu item.
        connect(nativeMenuItem->m_handle.get(), &QPlatformMenuItem::activated,
                nativeMenuItem->action(), [nativeMenuItem, parentMenu](){
            nativeMenuItem->action()->trigger(parentMenu);
        });
        // Handle programmatic changes in the Action.
        connect(nativeMenuItem->action(), &QQuickAction::textChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        connect(nativeMenuItem->action(), &QQuickAction::iconChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        connect(nativeMenuItem->action(), &QQuickAction::enabledChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        connect(nativeMenuItem->action(), &QQuickAction::checkedChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        connect(nativeMenuItem->action(), &QQuickAction::checkableChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        break;
    case Type::SubMenu:
        nativeMenuItem->m_handle->setMenu(QQuickMenuPrivate::get(
            nativeMenuItem->subMenu())->handle.get());

        // Handle programmatic changes in the Menu.
        connect(nativeMenuItem->subMenu(), &QQuickMenu::enabledChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        connect(nativeMenuItem->subMenu(), &QQuickMenu::titleChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        break;
    case Type::MenuItem:
        // Ensure that the MenuItem is clicked when the user clicks on a native menu item.
        connect(nativeMenuItem->m_handle.get(), &QPlatformMenuItem::activated,
                menuItem, [menuItem](){
            if (menuItem->isCheckable()) {
                // This changes the checked state, which we need when syncing but also to ensure that
                // the user can still use MenuItem's API even though they can't actually interact with it.
                menuItem->toggle();
            }
            // The same applies here: allow users to respond to the MenuItem's clicked signal.
            QQuickAbstractButtonPrivate::get(menuItem)->click();
        });
        // Handle programmatic changes in the MenuItem.
        connect(menuItem, &QQuickMenuItem::textChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        connect(menuItem, &QQuickMenuItem::iconChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        connect(menuItem, &QQuickMenuItem::enabledChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        connect(menuItem, &QQuickMenuItem::checkedChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        connect(menuItem, &QQuickMenuItem::checkableChanged, nativeMenuItem, &QQuickNativeMenuItem::sync);
        break;
    case Type::Separator:
    case Type::Unknown:
        break;
    }

    return nativeMenuItem;
}

QQuickNativeMenuItem::QQuickNativeMenuItem(QQuickMenu *parentMenu, QQuickItem *nonNativeItem,
        QQuickNativeMenuItem::Type type)
    : QObject(parentMenu)
    , m_parentMenu(parentMenu)
    , m_nonNativeItem(nonNativeItem)
    , m_type(type)
{
}

QQuickNativeMenuItem::~QQuickNativeMenuItem()
{
    qCDebug(lcNativeMenuItem) << "destroying" << this << debugText();
}

QQuickAction *QQuickNativeMenuItem::action() const
{
    return m_type == Type::Action ? qobject_cast<QQuickMenuItem *>(m_nonNativeItem)->action() : nullptr;
}

QQuickMenu *QQuickNativeMenuItem::subMenu() const
{
    return m_type == Type::SubMenu ? qobject_cast<QQuickMenuItem *>(m_nonNativeItem)->subMenu() : nullptr;
}

QQuickMenuSeparator *QQuickNativeMenuItem::separator() const
{
    return m_type == Type::Separator ? qobject_cast<QQuickMenuSeparator *>(m_nonNativeItem) : nullptr;
}

QPlatformMenuItem *QQuickNativeMenuItem::handle() const
{
    return m_handle.get();
}

void QQuickNativeMenuItem::sync()
{
    if (m_type == Type::Unknown)
        return;

    if (m_syncing)
        return;

    QScopedValueRollback recursionGuard(m_syncing, true);

    const auto *action = this->action();
    const auto *separator = this->separator();
    auto *subMenu = this->subMenu();
    auto *menuItem = qobject_cast<QQuickMenuItem *>(m_nonNativeItem);

    // Store the values in variables so that we can use it in the debug output below.
    const bool enabled = action ? action->isEnabled()
        : subMenu ? subMenu->isEnabled()
        : menuItem && menuItem->isEnabled();
    m_handle->setEnabled(enabled);
//    m_handle->setVisible(isVisible());

    const bool isSeparator = separator != nullptr;
    m_handle->setIsSeparator(isSeparator);

    const bool checkable = action ? action->isCheckable() : menuItem && menuItem->isCheckable();
    m_handle->setCheckable(checkable);

    const bool checked = action ? action->isChecked() : menuItem && menuItem->isChecked();
    m_handle->setChecked(checked);

    m_handle->setRole(QPlatformMenuItem::TextHeuristicRole);

    const QString text = action ? action->text()
        : subMenu ? subMenu->title()
        : menuItem ? menuItem->text() : QString();
    m_handle->setText(text);

//    m_handle->setFont(m_font);
//    m_handle->setHasExclusiveGroup(m_group && m_group->isExclusive());
    m_handle->setHasExclusiveGroup(false);

    const QQuickIcon icon = effectiveIcon();
    const auto *menuPrivate = QQuickMenuPrivate::get(m_parentMenu);

    // We should reload the icon if the window's DPR has changed, regardless if its properties have changed.
    // We can't check for ItemDevicePixelRatioHasChanged in QQuickMenu::itemChange,
    // because that isn't sent when the menu isn't visible, and will never
    // be sent for native menus. We instead store lastDevicePixelRatio in QQuickMenu
    // (to avoid storing it for each menu item) and set it whenever it's opened.
    bool dprChanged = false;
    if (!qGuiApp->topLevelWindows().isEmpty()) {
        const auto *window = qGuiApp->topLevelWindows().first();
        dprChanged = !qFuzzyCompare(window->devicePixelRatio(), menuPrivate->lastDevicePixelRatio);
    }

    if (!icon.isEmpty() && (icon != iconLoader()->icon() || dprChanged)) {
        // This will load the icon, which will call sync() recursively, hence the m_syncing check.
        reloadIcon();
    }

    if (subMenu) {
        // Sync first as dynamically created menus may need to get the handle recreated.
        auto *subMenuPrivate = QQuickMenuPrivate::get(subMenu);
        subMenuPrivate->syncWithNativeMenu();
        if (subMenuPrivate->handle)
            m_handle->setMenu(subMenuPrivate->handle.get());
    }

#if QT_CONFIG(shortcut)
    if (action)
        m_handle->setShortcut(action->shortcut());
#endif

    if (m_parentMenu) {
        auto *menuPrivate = QQuickMenuPrivate::get(m_parentMenu);
        if (menuPrivate->handle)
            menuPrivate->handle->syncMenuItem(m_handle.get());
    }

    qCDebug(lcNativeMenuItem) << "sync called on" << debugText() << "handle" << m_handle.get()
        << "enabled:" << enabled << "isSeparator" << isSeparator << "checkable" << checkable
        << "checked" << checked << "text" << text;
}

QQuickIcon QQuickNativeMenuItem::effectiveIcon() const
{
    if (const auto *action = this->action())
        return action->icon();
    if (const auto *subMenu = this->subMenu())
        return subMenu->icon();
    if (const auto *menuItem = qobject_cast<QQuickMenuItem *>(m_nonNativeItem))
        return menuItem->icon();
    return {};
}

QQuickNativeIconLoader *QQuickNativeMenuItem::iconLoader() const
{
    if (!m_iconLoader) {
        QQuickNativeMenuItem *that = const_cast<QQuickNativeMenuItem *>(this);
        static int slot = staticMetaObject.indexOfSlot("updateIcon()");
        m_iconLoader = new QQuickNativeIconLoader(slot, that);
        // Qt Labs Platform's QQuickMenuItem would call m_iconLoader->setEnabled(m_complete) here,
        // but since QQuickMenuPrivate::maybeCreateAndInsertNativeItem asserts that the menu's
        // completed loading, we can just set it to true.
       m_iconLoader->setEnabled(true);
    }
    return m_iconLoader;
}

void QQuickNativeMenuItem::reloadIcon()
{
    iconLoader()->setIcon(effectiveIcon());
    m_handle->setIcon(iconLoader()->toQIcon());
}

void QQuickNativeMenuItem::updateIcon()
{
    sync();
}

void QQuickNativeMenuItem::addShortcut()
{
#if QT_CONFIG(shortcut)
    const auto *action = this->action();
    const QKeySequence sequence = action ? action->shortcut() : QKeySequence();
    if (!sequence.isEmpty() && action->isEnabled()) {
        m_shortcutId = QGuiApplicationPrivate::instance()->shortcutMap.addShortcut(this, sequence,
            Qt::WindowShortcut, QQuickShortcutContext::matcher);
    } else {
        m_shortcutId = -1;
    }
#endif
}

void QQuickNativeMenuItem::removeShortcut()
{
#if QT_CONFIG(shortcut)
    if (m_shortcutId == -1)
        return;

    QKeySequence sequence;
    switch (m_type) {
    case Type::Action:
        sequence = action()->shortcut();
        break;
    default:
        // TODO
        break;
    }

    QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(m_shortcutId, this, sequence);
#endif
}

QString QQuickNativeMenuItem::debugText() const
{
    // Prepend below to avoid extra lines from break statements.
    QString text = QDebug::toString(m_handle.get());

    switch (m_type) {
    case Type::Action:
        return text.prepend(QString::fromLatin1("Action(text = %1) ").arg(action()->text()));
    case Type::SubMenu:
        return text.prepend(QString::fromLatin1("Sub-menu(title = %1) ").arg(subMenu()->title()));
    case Type::MenuItem:
        return text.prepend(QString::fromLatin1("MenuItem(text = %1) ").arg(
            qobject_cast<QQuickMenuItem *>(m_nonNativeItem)->text()));
    case Type::Separator:
        return text.prepend(QStringLiteral("Separator "));
    case Type::Unknown:
        return text.prepend(QStringLiteral("(Unknown) "));
    }

    Q_UNREACHABLE();
}

QT_END_NAMESPACE

#include "moc_qquicknativemenuitem_p.cpp"
