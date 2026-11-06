// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquicklabsplatformmenu_p.h"
#include "qquicklabsplatformmenubar_p.h"
#include "qquicklabsplatformmenuitem_p.h"
#include "qquicklabsplatformiconloader_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qicon.h>
#include <QtGui/qcursor.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qv4scopedvalue_p.h>
#include <QtQml/private/qv4qobjectwrapper_p.h>
#include <QtQuick/qquickrendercontrol.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>

#include "widgets/qwidgetplatform_p.h"

#if QT_CONFIG(systemtrayicon)
#include "qquicklabsplatformsystemtrayicon_p.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype Menu
    \inherits QtObject
//! \nativetype QQuickLabsPlatformMenu
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native menu.

    The Menu type provides a QML API for native platform menu popups.

    \image {qtlabsplatform-menu.png} {A native popup menu}

    Menu can be used in a \l MenuBar, or as a stand-alone context menu.
    The following example shows how to open a context menu on right mouse
    click:

    \code
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: zoomMenu.open()
    }

    Menu {
        id: zoomMenu

        MenuItem {
            text: qsTr("Zoom In")
            shortcut: StandardKey.ZoomIn
            onTriggered: zoomIn()
        }

        MenuItem {
            text: qsTr("Zoom Out")
            shortcut: StandardKey.ZoomOut
            onTriggered: zoomOut()
        }
    }
    \endcode

    \section2 Submenus

    To create submenus, declare a Menu as a child of another Menu:

    \qml
    Menu {
        title: qsTr("Edit")

        Menu {
            title: qsTr("Advanced")

            MenuItem {
                text: qsTr("Auto-indent Selection")
                onTriggered: autoIndentSelection()
            }

            MenuItem {
                text: qsTr("Rewrap Paragraph")
                onTriggered: rewrapParagraph()
            }
        }
    }
    \endqml

    \section2 Dynamically generating menu items

    You can dynamically generate menu items with \l Instantiator. The following
    code shows how you can implement a "Recent Files" submenu, where the items
    come from a list of files stored in settings:

    \snippet qtlabsplatform-menu-instantiator.qml menu

    \section2 Availability

    A native platform menu is currently available on the following platforms:

    \list
    \li macOS
    \li iOS
    \li Android
    \li Linux (only available as a stand-alone context menu when running with the GTK+ platform theme)
    \endlist

    \input includes/widgets.qdocinc 1

    \labs

    \sa MenuItem, MenuSeparator, MenuBar
*/

/*!
    \qmlsignal Qt.labs.platform::Menu::aboutToShow()

    This signal is emitted when the menu is about to be shown to the user.
*/

/*!
    \qmlsignal Qt.labs.platform::Menu::aboutToHide()

    This signal is emitted when the menu is about to be hidden from the user.
*/

Q_LOGGING_CATEGORY(qtLabsPlatformMenus, "qt.labs.platform.menus")

QQuickLabsPlatformMenu::QQuickLabsPlatformMenu(QObject *parent)
    : QObject(parent),
      m_complete(false),
      m_enabled(true),
      m_visible(true),
      m_minimumWidth(-1),
      m_type(QPlatformMenu::DefaultMenu),
      m_menuBar(nullptr),
      m_parentMenu(nullptr),
      m_systemTrayIcon(nullptr),
      m_menuItem(nullptr),
      m_iconLoader(nullptr),
      m_handle(nullptr)
{
}

QQuickLabsPlatformMenu::~QQuickLabsPlatformMenu()
{
    if (m_menuBar)
        m_menuBar->removeMenu(this);
    if (m_parentMenu)
        m_parentMenu->removeMenu(this);

    unparentSubmenus();

    delete m_iconLoader;
    m_iconLoader = nullptr;
    delete m_handle;
    m_handle = nullptr;
}

