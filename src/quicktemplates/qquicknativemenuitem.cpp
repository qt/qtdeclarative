// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicknativemenuitem_p.h"

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
#include <QtQuickTemplates2/private/qquicknativeiconloader_p.h>
#include <QtQuickTemplates2/private/qquickshortcutcontext_p_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcNativeMenuItem, "qt.quick.controls.nativemenuitem")

/*!
    \class QQuickNativeMenuItem
    \brief A native menu item.
    \since 6.7
    \internal

    Provides a way to sync between the properties and signals of action
    and the underlying native menu item.

    \sa Menu, Action
*/

/*!
    \internal

    Adds \a action as a menu item of \a parentMenu.
*/
QQuickNativeMenuItem::QQuickNativeMenuItem(QQuickMenu *parentMenu, QQuickAction *action)
    : QObject(parentMenu)
    , m_parentMenu(parentMenu)
    , m_action(action)
{
}

/*!
    \internal

    Adds \subMenu as a sub-menu menu item of \a parentMenu.
*/
QQuickNativeMenuItem::QQuickNativeMenuItem(QQuickMenu *parentMenu, QQuickMenu *subMenu)
    : QObject(parentMenu)
    , m_parentMenu(parentMenu)
    , m_subMenu(subMenu)
{
}

QQuickNativeMenuItem::~QQuickNativeMenuItem()
{
    qCDebug(lcNativeMenuItem) << "destroying" << this << debugText();
}

QQuickAction *QQuickNativeMenuItem::action() const
{
    return m_action;
}

QQuickMenu *QQuickNativeMenuItem::subMenu() const
{
    return m_subMenu;
}

void QQuickNativeMenuItem::clearSubMenu()
{
    m_subMenu = nullptr;
    m_handle->setMenu(nullptr);
}

QPlatformMenuItem *QQuickNativeMenuItem::handle() const
{
    return m_handle.get();
}

QPlatformMenuItem *QQuickNativeMenuItem::create()
{
    qCDebug(lcNativeMenuItem) << "create called on" << debugText() << "m_handle" << m_handle.get();
    if (m_handle)
        return m_handle.get();

    auto *parentMenuPrivate = QQuickMenuPrivate::get(m_parentMenu);
    m_handle.reset(parentMenuPrivate->nativeHandle->createMenuItem());

    if (!m_handle)
        m_handle.reset(QGuiApplicationPrivate::platformTheme()->createPlatformMenuItem());

    Q_ASSERT(m_action || m_subMenu);

    if (m_handle) {
        if (m_action) {
            connect(m_handle.get(), &QPlatformMenuItem::activated, m_action, [this](){
                m_action->trigger(m_parentMenu);
            });
        } else { // m_subMenu
//            m_handle->setMenu(parentMenuPrivate->nativeHandle.get());
            m_handle->setMenu(QQuickMenuPrivate::get(m_subMenu)->nativeHandle.get());
            // TODO: do we need to call anything here? need to at least ensure
            // that the QQuickMenu::isVisible returns true after this
//            connect(m_handle.get(), &QPlatformMenuItem::activated, m_subMenu, &QQuickMenu::?
        }
    }

    return m_handle.get();
}

void QQuickNativeMenuItem::sync()
{
    qCDebug(lcNativeMenuItem) << "sync called on" << debugText() << "handle" << m_handle.get();
    if (/* !m_complete || */!create())
        return;

    Q_ASSERT(m_action || m_subMenu);

    m_handle->setEnabled(m_action ? m_action->isEnabled() : m_subMenu->isEnabled());
//    m_handle->setVisible(isVisible());
//    m_handle->setIsSeparator(m_separator);
    m_handle->setCheckable(m_action && m_action->isCheckable());
    m_handle->setChecked(m_action && m_action->isChecked());
    m_handle->setRole(QPlatformMenuItem::TextHeuristicRole);
    m_handle->setText(m_action ? m_action->text() : m_subMenu->title());

//    m_handle->setFont(m_font);
//    m_handle->setHasExclusiveGroup(m_group && m_group->isExclusive());
    m_handle->setHasExclusiveGroup(false);

    if (m_iconLoader)
        m_handle->setIcon(m_iconLoader->toQIcon());

    if (m_subMenu) {
        // Sync first as dynamically created menus may need to get the handle recreated.
        auto *subMenuPrivate = QQuickMenuPrivate::get(m_subMenu);
        subMenuPrivate->syncWithNativeMenu();
        if (subMenuPrivate->nativeHandle)
            m_handle->setMenu(subMenuPrivate->nativeHandle.get());
    }

#if QT_CONFIG(shortcut)
    if (m_action)
        m_handle->setShortcut(m_action->shortcut());
#endif

    if (m_parentMenu) {
        auto *menuPrivate = QQuickMenuPrivate::get(m_parentMenu);
        if (menuPrivate->nativeHandle)
            menuPrivate->nativeHandle->syncMenuItem(m_handle.get());
    }
}

void QQuickNativeMenuItem::reset()
{
    m_parentMenu = nullptr;
    m_subMenu = nullptr;
    m_action = nullptr;
    m_iconLoader = nullptr;
    m_handle->setMenu(nullptr);
    m_handle.reset();
    m_shortcutId = -1;
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
    const QKeySequence sequence = m_action->shortcut();
    if (!sequence.isEmpty() && m_action->isEnabled()) {
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

    const QKeySequence sequence = m_action->shortcut();
    QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(m_shortcutId, this, sequence);
#endif
}

QString QQuickNativeMenuItem::debugText() const
{
    return m_action ? m_action->text() : (m_subMenu ? m_subMenu->title() : QStringLiteral("(No text)"));
}

QT_END_NAMESPACE

#include "moc_qquicknativemenuitem_p.cpp"
