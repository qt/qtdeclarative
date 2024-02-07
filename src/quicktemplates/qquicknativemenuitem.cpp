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
#include <QtQuickTemplates2/private/qquickaction_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p_p.h>
#include <QtQuickTemplates2/private/qquickmenuseparator_p.h>
#include <QtQuickTemplates2/private/qquicknativeiconloader_p.h>
#include <QtQuickTemplates2/private/qquicknativemenuitem_p.h>
#include <QtQuickTemplates2/private/qquickshortcutcontext_p_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcNativeMenuItem, "qt.quick.controls.nativemenuitem")

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

    if (type == Type::Unknown)
        return nullptr;

    std::unique_ptr<QQuickNativeMenuItem> nativeMenuItemPtr(new QQuickNativeMenuItem(
        parentMenu, nonNativeItem, type));
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
        connect(nativeMenuItem->m_handle.get(), &QPlatformMenuItem::activated,
                nativeMenuItem->action(), [nativeMenuItem, parentMenu](){
            nativeMenuItem->action()->trigger(parentMenu);
        });
        break;
    case Type::SubMenu:
        nativeMenuItem->m_handle->setMenu(QQuickMenuPrivate::get(
            nativeMenuItem->subMenu())->handle.get());
        break;
    case Type::MenuItem:
    case Type::Separator:
        break;
    case Type::Unknown:
        Q_UNREACHABLE();
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
    qCDebug(lcNativeMenuItem) << "sync called on" << debugText() << "handle" << m_handle.get();
    Q_ASSERT(m_type != Type::Unknown);

    const auto *action = this->action();
    const auto *separator = this->separator();
    auto *subMenu = this->subMenu();
    auto *menuItem = qobject_cast<QQuickMenuItem *>(m_nonNativeItem);

    m_handle->setEnabled(action ? action->isEnabled()
        : subMenu ? subMenu->isEnabled()
        : menuItem && menuItem->isEnabled());
//    m_handle->setVisible(isVisible());
    m_handle->setIsSeparator(separator != nullptr);
    m_handle->setCheckable(action ? action->isCheckable() : menuItem && menuItem->isCheckable());
    m_handle->setChecked(action ? action->isChecked() : menuItem && menuItem->isChecked());
    m_handle->setRole(QPlatformMenuItem::TextHeuristicRole);
    m_handle->setText(action ? action->text()
        : subMenu ? subMenu->title()
        : menuItem ? menuItem->text() : QString());

//    m_handle->setFont(m_font);
//    m_handle->setHasExclusiveGroup(m_group && m_group->isExclusive());
    m_handle->setHasExclusiveGroup(false);

    if (m_iconLoader)
        m_handle->setIcon(m_iconLoader->toQIcon());

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
}

void QQuickNativeMenuItem::reset()
{
    qCDebug(lcNativeMenuItem) << "reset called on" << debugText();
    m_parentMenu = nullptr;
    m_iconLoader = nullptr;
    m_handle->setMenu(nullptr);
    m_handle.reset();
    m_shortcutId = -1;
    m_type = Type::Unknown;
}

QQuickNativeIconLoader *QQuickNativeMenuItem::iconLoader() const
{
    if (!m_iconLoader) {
        QQuickNativeMenuItem *that = const_cast<QQuickNativeMenuItem *>(this);
        static int slot = staticMetaObject.indexOfSlot("updateIcon()");
        m_iconLoader = new QQuickNativeIconLoader(slot, that);
//        m_iconLoader->setEnabled(m_complete);
    }
    return m_iconLoader;
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
    if (const auto *action = this->action())
        return action->text().isEmpty() ? QStringLiteral("(No action text)") : action->text();

    if (const auto *subMenu = this->subMenu())
        return subMenu->title().isEmpty() ? QStringLiteral("(No menu title)") : subMenu->title();

    return QStringLiteral("(Unknown)");
}

QT_END_NAMESPACE

#include "moc_qquicknativemenuitem_p.cpp"