void QQuickLabsPlatformMenu::unparentSubmenus()
{
    for (QQuickLabsPlatformMenuItem *item : std::as_const(m_items)) {
        if (QQuickLabsPlatformMenu *subMenu = item->subMenu())
            subMenu->setParentMenu(nullptr);
        item->setMenu(nullptr);
    }
}

QPlatformMenu *QQuickLabsPlatformMenu::handle() const
{
    return m_handle;
}

QPlatformMenu * QQuickLabsPlatformMenu::create()
{
    if (!m_handle) {
        if (m_menuBar && m_menuBar->handle())
            m_handle = m_menuBar->handle()->createMenu();
        else if (m_parentMenu && m_parentMenu->handle())
            m_handle = m_parentMenu->handle()->createSubMenu();
#if QT_CONFIG(systemtrayicon)
        else if (m_systemTrayIcon && m_systemTrayIcon->handle())
            m_handle = m_systemTrayIcon->handle()->createMenu();
#endif

        // TODO: implement ^
        // - QCocoaMenuBar::createMenu()
        // - QCocoaMenu::createSubMenu()
        // - QCocoaSystemTrayIcon::createMenu()
        if (!m_handle)
            m_handle = QGuiApplicationPrivate::platformTheme()->createPlatformMenu();

        if (!m_handle)
            m_handle = QWidgetPlatform::createMenu();

        qCDebug(qtLabsPlatformMenus) << "Menu ->" << m_handle;

        if (m_handle) {
            connect(m_handle, &QPlatformMenu::aboutToShow, this, &QQuickLabsPlatformMenu::aboutToShow);
            connect(m_handle, &QPlatformMenu::aboutToHide, this, &QQuickLabsPlatformMenu::aboutToHide);

            for (QQuickLabsPlatformMenuItem *item : std::as_const(m_items))
                m_handle->insertMenuItem(item->create(), nullptr);

            if (m_menuItem) {
                if (QPlatformMenuItem *handle = m_menuItem->create())
                    handle->setMenu(m_handle);
            }
        }
    }
    return m_handle;
}

void QQuickLabsPlatformMenu::destroy()
{
    if (!m_handle)
        return;

    // Ensure that all submenus are unparented before we are destroyed,
    // so that they don't try to access a destroyed menu.
    unparentSubmenus();

    delete m_handle;
    m_handle = nullptr;
}

void QQuickLabsPlatformMenu::sync()
{
    if (!m_complete || !create())
        return;

    m_handle->setText(m_title);
    m_handle->setEnabled(m_enabled);
    m_handle->setVisible(m_visible);
    m_handle->setMinimumWidth(m_minimumWidth);
    m_handle->setMenuType(m_type);
    m_handle->setFont(m_font);

    if (m_menuBar && m_menuBar->handle())
        m_menuBar->handle()->syncMenu(m_handle);
#if QT_CONFIG(systemtrayicon)
    else if (m_systemTrayIcon && m_systemTrayIcon->handle())
        m_systemTrayIcon->handle()->updateMenu(m_handle);
#endif

    for (QQuickLabsPlatformMenuItem *item : std::as_const(m_items))
        item->sync();
}

/*!
    \qmldefault
    \qmlproperty list<QtObject> Qt.labs.platform::Menu::data

    This default property holds the list of all objects declared as children of
    the menu. The data property includes objects that are not \l MenuItem instances,
    such as \l Timer and \l QtObject.

    \sa items
*/
QQmlListProperty<QObject> QQuickLabsPlatformMenu::data()
{
    return QQmlListProperty<QObject>(this, nullptr, data_append, data_count, data_at, data_clear);
}

/*!
    \qmlproperty list<MenuItem> Qt.labs.platform::Menu::items

    This property holds the list of items in the menu.
*/
QQmlListProperty<QQuickLabsPlatformMenuItem> QQuickLabsPlatformMenu::items()
{
    return QQmlListProperty<QQuickLabsPlatformMenuItem>(this, nullptr, items_append, items_count, items_at, items_clear);
}

/*!
    \readonly
    \qmlproperty MenuBar Qt.labs.platform::Menu::menuBar

    This property holds the menubar that the menu belongs to, or \c null if the
    menu is not in a menubar.
*/
QQuickLabsPlatformMenuBar *QQuickLabsPlatformMenu::menuBar() const
{
    return m_menuBar;
}

void QQuickLabsPlatformMenu::setMenuBar(QQuickLabsPlatformMenuBar *menuBar)
{
    if (m_menuBar == menuBar)
        return;

    m_menuBar = menuBar;
    destroy();
    emit menuBarChanged();
}

/*!
    \readonly
    \qmlproperty Menu Qt.labs.platform::Menu::parentMenu

    This property holds the parent menu that the menu belongs to, or \c null if the
    menu is not a sub-menu.
*/
QQuickLabsPlatformMenu *QQuickLabsPlatformMenu::parentMenu() const
{
    return m_parentMenu;
}

void QQuickLabsPlatformMenu::setParentMenu(QQuickLabsPlatformMenu *menu)
{
    if (m_parentMenu == menu)
        return;

    m_parentMenu = menu;
    destroy();
    emit parentMenuChanged();
}

#if QT_CONFIG(systemtrayicon)
/*!
    \readonly
    \qmlproperty SystemTrayIcon Qt.labs.platform::Menu::systemTrayIcon

    This property holds the system tray icon that the menu belongs to, or \c null
    if the menu is not in a system tray icon.
*/
QQuickLabsPlatformSystemTrayIcon *QQuickLabsPlatformMenu::systemTrayIcon() const
{
    return m_systemTrayIcon;
}

void QQuickLabsPlatformMenu::setSystemTrayIcon(QQuickLabsPlatformSystemTrayIcon *icon)
{
    if (m_systemTrayIcon == icon)
        return;

    m_systemTrayIcon = icon;
    destroy();
    emit systemTrayIconChanged();
}
#endif

/*!
    \readonly
    \qmlproperty MenuItem Qt.labs.platform::Menu::menuItem

    This property holds the item that presents the menu (in a parent menu).
*/
QQuickLabsPlatformMenuItem *QQuickLabsPlatformMenu::menuItem() const
{
    if (!m_menuItem) {
        QQuickLabsPlatformMenu *that = const_cast<QQuickLabsPlatformMenu *>(this);
        m_menuItem = new QQuickLabsPlatformMenuItem(that);
        m_menuItem->setSubMenu(that);
        m_menuItem->setText(m_title);
        m_menuItem->setIcon(icon());
        m_menuItem->setVisible(m_visible);
        m_menuItem->setEnabled(m_enabled);
        m_menuItem->componentComplete();
    }
    return m_menuItem;
}

/*!
    \qmlproperty bool Qt.labs.platform::Menu::enabled

    This property holds whether the menu is enabled. The default value is \c true.
*/
bool QQuickLabsPlatformMenu::isEnabled() const
{
    return m_enabled;
}

void QQuickLabsPlatformMenu::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    if (m_menuItem)
        m_menuItem->setEnabled(enabled);

    m_enabled = enabled;
    sync();
    emit enabledChanged();
}

/*!
    \qmlproperty bool Qt.labs.platform::Menu::visible

    This property holds whether the menu is visible. The default value is \c true.
*/
bool QQuickLabsPlatformMenu::isVisible() const
{
    return m_visible;
}

void QQuickLabsPlatformMenu::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    if (m_menuItem)
        m_menuItem->setVisible(visible);

    m_visible = visible;
    sync();
    emit visibleChanged();
}

/*!
    \qmlproperty int Qt.labs.platform::Menu::minimumWidth

    This property holds the minimum width of the menu. The default value is \c -1 (no minimum width).
*/
int QQuickLabsPlatformMenu::minimumWidth() const
{
    return m_minimumWidth;
}

void QQuickLabsPlatformMenu::setMinimumWidth(int width)
{
    if (m_minimumWidth == width)
        return;

    m_minimumWidth = width;
    sync();
    emit minimumWidthChanged();
}

/*!
    \qmlproperty enumeration Qt.labs.platform::Menu::type

    This property holds the type of the menu.

    Available values:
    \value Menu.DefaultMenu A normal menu (default).
    \value Menu.EditMenu An edit menu with pre-populated cut, copy and paste items.
*/
QPlatformMenu::MenuType QQuickLabsPlatformMenu::type() const
{
    return m_type;
}

void QQuickLabsPlatformMenu::setType(QPlatformMenu::MenuType type)
{
    if (m_type == type)
        return;

    m_type = type;
    sync();
    emit typeChanged();
}

/*!
    \qmlproperty string Qt.labs.platform::Menu::title

    This property holds the menu's title.
*/
QString QQuickLabsPlatformMenu::title() const
{
    return m_title;
}

void QQuickLabsPlatformMenu::setTitle(const QString &title)
{
    if (m_title == title)
        return;

    if (m_menuItem)
        m_menuItem->setText(title);

    m_title = title;
    sync();
    emit titleChanged();
}

/*!
    \qmlproperty font Qt.labs.platform::Menu::font

    This property holds the menu's font.

    \sa title
*/
QFont QQuickLabsPlatformMenu::font() const
{
    return m_font;
}

void QQuickLabsPlatformMenu::setFont(const QFont& font)
{
    if (m_font == font)
        return;

    m_font = font;
    sync();
    emit fontChanged();
}

/*!
    \since Qt.labs.platform 1.1 (Qt 5.12)
    \qmlproperty url Qt.labs.platform::Menu::icon.source
    \qmlproperty string Qt.labs.platform::Menu::icon.name
    \qmlproperty bool Qt.labs.platform::Menu::icon.mask

    This property holds the menu item's icon.
*/
QQuickLabsPlatformIcon QQuickLabsPlatformMenu::icon() const
{
    if (!m_iconLoader)
        return QQuickLabsPlatformIcon();

    return iconLoader()->icon();
}

void QQuickLabsPlatformMenu::setIcon(const QQuickLabsPlatformIcon &icon)
{
    if (iconLoader()->icon() == icon)
        return;

    if (m_menuItem)
        m_menuItem->setIcon(icon);

    iconLoader()->setIcon(icon);
    emit iconChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::addItem(MenuItem item)

    Adds an \a item to the end of the menu.
*/
void QQuickLabsPlatformMenu::addItem(QQuickLabsPlatformMenuItem *item)
{
    insertItem(m_items.size(), item);
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::insertItem(int index, MenuItem item)

    Inserts an \a item at the specified \a index in the menu.
*/
void QQuickLabsPlatformMenu::insertItem(int index, QQuickLabsPlatformMenuItem *item)
{
    if (!item || m_items.contains(item))
        return;

    m_items.insert(index, item);
    m_data.append(item);
    item->setMenu(this);
    if (m_handle && item->create()) {
        QQuickLabsPlatformMenuItem *before = m_items.value(index + 1);
        m_handle->insertMenuItem(item->handle(), before ? before->create() : nullptr);
    }
    sync();
    emit itemsChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::removeItem(MenuItem item)

    Removes an \a item from the menu.
*/
void QQuickLabsPlatformMenu::removeItem(QQuickLabsPlatformMenuItem *item)
{
    if (!item || !m_items.removeOne(item))
        return;

    m_data.removeOne(item);
    if (m_handle)
        m_handle->removeMenuItem(item->handle());
    item->setMenu(nullptr);
    sync();
    emit itemsChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::addMenu(Menu submenu)

    Adds a \a submenu to the end of the menu.
*/
void QQuickLabsPlatformMenu::addMenu(QQuickLabsPlatformMenu *menu)
{
    insertMenu(m_items.size(), menu);
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::insertMenu(int index, Menu submenu)

    Inserts a \a submenu at the specified \a index in the menu.
*/
void QQuickLabsPlatformMenu::insertMenu(int index, QQuickLabsPlatformMenu *menu)
{
    if (!menu)
        return;

    menu->setParentMenu(this);
    insertItem(index, menu->menuItem());
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::removeMenu(Menu submenu)

    Removes a \a submenu from the menu.
*/
void QQuickLabsPlatformMenu::removeMenu(QQuickLabsPlatformMenu *menu)
{
    if (!menu)
        return;

    menu->setParentMenu(nullptr);
    removeItem(menu->menuItem());
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::clear()

    Removes all items from the menu.
*/
void QQuickLabsPlatformMenu::clear()
{
    if (m_items.isEmpty())
        return;

    for (QQuickLabsPlatformMenuItem *item : std::as_const(m_items)) {
        m_data.removeOne(item);
        if (m_handle)
            m_handle->removeMenuItem(item->handle());
        item->setMenu(nullptr);
        delete item;
    }

    m_items.clear();
    sync();
    emit itemsChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::open(MenuItem item)

    Opens the menu at the current mouse position, optionally aligned to a menu \a item.
*/

/*!
    \qmlmethod void Qt.labs.platform::Menu::open(Item target, MenuItem item)

    Opens the menu at the specified \a target item, optionally aligned to a menu \a item.
*/
void QQuickLabsPlatformMenu::open(QQmlV4FunctionPtr args)
{
    if (!m_handle)
        return;

    if (args->length() > 2) {
        args->v4engine()->throwTypeError();
        return;
    }

    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    QQuickItem *targetItem = nullptr;
    if (args->length() > 0) {
        QV4::ScopedValue value(scope, (*args)[0]);
        QV4::Scoped<QV4::QObjectWrapper> object(scope, value->as<QV4::QObjectWrapper>());
        if (object)
            targetItem = qobject_cast<QQuickItem *>(object->object());
    }

    QQuickLabsPlatformMenuItem *menuItem = nullptr;
    if (args->length() > 1) {
        QV4::ScopedValue value(scope, (*args)[1]);
        QV4::Scoped<QV4::QObjectWrapper> object(scope, value->as<QV4::QObjectWrapper>());
        if (object)
            menuItem = qobject_cast<QQuickLabsPlatformMenuItem *>(object->object());
    }

    QPoint offset;
    QWindow *window = findWindow(targetItem, &offset);

    QRect targetRect;
    if (targetItem) {
        QRectF sceneBounds = targetItem->mapRectToScene(targetItem->boundingRect());
        targetRect = sceneBounds.toAlignedRect().translated(offset);
    } else {
#if QT_CONFIG(cursor)
        QPoint pos = QCursor::pos();
        if (window)
            pos = window->mapFromGlobal(pos);
        targetRect.moveTo(pos);
#endif
    }
    m_handle->showPopup(window,
                        QHighDpi::toNativeLocalPosition(targetRect, window),
                        menuItem ? menuItem->handle() : nullptr);
}

/*!
    \qmlmethod void Qt.labs.platform::Menu::close()

    Closes the menu.
*/
void QQuickLabsPlatformMenu::close()
{
    if (m_handle)
        m_handle->dismiss();
}

void QQuickLabsPlatformMenu::classBegin()
{
}

void QQuickLabsPlatformMenu::componentComplete()
{
    m_complete = true;
    if (m_handle && m_iconLoader)
        m_iconLoader->setEnabled(true);
    sync();
}

QQuickLabsPlatformIconLoader *QQuickLabsPlatformMenu::iconLoader() const
{
    if (!m_iconLoader) {
        QQuickLabsPlatformMenu *that = const_cast<QQuickLabsPlatformMenu *>(this);
        static int slot = staticMetaObject.indexOfSlot("updateIcon()");
        m_iconLoader = new QQuickLabsPlatformIconLoader(slot, that);
        m_iconLoader->setEnabled(m_complete);
    }
    return m_iconLoader;
}

static QWindow *effectiveWindow(QWindow *window, QPoint *offset)
{
    QQuickWindow *quickWindow = qobject_cast<QQuickWindow *>(window);
    if (quickWindow) {
        QWindow *renderWindow = QQuickRenderControl::renderWindowFor(quickWindow, offset);
        if (renderWindow)
            return renderWindow;
    }
    return window;
}

QWindow *QQuickLabsPlatformMenu::findWindow(QQuickItem *target, QPoint *offset) const
{
    if (target)
        return effectiveWindow(target->window(), offset);

    if (m_menuBar && m_menuBar->window())
        return effectiveWindow(m_menuBar->window(), offset);

    QObject *obj = parent();
    while (obj) {
        QWindow *window = qobject_cast<QWindow *>(obj);
        if (window)
            return effectiveWindow(window, offset);

        QQuickItem *item = qobject_cast<QQuickItem *>(obj);
        if (item && item->window())
            return effectiveWindow(item->window(), offset);

        obj = obj->parent();
    }
    return nullptr;
}

void QQuickLabsPlatformMenu::data_append(QQmlListProperty<QObject> *property, QObject *object)
{
    QQuickLabsPlatformMenu *menu = static_cast<QQuickLabsPlatformMenu *>(property->object);
    if (QQuickLabsPlatformMenuItem *item = qobject_cast<QQuickLabsPlatformMenuItem *>(object))
        menu->addItem(item);
    else if (QQuickLabsPlatformMenu *subMenu = qobject_cast<QQuickLabsPlatformMenu *>(object))
        menu->addMenu(subMenu);
    else
        menu->m_data.append(object);
}

qsizetype QQuickLabsPlatformMenu::data_count(QQmlListProperty<QObject> *property)
{
    QQuickLabsPlatformMenu *menu = static_cast<QQuickLabsPlatformMenu *>(property->object);
    return menu->m_data.size();
}

QObject *QQuickLabsPlatformMenu::data_at(QQmlListProperty<QObject> *property, qsizetype index)
{
    QQuickLabsPlatformMenu *menu = static_cast<QQuickLabsPlatformMenu *>(property->object);
    return menu->m_data.value(index);
}

void QQuickLabsPlatformMenu::data_clear(QQmlListProperty<QObject> *property)
{
    QQuickLabsPlatformMenu *menu = static_cast<QQuickLabsPlatformMenu *>(property->object);
    menu->m_data.clear();
}

void QQuickLabsPlatformMenu::items_append(QQmlListProperty<QQuickLabsPlatformMenuItem> *property, QQuickLabsPlatformMenuItem *item)
{
    QQuickLabsPlatformMenu *menu = static_cast<QQuickLabsPlatformMenu *>(property->object);
    menu->addItem(item);
}

qsizetype QQuickLabsPlatformMenu::items_count(QQmlListProperty<QQuickLabsPlatformMenuItem> *property)
{
    QQuickLabsPlatformMenu *menu = static_cast<QQuickLabsPlatformMenu *>(property->object);
    return menu->m_items.size();
}

QQuickLabsPlatformMenuItem *QQuickLabsPlatformMenu::items_at(QQmlListProperty<QQuickLabsPlatformMenuItem> *property, qsizetype index)
{
    QQuickLabsPlatformMenu *menu = static_cast<QQuickLabsPlatformMenu *>(property->object);
    return menu->m_items.value(index);
}

void QQuickLabsPlatformMenu::items_clear(QQmlListProperty<QQuickLabsPlatformMenuItem> *property)
{
    QQuickLabsPlatformMenu *menu = static_cast<QQuickLabsPlatformMenu *>(property->object);
    menu->clear();
}

void QQuickLabsPlatformMenu::updateIcon()
{
    if (!m_handle || !m_iconLoader)
        return;

    m_handle->setIcon(m_iconLoader->toQIcon());
    sync();
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformmenu_p.cpp"
