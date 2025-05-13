// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmenu_p.h"
#include "qquickmenu_p_p.h"
#include "qquickmenuitem_p_p.h"
#include <private/qtquicktemplates2-config_p.h>
#if QT_CONFIG(quicktemplates2_container)
#include "qquickmenubaritem_p.h"
#include "qquickmenubar_p_p.h"
#endif
#include "qquickmenuseparator_p.h"
#include "qquicknativemenuitem_p.h"
#include "qquickpopupitem_p_p.h"
#include "qquickpopuppositioner_p_p.h"
#include "qquickaction_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qevent.h>
#include <QtGui/qcursor.h>
#if QT_CONFIG(shortcut)
#include <QtGui/qkeysequence.h>
#endif
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qv4scopedvalue_p.h>
#include <QtQml/private/qv4variantobject_p.h>
#include <QtQml/private/qv4qobjectwrapper_p.h>
#include <private/qqmlobjectmodel_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquickrendercontrol_p.h>
#include <QtQuick/private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcMenu, "qt.quick.controls.menu")
Q_STATIC_LOGGING_CATEGORY(lcNativeMenus, "qt.quick.controls.nativemenus")

// copied from qfusionstyle.cpp
static const int SUBMENU_DELAY = 225;

/*!
    \qmltype Menu
    \inherits Popup
//!     \nativetype QQuickMenu
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-menus
    \ingroup qtquickcontrols-popups
    \brief Menu popup that can be used as a context menu or popup menu.

    \table
        \row
          \li \image qtquickcontrols-menu-native.png
             \caption Native macOS menu.
          \li \image qtquickcontrols-menu.png
             \caption Non-native \l {Material Style}{Material style} menu.
   \endtable

    Menu has two main use cases:
    \list
        \li Context menus; for example, a menu that is shown after right clicking
        \li Popup menus; for example, a menu that is shown after clicking a button
    \endlist

    When used as a context menu, the recommended way of opening the menu is to call
    \l popup(). Unless a position is explicitly specified, the menu is positioned at
    the mouse cursor on desktop platforms that have a mouse cursor available, and
    otherwise centered over its parent item.

    \snippet qtquickcontrols-menu-contextmenu.qml root

    When used as a popup menu, it is easiest to specify the position by specifying
    the desired \l {Popup::}{x} and \l {Popup::}{y} coordinates using the respective
    properties, and call \l {Popup::}{open()} to open the menu.

    \snippet qtquickcontrols-menu-button-menu.qml root

    If the button should also close the menu when clicked, use the
    \c Popup.CloseOnPressOutsideParent flag:

    \snippet qtquickcontrols-menu-closepolicy.qml closePolicy

    Since QtQuick.Controls 2.3 (Qt 5.10), it is also possible to create sub-menus
    and declare Action objects inside Menu:

    \snippet qtquickcontrols-menu-submenus-and-actions.qml root

    Sub-menus are \l {cascade}{cascading} by default on desktop platforms
    that have a mouse cursor available. Non-cascading menus are shown one
    menu at a time, and centered over the parent menu.

    Typically, menu items are statically declared as children of the menu, but
    Menu also provides API to \l {addItem}{add}, \l {insertItem}{insert},
    \l {moveItem}{move} and \l {removeItem}{remove} items dynamically. The
    items in a menu can be accessed using \l itemAt() or
    \l {Popup::}{contentChildren}.

    Although \l {MenuItem}{MenuItems} are most commonly used with Menu, it can
    contain any type of item.

    \section1 Margins

    As it is inherited from Popup, Menu supports \l {Popup::}{margins}. By
    default, all of the built-in styles specify \c 0 for Menu's margins to
    ensure that the menu is kept within the bounds of the window. To allow a
    menu to go outside of the window (to animate it moving into view, for
    example), set the margins property to \c -1.

    \section1 Dynamically Generating Menu Items

    You can dynamically create menu items with \l Instantiator or
    \l {Dynamic QML Object Creation from JavaScript} {dynamic object creation}.

    \section2 Using Instantiator

    You can dynamically generate menu items with \l Instantiator. The
    following code shows how you can implement a "Recent Files" submenu,
    where the items come from a list of files stored in settings:

    \snippet qtquickcontrols-menu-instantiator.qml menu

    \section2 Using Dynamic Object Creation

    You can also dynamically load a component from a QML file using
    \l {QtQml::Qt::createComponent()} {Qt.createComponent()}. Once the component
    is ready, you can call its \l {Component::createObject()} {createObject()}
    method to create an instance of that component.

    \snippet qtquickcontrols-menu-createObject.qml createObject

    \sa {Customizing Menu}, MenuItem, {Menu Controls}, {Popup Controls},
        {Dynamic QML Object Creation from JavaScript}

    \section1 Menu types

    Since Qt 6.8, a menu offers three different implementations, depending on the
    platform. You can choose which one should be preferred by setting
    \l [QML] {Popup::} {popupType}. This will let you control if a menu should
    be shown as a separate window, as an item inside the parent window, or as a
    native menu. You can read more about these options \l{Popup type}{here}.

    The default \l [QML] {Popup::}{popupType} is decided by the style. The \l {macOS Style}, for example,
    sets it to be \c Popup.Native, while the \l{Imagine Style} uses \c Popup.Window (which
    is the default when the style doesn't set a popup type).
    If you add customizations to a menu, and want those to be used regardless of the
    style, you should set the popup type to be \c Popup.Window (or \c Popup.Item) explicitly.
    Another alternative is to set the \c Qt::AA_DontUseNativeMenuWindows
    \l {Qt::ApplicationAttribute}{application attribute}. This will disable native context
    menus for the whole application, irrespective of the style.

    Whether a menu will be able to use the preferred type depends on the platform.
    \c Popup.Item is supported on all platforms, but \c Popup.Window is
    normally only supported on desktop platforms. Additionally, if the menu is inside
    a \l {Native menu bars}{native menubar}, the menu will be native as well. And if
    the menu is a sub-menu inside another menu, the parent (or root) menu will decide the type.

    \section2 Limitations when using native menus

    When setting \l [QML] {Popup::} {popupType} to \c Popup.Native
    there are some limitations and differences compared to using \c Popup.Item
    and \c Popup.Window.

    \section3 API differences

    When using native menus, only a subset of the Menu API is supported on all platforms:

    \list
    \li \l {Popup::}{x}
    \li \l {Popup::}{y}
    \li \l {Popup::}{visible}
    \li \l {Popup::}{opened}
    \li \l title
    \li \l count
    \li \l {Popup::}{contentData}
    \li \l {Popup::}{contentChildren} (visual children will not be visible)
    \li \l contentModel
    \li \l {Popup::}{open()}
    \li \l popup()
    \li \l {Popup::}{close()}
    \li \l {Popup::}{opened()}
    \li \l {Popup::}{closed()}
    \li \l {Popup::}{aboutToShow()}
    \li \l {Popup::}{aboutToHide()}
    \endlist

    In addition, showing a popup (using, for example, \l {Popup::}{open()} or \l
    {popup()}) will be a blocking call on some platforms. This means that the
    call will not return before the menu is closed again, which can affect the
    logic in your application. This is especially important to take into
    consideration if your application is targeting multiple
    platforms, and as such, sometimes run on platforms where native menus are
    not supported. In that case the popupType will fall back to \c Popup.Item,
    for example, and calls to \l {Popup::}{open()} will not be blocking.

    Items like \l MenuItem will still react to clicks in the corresponding
    native menu item by emitting signals, for example, but will be replaced by
    their native counterpart.

    \section3 Rendering differences

    Native menus are implemented using the available native menu APIs on the platform.
    Those menus, and all of their contents, will therefore be rendered by the platform, and
    not by QML. This means that the \l delegate will \e not be used for rendering. It will,
    however, always be instantiated (but hidden), so that functions such as
    \l [QML] {QtQml::Component::completed}{onCompleted()} execute regardless of platform and
    \l [QML] {Popup::} {popupType}.

    \section3 Supported platforms

    Native menus are currently supported on the following platforms:

    \list
    \li Android
    \li iOS
    \li Linux (only available as a stand-alone context menu when running with the GTK+ platform theme)
    \li macOS
    \li Windows
    \endlist

    \sa {Popup type}, [QML] {Popup::}{popupType}
*/

/*!
    \qmlproperty bool QtQuick.Controls::Menu::focus

    This property holds whether the popup wants focus.

    When the popup actually receives focus, \l{Popup::}{activeFocus}
    will be \c true. For more information, see
    \l {Keyboard Focus in Qt Quick}.

    The default value is \c true.

    \include qquickmenu.qdocinc non-native-only-property

    \sa {Popup::}{activeFocus}
*/

static const QQuickPopup::ClosePolicy cascadingSubMenuClosePolicy = QQuickPopup::CloseOnEscape | QQuickPopup::CloseOnPressOutsideParent;

static bool shouldCascade()
{
#if QT_CONFIG(cursor)
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows);
#else
    return false;
#endif
}

class QQuickMenuPositioner : public QQuickPopupPositioner
{
public:
    QQuickMenuPositioner(QQuickMenu *menu) : QQuickPopupPositioner(menu) { }

    void reposition() override;
};

QQuickMenuPrivate::QQuickMenuPrivate()
{
    cascade = shouldCascade();
}

void QQuickMenuPrivate::init()
{
    Q_Q(QQuickMenu);
    contentModel = new QQmlObjectModel(q);
}

QQuickMenu *QQuickMenuPrivate::rootMenu() const
{
    Q_Q(const QQuickMenu);
    const QQuickMenu *rootMenu = q;
    const QObject *p = q->parent();
    while (p) {
        if (auto menu = qobject_cast<const QQuickMenu *>(p))
            rootMenu = menu;
        p = p->parent();
    }

    return const_cast<QQuickMenu *>(rootMenu);
}

QQuickPopup::PopupType QQuickMenuPrivate::resolvedPopupType() const
{
    // The root menu (which can be this menu, unless it's a
    // sub menu) decides the popup type for all sub menus.
    QQuickMenu *root = rootMenu();
    QQuickMenuPrivate *root_d = QQuickMenuPrivate::get(rootMenu());

    if (auto menuBar = QQuickMenuPrivate::get(root)->menuBar.get()) {
        // When a menu is inside a MenuBar, the MenuBar decides if the menu
        // should be native or not. The menu's popupType is therefore ignored.
        // Basically, a native MenuBar can only contain native Menus, and
        // a non-native MenuBar can only contain non-native Menus.
        if (QQuickMenuBarPrivate::get(menuBar)->useNativeMenu(q_func()))
            return QQuickPopup::Native;
    } else {
        // If the root menu is native, this menu needs to be native as well
        if (root_d->maybeNativeHandle()) {
            return QQuickPopup::Native;
        } else if (!root_d->triedToCreateNativeMenu) {
            // If popupType is Native, and native popups seems to be available, then we return
            // the resolved type to be Native. But note that this doesn't guarantee that the
            // menu will actually become native. QPA can still refuse to create a QPlaformMenu,
            // if the QPA plugin or theme doesn't support it. The only way to know for sure if
            // a menu became native, is to also check QQuickMenuPrivate::nativeHandle().
            if (root->popupType() == QQuickPopup::Native
                && !QGuiApplication::testAttribute(Qt::AA_DontUseNativeMenuWindows)) {
                return QQuickPopup::Native;
            }
        }
    }

    // Let QQuickPopup decide if the menu should be Popup.Window or Popup.Item, based
    // on e.g popuptype and platform limitations. QQuickPopupPrivate should never resolve
    // the popup type to be Native, since that's up to the individual subclasses to decide.
    const auto type = root_d->QQuickPopupPrivate::resolvedPopupType();
    Q_ASSERT(type != QQuickPopup::Native);
    return type;
}

bool QQuickMenuPrivate::useNativeMenu() const
{
    return resolvedPopupType() == QQuickPopup::Native;
}

QPlatformMenu *QQuickMenuPrivate::nativeHandle()
{
    Q_ASSERT(handle || useNativeMenu());
    if (!handle && !triedToCreateNativeMenu)
        createNativeMenu();
    return handle.get();
}

QPlatformMenu *QQuickMenuPrivate::maybeNativeHandle() const
{
    return handle.get();
}

bool QQuickMenuPrivate::createNativeMenu()
{
    Q_ASSERT(!handle);
    Q_Q(QQuickMenu);
    qCDebug(lcNativeMenus) << "createNativeMenu called on" << q;

    if (auto menuBar = QQuickMenuPrivate::get(rootMenu())->menuBar) {
        auto menuBarPrivate = QQuickMenuBarPrivate::get(menuBar);
        if (menuBarPrivate->useNativeMenuBar()) {
            qCDebug(lcNativeMenus) << "- creating native menu from native menubar";
            if (QPlatformMenuBar *menuBarHandle = menuBarPrivate->nativeHandle())
                handle.reset(menuBarHandle->createMenu());
        }
    }

    if (!handle) {
        QPlatformMenu *parentMenuHandle(parentMenu ? get(parentMenu)->handle.get() : nullptr);
        if (parentMenu && parentMenuHandle) {
            qCDebug(lcNativeMenus) << "- creating native sub-menu";
            handle.reset(parentMenuHandle->createSubMenu());
        } else {
            qCDebug(lcNativeMenus) << "- creating native menu";
            handle.reset(QGuiApplicationPrivate::platformTheme()->createPlatformMenu());
        }
    }

    triedToCreateNativeMenu = true;

    if (!handle)
        return false;

    q->connect(handle.get(), &QPlatformMenu::aboutToShow, q, [q, this](){
        emit q->aboutToShow();
        visible = true;
        emit q->visibleChanged();
        emit q->openedChanged();
        opened();
    });
    q->connect(handle.get(), &QPlatformMenu::aboutToHide, q, [q](){
        qCDebug(lcNativeMenus) << "QPlatformMenu::aboutToHide called; about to call setVisible(false) on Menu";
        emit q->aboutToHide();
    });
    // On some platforms (Windows and macOS), it can happen that QPlatformMenuItem::activated
    // is emitted after QPlatformMenu::aboutToHide. Since sending signals out of order can
    // cause an application to fail (QTBUG-128158) we use Qt::QueuedConnection to work around
    // this, so that we emit the signals in the right order.
    q->connect(handle.get(), &QPlatformMenu::aboutToHide, q, [q, this](){
        visible = false;
        emit q->visibleChanged();
        emit q->openedChanged();
        emit q->closed();
    }, Qt::QueuedConnection);

    recursivelyCreateNativeMenuItems(q);
    syncWithNativeMenu();

    return true;
}

QString nativeMenuItemListToString(const QList<QQuickNativeMenuItem *> &nativeItems)
{
    if (nativeItems.isEmpty())
        return QStringLiteral("(Empty)");

    QString str;
    QTextStream debug(&str);
    for (const auto *nativeItem : nativeItems)
        debug << nativeItem->debugText() << ", ";
    // Remove trailing space and comma.
    if (!nativeItems.isEmpty())
        str.chop(2);
    return str;
}

void QQuickMenuPrivate::syncWithNativeMenu()
{
    Q_Q(QQuickMenu);
    if (!complete || !handle)
        return;

    qCDebug(lcNativeMenus).nospace() << "syncWithNativeMenu called on " << q
        << " (complete: " << complete << " visible: " << visible << ") - "
        << "syncing " << nativeItems.size() << " item(s)...";

    // TODO: call this function when any of the variables below change

    handle->setText(title);
    handle->setEnabled(q->isEnabled());
    handle->setMinimumWidth(q->implicitWidth());
//    nativeHandle->setMenuType(m_type);
    handle->setFont(q->font());

    // Note: the QQuickMenu::visible property is used to open or close the menu.
    // This is in contrast to QPlatformMenu::visible, which tells if the menu
    // should be visible in the menubar or not (if it belongs to one). To control
    // if a QPlatformMenu should be open, we instead use QPlatformMenu::showPopup()
    // and dismiss(). As such, we don't want to call handle->setVisible(visible)
    // from this function since we always want the menu to be visible in the menubar
    // (if it belongs to one). The currently only way to hide a menu from a menubar is
    // to instead call MenuBar.removeMenu(menu).

//    if (m_menuBar && m_menuBar->handle())
//        m_menuBar->handle()->syncMenu(handle);
//#if QT_CONFIG(systemtrayicon)
//    else if (m_systemTrayIcon && m_systemTrayIcon->handle())
//        m_systemTrayIcon->handle()->updateMenu(handle);
//#endif

    for (QQuickNativeMenuItem *item : std::as_const(nativeItems)) {
        qCDebug(lcNativeMenus) << "- syncing" << item << "action" << item->action()
            << "sub-menu" << item->subMenu() << item->debugText();
        item->sync();
    }

    qCDebug(lcNativeMenus) << "... finished syncing" << q;
}

void QQuickMenuPrivate::removeNativeMenu()
{
    // Remove the native menu, including it's native menu items
    Q_Q(QQuickMenu);
    const int qtyItemsToRemove = nativeItems.size();
    if (qtyItemsToRemove != 0)
        Q_ASSERT(q->count() == qtyItemsToRemove);
    for (int i = 0; i < qtyItemsToRemove; ++i)
        removeNativeItem(0);
    Q_ASSERT(nativeItems.isEmpty());

    // removeNativeItem will take care of destroying sub-menus and resetting their native data,
    // but as the root menu, we have to take care of our own.
    resetNativeData();
}

void QQuickMenuPrivate::syncWithUseNativeMenu()
{
    Q_Q(QQuickMenu);
    // Users can change AA_DontUseNativeMenuWindows while a menu is visible,
    // but the changes won't take affect until the menu is re-opened.
    if (q->isVisible() || parentMenu)
        return;

    if (maybeNativeHandle() && !useNativeMenu()) {
        // Switch to a non-native menu by removing the native menu and its native items.
        // Note that there's nothing to do if a native menu was requested but we failed to create it.
        removeNativeMenu();
    } else if (useNativeMenu()) {
        Q_ASSERT(nativeItems.isEmpty());
        // Try to create a native menu.
        nativeHandle();
    }
}

/*!
    \internal

    Recursively destroys native sub-menus of \a menu.

    This function checks if each native item in \c menu has a sub-menu,
    and if so:
    \list
    \li Calls itself with that sub-menu
    \li Resets the item's data (important to avoid accessing a deleted QQuickAction
        when printing in QQuickNativeMenuItem's destructor)
    \li Deletes (eventually) the native item
    \endlist

    Similar (besides the recursion) to removeNativeItem(), except that
    we can avoid repeated calls to syncWithNativeMenu().
*/
void QQuickMenuPrivate::recursivelyDestroyNativeSubMenus(QQuickMenu *menu)
{
    auto *menuPrivate = QQuickMenuPrivate::get(menu);
    Q_ASSERT(menuPrivate->handle);
    qCDebug(lcNativeMenus) << "recursivelyDestroyNativeSubMenus called with" << menu << "...";

    while (!menuPrivate->nativeItems.isEmpty()) {
        std::unique_ptr<QQuickNativeMenuItem> item(menuPrivate->nativeItems.takeFirst());
        qCDebug(lcNativeMenus) << "- taking and destroying" << item->debugText();
        if (QQuickMenu *subMenu = item->subMenu())
            recursivelyDestroyNativeSubMenus(subMenu);

        if (item->handle())
            menuPrivate->handle->removeMenuItem(item->handle());
    }

    menuPrivate->resetNativeData();

    qCDebug(lcNativeMenus) << "... finished destroying native sub-menus of" << menu;
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

void QQuickMenuPrivate::setNativeMenuVisible(bool visible)
{
    Q_Q(QQuickMenu);
    qCDebug(lcNativeMenus) << "setNativeMenuVisible called with visible" << visible;
    if (visible)
        emit q->aboutToShow();
    else
        emit q->aboutToHide();

    this->visible = visible;
    syncWithNativeMenu();

    QPoint offset;
    QWindow *window = effectiveWindow(qGuiApp->topLevelWindows().first(), &offset);

    if (visible) {
        lastDevicePixelRatio = window->devicePixelRatio();

        const QPointF globalPos = parentItem->mapToGlobal(x, y);
        const QPoint windowPos = window->mapFromGlobal(globalPos.toPoint());
        QRect targetRect(windowPos, QSize(0, 0));
        auto *daPriv = QQuickItemPrivate::get(parentItem)->deliveryAgentPrivate();
        Q_ASSERT(daPriv);
        // A menu is typically opened when some event-handling object (like TapHandler) calls
        // QQuickMenu::popup(). We don't have the event or the caller available directly here.
        // But showPopup() below is expected to "eat" the release event, so
        // the caller will not see it. Cancel all grabs so that the object that
        // handled the press event will not get stuck in pressed state.
        if (QPointerEvent *openingEvent = daPriv->eventInDelivery()) {
            auto *devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(openingEvent->pointingDevice()));
            for (const auto &pt : std::as_const(openingEvent->points())) {
                qCDebug(lcNativeMenus) << "popup over" << window << "its DA" << daPriv->q_func() << "opening due to" << openingEvent
                                       << "with grabbers" << openingEvent->exclusiveGrabber(pt) << openingEvent->passiveGrabbers(pt);

                if (auto *opener = openingEvent->exclusiveGrabber(pt))
                    devPriv->removeGrabber(opener, true); // cancel
                for (auto passiveGrabber : openingEvent->passiveGrabbers(pt)) {
                    if (auto *opener = passiveGrabber.get())
                        devPriv->removeGrabber(opener, true); // cancel
                }
            }
        }
        handle->showPopup(window, QHighDpi::toNativeLocalPosition(targetRect, window),
            /*menuItem ? menuItem->handle() : */nullptr);
    } else {
        handle->dismiss();
    }
}

// Used by QQuickContextMenu when it's opened on a text-editing control.
void QQuickMenuPrivate::makeEditMenu()
{
    handle->setMenuType(QPlatformMenu::EditMenu);
}

QQuickItem *QQuickMenuPrivate::itemAt(int index) const
{
    return qobject_cast<QQuickItem *>(contentModel->get(index));
}

void QQuickMenuPrivate::insertItem(int index, QQuickItem *item)
{
    qCDebug(lcMenu) << "insert called with index" << index << "item" << item;

    Q_Q(QQuickMenu);
    contentData.append(item);
    item->setParentItem(contentItem);
    QQuickItemPrivate::get(item)->setCulled(true); // QTBUG-53262
    if (complete)
        resizeItem(item);
    QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
    QQuickItemPrivate::get(item)->updateOrAddGeometryChangeListener(this, QQuickGeometryChange::Width);
    contentModel->insert(index, item);

    QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(item);
    if (menuItem) {
        QQuickMenuItemPrivate::get(menuItem)->setMenu(q);
        if (QQuickMenu *subMenu = menuItem->subMenu())
            QQuickMenuPrivate::get(subMenu)->setParentMenu(q);
        QObjectPrivate::connect(menuItem, &QQuickMenuItem::triggered, this, &QQuickMenuPrivate::onItemTriggered);
        QObjectPrivate::connect(menuItem, &QQuickMenuItem::implicitTextPaddingChanged, this, &QQuickMenuPrivate::updateTextPadding);
        QObjectPrivate::connect(menuItem, &QQuickMenuItem::visibleChanged, this, &QQuickMenuPrivate::updateTextPadding);
        QObjectPrivate::connect(menuItem, &QQuickItem::activeFocusChanged, this, &QQuickMenuPrivate::onItemActiveFocusChanged);
        QObjectPrivate::connect(menuItem, &QQuickControl::hoveredChanged, this, &QQuickMenuPrivate::onItemHovered);
    }

    if (maybeNativeHandle() && complete)
        maybeCreateAndInsertNativeItem(index, item);

    if (lcMenu().isDebugEnabled())
        printContentModelItems();

    updateTextPadding();
}

void QQuickMenuPrivate::maybeCreateAndInsertNativeItem(int index, QQuickItem *item)
{
    Q_Q(QQuickMenu);
    Q_ASSERT(complete);
    Q_ASSERT_X(handle, Q_FUNC_INFO, qPrintable(QString::fromLatin1(
        "Expected %1 to be using a native menu").arg(QDebug::toString(q))));
    std::unique_ptr<QQuickNativeMenuItem> nativeMenuItem(QQuickNativeMenuItem::createFromNonNativeItem(q, item));
    if (!nativeMenuItem) {
        // TODO: fall back to non-native menu
        qmlWarning(q) << "Native menu failed to create a native menu item for item at index" << index;
        return;
    }

    nativeItems.insert(index, nativeMenuItem.get());

    // Having a QQuickNativeMenuItem doesn't mean that we were able to create a native handle:
    // it could be e.g. a Rectangle. See comment in QQuickNativeMenuItem::createFromNonNativeItem.
    if (nativeMenuItem->handle()) {
        QQuickNativeMenuItem *before = nativeItems.value(index + 1);
        handle->insertMenuItem(nativeMenuItem->handle(), before ? before->handle() : nullptr);
        qCDebug(lcNativeMenus) << "inserted native menu item at index" << index
            << "before" << (before ? before->debugText() : QStringLiteral("null"));

        if (nativeMenuItem->subMenu() && QQuickMenuPrivate::get(nativeMenuItem->subMenu())->nativeItems.count()
                < nativeMenuItem->subMenu()->count()) {
            // We're inserting a sub-menu item, and it hasn't had native items added yet,
            // which probably means it's a menu that's been added back in after being removed
            // with takeMenu(). Sub-menus added for the first time have their native items already
            // constructed by virtue of contentData_append. Sub-menus that are removed always
            // have their native items destroyed and removed too.
            recursivelyCreateNativeMenuItems(nativeMenuItem->subMenu());
        }
    }

    nativeMenuItem.release();

    qCDebug(lcNativeMenus) << "nativeItems now contains the following items:"
        << nativeMenuItemListToString(nativeItems);
}

void QQuickMenuPrivate::moveItem(int from, int to)
{
    contentModel->move(from, to);

    if (maybeNativeHandle())
        nativeItems.move(from, to);
}

/*!
    \internal

    Removes the specified \a item, potentially destroying it depending on
    \a destructionPolicy.

    \note the native menu item is destroyed regardless of the destruction
    policy, because it's an implementation detail and hence is not created by
    or available to the user.
*/
void QQuickMenuPrivate::removeItem(int index, QQuickItem *item, DestructionPolicy destructionPolicy)
{
    qCDebug(lcMenu) << "removeItem called with index" << index << "item" << item;

    if (maybeNativeHandle())
        removeNativeItem(index);

    contentData.removeOne(item);

    QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
    QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Geometry);
    item->setParentItem(nullptr);
    contentModel->remove(index);

    QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(item);
    if (menuItem) {
        QQuickMenuItemPrivate::get(menuItem)->setMenu(nullptr);
        if (QQuickMenu *subMenu = menuItem->subMenu())
            QQuickMenuPrivate::get(subMenu)->setParentMenu(nullptr);
        QObjectPrivate::disconnect(menuItem, &QQuickMenuItem::triggered, this, &QQuickMenuPrivate::onItemTriggered);
        QObjectPrivate::disconnect(menuItem, &QQuickMenuItem::implicitTextPaddingChanged, this, &QQuickMenuPrivate::updateTextPadding);
        QObjectPrivate::disconnect(menuItem, &QQuickMenuItem::visibleChanged, this, &QQuickMenuPrivate::updateTextPadding);
        QObjectPrivate::disconnect(menuItem, &QQuickItem::activeFocusChanged, this, &QQuickMenuPrivate::onItemActiveFocusChanged);
        QObjectPrivate::disconnect(menuItem, &QQuickControl::hoveredChanged, this, &QQuickMenuPrivate::onItemHovered);
    }

    if (destructionPolicy == DestructionPolicy::Destroy)
        item->deleteLater();

    if (lcMenu().isDebugEnabled())
        printContentModelItems();
}

void QQuickMenuPrivate::removeNativeItem(int index)
{
    // Either we're still using native menus and are removing item(s), or we've switched
    // to a non-native menu; either way, we should actually have items to remove before we're called.
    Q_ASSERT(handle);
    Q_ASSERT_X(index >= 0 && index < nativeItems.size(), Q_FUNC_INFO, qPrintable(QString::fromLatin1(
        "index %1 is less than 0 or greater than or equal to %2").arg(index).arg(nativeItems.size())));

    // We can delete the item synchronously because there aren't any external (e.g. QML)
    // references to it.
    std::unique_ptr<QQuickNativeMenuItem> nativeItem(nativeItems.takeAt(index));
    qCDebug(lcNativeMenus) << "removing native item" << nativeItem->debugText() << "at index" << index
        << "from" << q_func() << "...";
    if (QQuickMenu *subMenu = nativeItem->subMenu())
        recursivelyDestroyNativeSubMenus(subMenu);

    if (nativeItem->handle()) {
        handle->removeMenuItem(nativeItem->handle());
        syncWithNativeMenu();
    }

    qCDebug(lcNativeMenus).nospace() << "... after removing item at index " << index
        << ", nativeItems now contains the following items: " << nativeMenuItemListToString(nativeItems);
}

void QQuickMenuPrivate::resetNativeData()
{
    qCDebug(lcNativeMenus) << "resetNativeData called on" << q_func();
    handle.reset();
    triedToCreateNativeMenu = false;
}

void QQuickMenuPrivate::recursivelyCreateNativeMenuItems(QQuickMenu *menu)
{
    auto *menuPrivate = QQuickMenuPrivate::get(menu);
    // If we're adding a sub-menu, we need to ensure its handle has been created
    // before trying to create native items for it.
    if (!menuPrivate->triedToCreateNativeMenu)
        menuPrivate->createNativeMenu();

    const int qtyItemsToCreate = menuPrivate->contentModel->count();
    if (menuPrivate->nativeItems.count() == qtyItemsToCreate)
        return;

    qCDebug(lcNativeMenus) << "recursively creating" << qtyItemsToCreate << "menu item(s) for" << menu;
    Q_ASSERT(menuPrivate->nativeItems.count() == 0);
    for (int i = 0; i < qtyItemsToCreate; ++i) {
        QQuickItem *item = menu->itemAt(i);
        menuPrivate->maybeCreateAndInsertNativeItem(i, item);
        auto *menuItem = qobject_cast<QQuickMenuItem *>(item);
        if (menuItem && menuItem->subMenu())
            recursivelyCreateNativeMenuItems(menuItem->subMenu());
    }
}

void QQuickMenuPrivate::printContentModelItems() const
{
    qCDebug(lcMenu) << "contentModel now contains:";
    for (int i = 0; i < contentModel->count(); ++i)
        qCDebug(lcMenu) << "-" << itemAt(i);
}

QQuickItem *QQuickMenuPrivate::beginCreateItem()
{
    Q_Q(QQuickMenu);
    if (!delegate)
        return nullptr;

    QQmlContext *context = delegate->creationContext();
    if (!context)
        context = qmlContext(q);

    QObject *object = delegate->beginCreate(context);
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item)
        delete object;
    else
        QQml_setParent_noEvent(item, q);

    return item;
}

void QQuickMenuPrivate::completeCreateItem()
{
    if (!delegate)
        return;

    delegate->completeCreate();
}

QQuickItem *QQuickMenuPrivate::createItem(QQuickMenu *menu)
{
    QQuickItem *item = beginCreateItem();
    if (QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(item))
        QQuickMenuItemPrivate::get(menuItem)->setSubMenu(menu);
    completeCreateItem();
    return item;
}

QQuickItem *QQuickMenuPrivate::createItem(QQuickAction *action)
{
    QQuickItem *item = beginCreateItem();
    if (QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(item))
        button->setAction(action);
    completeCreateItem();
    return item;
}

void QQuickMenuPrivate::resizeItem(QQuickItem *item)
{
    if (!item || !contentItem)
        return;

    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    if (!p->widthValid()) {
        item->setWidth(contentItem->width());
        p->widthValidFlag = false;
    }
}

void QQuickMenuPrivate::resizeItems()
{
    if (!contentModel)
        return;

    for (int i = 0; i < contentModel->count(); ++i)
        resizeItem(itemAt(i));
}

void QQuickMenuPrivate::itemChildAdded(QQuickItem *, QQuickItem *child)
{
    // add dynamically reparented items (eg. by a Repeater)
    if (!QQuickItemPrivate::get(child)->isTransparentForPositioner() && !contentData.contains(child))
        insertItem(contentModel->count(), child);
}

void QQuickMenuPrivate::itemParentChanged(QQuickItem *item, QQuickItem *parent)
{
    // remove dynamically unparented items (eg. by a Repeater)
    if (!parent)
        removeItem(contentModel->indexOf(item, nullptr), item);
}

void QQuickMenuPrivate::itemSiblingOrderChanged(QQuickItem *)
{
    // reorder the restacked items (eg. by a Repeater)
    Q_Q(QQuickMenu);
    QList<QQuickItem *> siblings = contentItem->childItems();

    int to = 0;
    for (int i = 0; i < siblings.size(); ++i) {
        QQuickItem* sibling = siblings.at(i);
        if (QQuickItemPrivate::get(sibling)->isTransparentForPositioner())
            continue;
        int index = contentModel->indexOf(sibling, nullptr);
        q->moveItem(index, to++);
    }
}

void QQuickMenuPrivate::itemDestroyed(QQuickItem *item)
{
    QQuickPopupPrivate::itemDestroyed(item);
    int index = contentModel->indexOf(item, nullptr);
    if (index != -1)
        removeItem(index, item);
}

void QQuickMenuPrivate::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange, const QRectF &)
{
    if (!complete)
        return;

    if (item == contentItem) {
        // The contentItem's geometry changed, so resize any items
        // that don't have explicit widths set so that they fill the width of the menu.
        resizeItems();
    } else {
        // The geometry of an item in the menu changed. If the item
        // doesn't have an explicit width set, make it fill the width of the menu.
        resizeItem(item);
    }
}

QQuickPopupPositioner *QQuickMenuPrivate::getPositioner()
{
    Q_Q(QQuickMenu);
    if (!positioner)
        positioner = new QQuickMenuPositioner(q);
    return positioner;
}

void QQuickMenuPositioner::reposition()
{
    QQuickMenu *menu = static_cast<QQuickMenu *>(popup());
    QQuickMenuPrivate *menu_d = QQuickMenuPrivate::get(menu);

    if (QQuickMenu *parentMenu = menu_d->parentMenu) {
        if (menu_d->cascade) {
            // Align the menu to the frame of the parent menuItem, minus overlap. The position
            // should be in the coordinate system of the parentItem.
            if (menu_d->popupItem->isMirrored()) {
                const qreal distanceToFrame = parentMenu->leftPadding();
                const qreal menuX = -menu->width() - distanceToFrame + menu->overlap();
                menu->setPosition({menuX, -menu->topPadding()});
            } else if (menu_d->parentItem) {
                const qreal distanceToFrame = parentMenu->rightPadding();
                const qreal menuX = menu_d->parentItem->width() + distanceToFrame - menu->overlap();
                menu->setPosition({menuX, -menu->topPadding()});
            }
        } else {
            const qreal menuX = parentMenu->x() + (parentMenu->width() - menu->width()) / 2;
            const qreal menuY = parentMenu->y() + (parentMenu->height() - menu->height()) / 2;
            menu->setPosition({menuX, menuY});
        }
    }

    QQuickPopupPositioner::reposition();
}

bool QQuickMenuPrivate::prepareEnterTransition()
{
    Q_Q(QQuickMenu);
    if (parentMenu && !cascade)
        parentMenu->close();

    // If a cascading sub-menu doesn't have enough space to open on
    // the right, it flips on the other side of the parent menu.
    allowHorizontalFlip = cascade && parentMenu;

    if (!QQuickPopupPrivate::prepareEnterTransition())
        return false;

    if (!hasClosePolicy) {
        if (cascade && parentMenu)
            closePolicy = cascadingSubMenuClosePolicy;
        else
            q->resetClosePolicy();
    }
    return true;
}

bool QQuickMenuPrivate::prepareExitTransition()
{
    if (!QQuickPopupPrivate::prepareExitTransition())
        return false;

    stopHoverTimer();

    QQuickMenu *subMenu = currentSubMenu();
    while (subMenu) {
        QPointer<QQuickMenuItem> currentSubMenuItem = QQuickMenuPrivate::get(subMenu)->currentItem;
        subMenu->close();
        subMenu = currentSubMenuItem ? currentSubMenuItem->subMenu() : nullptr;
    }
    return true;
}

bool QQuickMenuPrivate::blockInput(QQuickItem *item, const QPointF &point) const
{
    // keep the parent menu open when a cascading sub-menu (this menu) is interacted with
    return (cascade && parentMenu && contains(point)) || QQuickPopupPrivate::blockInput(item, point);
}

bool QQuickMenuPrivate::handlePress(QQuickItem *item, const QPointF &point, ulong timestamp)
{
    // Don't propagate mouse event as it can cause underlying item to receive
    // events
    return QQuickPopupPrivate::handlePress(item, point, timestamp)
            || (popupItem == item);
}


/*! \internal
    QQuickPopupWindow::event() calls this to handle the release event of a
    menu drag-press-release gesture, because the \a eventPoint does not have
    a grabber within the popup window. This override finds and activates the
    appropriate menu item, as if it had been pressed and released.
    Returns true on success, to indicate that handling \a eventPoint is done.
 */
bool QQuickMenuPrivate::handleReleaseWithoutGrab(const QEventPoint &eventPoint)
{
    const QPointF scenePos = eventPoint.scenePosition();
    if (!contains(scenePos))
        return false;

    auto *list = qobject_cast<QQuickListView *>(contentItem);
    if (!list)
        return false;

    const QPointF listPos = list->mapFromScene(scenePos);

    auto *menuItem = qobject_cast<QQuickMenuItem *>(list->itemAt(listPos.x(), listPos.y()));
    if (menuItem && menuItem->isHighlighted()) {
        menuItem->animateClick();
        return true;
    }

    return false;
}

void QQuickMenuPrivate::onItemHovered()
{
    Q_Q(QQuickMenu);
    QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(q->sender());
    if (!button || !button->isHovered() || !button->isEnabled() || QQuickAbstractButtonPrivate::get(button)->touchId != -1)
        return;

    QQuickMenuItem *oldCurrentItem = currentItem;

    int index = contentModel->indexOf(button, nullptr);
    if (index != -1) {
        setCurrentIndex(index, Qt::OtherFocusReason);
        if (oldCurrentItem != currentItem) {
            if (oldCurrentItem) {
                QQuickMenu *subMenu = oldCurrentItem->subMenu();
                if (subMenu)
                    subMenu->close();
            }
            if (currentItem) {
                QQuickMenu *subMenu = currentItem->menu();
                if (subMenu && subMenu->cascade())
                    startHoverTimer();
            }
        }
    }
}

void QQuickMenuPrivate::onItemTriggered()
{
    Q_Q(QQuickMenu);
    QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(q->sender());
    if (!item)
        return;

    if (QQuickMenu *subMenu = item->subMenu()) {
        auto subMenuPrivate = QQuickMenuPrivate::get(subMenu);
        subMenuPrivate->popup(subMenuPrivate->firstEnabledMenuItem());
    } else {
        q->dismiss();
    }
}

void QQuickMenuPrivate::onItemActiveFocusChanged()
{
    Q_Q(QQuickMenu);
    QQuickItem *item = qobject_cast<QQuickItem*>(q->sender());
    if (!item->hasActiveFocus())
        return;

    int indexOfItem = contentModel->indexOf(item, nullptr);
    QQuickControl *control = qobject_cast<QQuickControl *>(item);
    setCurrentIndex(indexOfItem, control ? control->focusReason() : Qt::OtherFocusReason);
}

void QQuickMenuPrivate::updateTextPadding()
{
    Q_Q(QQuickMenu);
    if (!complete)
        return;

    qreal padding = 0;
    for (int i = 0; i < q->count(); ++i) {
        if (const auto menuItem = qobject_cast<QQuickMenuItem *>(itemAt(i)))
            if (menuItem->isVisible())
                padding = qMax(padding, menuItem->implicitTextPadding());
    }

    if (padding == textPadding)
        return;

    textPadding = padding;

    for (int i = 0; i < q->count(); ++i) {
        if (const auto menuItem = qobject_cast<QQuickMenuItem *>(itemAt(i)))
            emit menuItem->textPaddingChanged();
    }
}

QQuickMenu *QQuickMenuPrivate::currentSubMenu() const
{
    if (!currentItem)
        return nullptr;

    return currentItem->subMenu();
}

void QQuickMenuPrivate::setParentMenu(QQuickMenu *parent)
{
    Q_Q(QQuickMenu);
    if (parentMenu == parent)
        return;

    if (parentMenu) {
        QObject::disconnect(parentMenu.data(), &QQuickMenu::cascadeChanged, q, &QQuickMenu::setCascade);
        disconnect(parentMenu.data(), &QQuickMenu::parentChanged, this, &QQuickMenuPrivate::resolveParentItem);
    }
    if (parent) {
        QObject::connect(parent, &QQuickMenu::cascadeChanged, q, &QQuickMenu::setCascade);
        connect(parent, &QQuickMenu::parentChanged, this, &QQuickMenuPrivate::resolveParentItem);
    }

    parentMenu = parent;
    q->resetCascade();
    resolveParentItem();
}

static QQuickItem *findParentMenuItem(QQuickMenu *subMenu)
{
    QQuickMenu *menu = QQuickMenuPrivate::get(subMenu)->parentMenu;
    for (int i = 0; i < QQuickMenuPrivate::get(menu)->contentModel->count(); ++i) {
        QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(menu->itemAt(i));
        if (item && item->subMenu() == subMenu)
            return item;
    }
    return nullptr;
}

void QQuickMenuPrivate::resolveParentItem()
{
    Q_Q(QQuickMenu);
    if (!parentMenu)
        q->resetParentItem();
    else if (!cascade)
        q->setParentItem(parentMenu->parentItem());
    else
        q->setParentItem(findParentMenuItem(q));
}

void QQuickMenuPrivate::propagateKeyEvent(QKeyEvent *event)
{
    if (QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(parentItem)) {
        if (QQuickMenu *menu = menuItem->menu())
            QQuickMenuPrivate::get(menu)->propagateKeyEvent(event);
#if QT_CONFIG(quicktemplates2_container)
    } else if (QQuickMenuBarItem *menuBarItem = qobject_cast<QQuickMenuBarItem *>(parentItem)) {
        if (QQuickMenuBar *menuBar = menuBarItem->menuBar()) {
            event->accept();
            QCoreApplication::sendEvent(menuBar, event);
        }
#endif
    }
}

void QQuickMenuPrivate::startHoverTimer()
{
    Q_Q(QQuickMenu);
    stopHoverTimer();
    hoverTimer = q->startTimer(SUBMENU_DELAY);
}

void QQuickMenuPrivate::stopHoverTimer()
{
    Q_Q(QQuickMenu);
    if (!hoverTimer)
        return;

    q->killTimer(hoverTimer);
    hoverTimer = 0;
}

void QQuickMenuPrivate::setCurrentIndex(int index, Qt::FocusReason reason)
{
    Q_Q(QQuickMenu);
    if (currentIndex == index)
        return;

    QQuickMenuItem *newCurrentItem = qobject_cast<QQuickMenuItem *>(itemAt(index));
    if (currentItem != newCurrentItem) {
        stopHoverTimer();
        if (currentItem) {
            currentItem->setHighlighted(false);
            if (!newCurrentItem && window) {
                QQuickItem *focusItem = QQuickItemPrivate::get(contentItem)->subFocusItem;
                if (focusItem) {
                    auto *daPriv = QQuickWindowPrivate::get(window)->deliveryAgentPrivate();
                    daPriv->clearFocusInScope(contentItem, focusItem, Qt::OtherFocusReason);
                }
            }
        }
        if (newCurrentItem) {
            newCurrentItem->setHighlighted(true);
            newCurrentItem->forceActiveFocus(reason);
        }
        currentItem = newCurrentItem;
    }

    currentIndex = index;
    emit q->currentIndexChanged();
}

bool QQuickMenuPrivate::activateNextItem()
{
    int index = currentIndex;
    int count = contentModel->count();
    while (++index < count) {
        QQuickItem *item = itemAt(index);
        if (!item || !item->activeFocusOnTab() || !item->isEnabled())
            continue;
        setCurrentIndex(index, Qt::TabFocusReason);
        return true;
    }
    return false;
}

bool QQuickMenuPrivate::activatePreviousItem()
{
    int index = currentIndex;
    while (--index >= 0) {
        QQuickItem *item = itemAt(index);
        if (!item || !item->activeFocusOnTab() || !item->isEnabled())
            continue;
        setCurrentIndex(index, Qt::BacktabFocusReason);
        return true;
    }
    return false;
}

QQuickMenuItem *QQuickMenuPrivate::firstEnabledMenuItem() const
{
    for (int i = 0; i < contentModel->count(); ++i) {
        QQuickItem *item = itemAt(i);
        if (!item || !item->isEnabled())
            continue;

        QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(item);
        if (!menuItem)
            continue;

        return menuItem;
    }
    return nullptr;
}

void QQuickMenuPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickMenu *q = qobject_cast<QQuickMenu *>(prop->object);
    QQuickMenuPrivate *p = QQuickMenuPrivate::get(q);

    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    if (!item) {
        if (QQuickAction *action = qobject_cast<QQuickAction *>(obj))
            item = p->createItem(action);
        else if (QQuickMenu *menu = qobject_cast<QQuickMenu *>(obj))
            item = p->createItem(menu);
    }

    if (item) {
        if (QQuickItemPrivate::get(item)->isTransparentForPositioner()) {
            QQuickItemPrivate::get(item)->addItemChangeListener(p, QQuickItemPrivate::SiblingOrder);
            item->setParentItem(p->contentItem);
        } else if (p->contentModel->indexOf(item, nullptr) == -1) {
            q->addItem(item);
        }
    } else {
        p->contentData.append(obj);
    }
}

qsizetype QQuickMenuPrivate::contentData_count(QQmlListProperty<QObject> *prop)
{
    QQuickMenu *q = static_cast<QQuickMenu *>(prop->object);
    return QQuickMenuPrivate::get(q)->contentData.size();
}

QObject *QQuickMenuPrivate::contentData_at(QQmlListProperty<QObject> *prop, qsizetype index)
{
    QQuickMenu *q = static_cast<QQuickMenu *>(prop->object);
    return QQuickMenuPrivate::get(q)->contentData.value(index);
}

QPalette QQuickMenuPrivate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::Menu);
}

void QQuickMenuPrivate::contentData_clear(QQmlListProperty<QObject> *prop)
{
    QQuickMenu *q = static_cast<QQuickMenu *>(prop->object);
    QQuickMenuPrivate::get(q)->contentData.clear();
}

QQuickMenu::QQuickMenu(QObject *parent)
    : QQuickPopup(*(new QQuickMenuPrivate), parent)
{
    Q_D(QQuickMenu);
    setFocus(true);
    d->init();
    connect(d->contentModel, &QQmlObjectModel::countChanged, this, &QQuickMenu::countChanged);
}

QQuickMenu::~QQuickMenu()
{
    Q_D(QQuickMenu);
    qCDebug(lcNativeMenus) << "destroying" << this
                          << "item count:"
                          << d->contentModel->count()
                          << "native item count:" << d->nativeItems.count();
    // We have to remove items to ensure that our change listeners on the item
    // are removed. It's too late to do this in ~QQuickMenuPrivate, as
    // contentModel has already been destroyed before that is called.
    // Destruction isn't necessary for the QQuickItems themselves, but it is
    // required for the native menus (see comment in removeItem()).
    while (d->contentModel->count() > 0)
        d->removeItem(0, d->itemAt(0), QQuickMenuPrivate::DestructionPolicy::Destroy);

    if (d->contentItem) {
        QQuickItemPrivate::get(d->contentItem)->removeItemChangeListener(d, QQuickItemPrivate::Children);
        QQuickItemPrivate::get(d->contentItem)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);

        const auto children = d->contentItem->childItems();
        for (QQuickItem *child : std::as_const(children))
            QQuickItemPrivate::get(child)->removeItemChangeListener(d, QQuickItemPrivate::SiblingOrder);
    }
}

/*!
    \qmlmethod Item QtQuick.Controls::Menu::itemAt(int index)

    Returns the item at \a index, or \c null if it does not exist.
*/
QQuickItem *QQuickMenu::itemAt(int index) const
{
    Q_D(const QQuickMenu);
    return d->itemAt(index);
}

/*!
    \qmlmethod void QtQuick.Controls::Menu::addItem(Item item)

    Adds \a item to the end of the list of items.
*/
void QQuickMenu::addItem(QQuickItem *item)
{
    Q_D(QQuickMenu);
    insertItem(d->contentModel->count(), item);
}

/*!
    \qmlmethod void QtQuick.Controls::Menu::insertItem(int index, Item item)

    Inserts \a item at \a index.
*/
void QQuickMenu::insertItem(int index, QQuickItem *item)
{
    Q_D(QQuickMenu);
    if (!item)
        return;
    const int count = d->contentModel->count();
    if (index < 0 || index > count)
        index = count;

    int oldIndex = d->contentModel->indexOf(item, nullptr);
    if (oldIndex != -1) {
        if (oldIndex < index)
            --index;
        if (oldIndex != index) {
            d->moveItem(oldIndex, index);
        }
    } else {
        d->insertItem(index, item);
    }
}

/*!
    \qmlmethod void QtQuick.Controls::Menu::moveItem(int from, int to)

    Moves an item \a from one index \a to another.
*/
void QQuickMenu::moveItem(int from, int to)
{
    Q_D(QQuickMenu);
    const int count = d->contentModel->count();
    if (from < 0 || from > count - 1)
        return;
    if (to < 0 || to > count - 1)
        to = count - 1;

    if (from != to)
        d->moveItem(from, to);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::removeItem(Item item)

    Removes and destroys the specified \a item.
*/
void QQuickMenu::removeItem(QQuickItem *item)
{
    Q_D(QQuickMenu);
    if (!item)
        return;

    const int index = d->contentModel->indexOf(item, nullptr);
    if (index == -1)
        return;

    d->removeItem(index, item, QQuickMenuPrivate::DestructionPolicy::Destroy);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod MenuItem QtQuick.Controls::Menu::takeItem(int index)

    Removes and returns the item at \a index.

    \note The ownership of the item is transferred to the caller.
*/
QQuickItem *QQuickMenu::takeItem(int index)
{
    Q_D(QQuickMenu);
    const int count = d->contentModel->count();
    if (index < 0 || index >= count)
        return nullptr;

    QQuickItem *item = itemAt(index);
    if (item)
        d->removeItem(index, item);
    return item;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod Menu QtQuick.Controls::Menu::menuAt(int index)

    Returns the sub-menu at \a index, or \c null if the index is not valid or
    there is no sub-menu at the specified index.
*/
QQuickMenu *QQuickMenu::menuAt(int index) const
{
    Q_D(const QQuickMenu);
    QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(d->itemAt(index));
    if (!item)
        return nullptr;

    return item->subMenu();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::addMenu(Menu menu)

    Adds \a menu as a sub-menu to the end of this menu.
*/
void QQuickMenu::addMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenu);
    insertMenu(d->contentModel->count(), menu);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::insertMenu(int index, Menu menu)

    Inserts \a menu as a sub-menu at \a index. The index is within all items in the menu.
*/
void QQuickMenu::insertMenu(int index, QQuickMenu *menu)
{
    Q_D(QQuickMenu);
    if (!menu)
        return;

    insertItem(index, d->createItem(menu));
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::removeMenu(Menu menu)

    Removes and destroys the specified \a menu.
*/
void QQuickMenu::removeMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenu);
    if (!menu)
        return;

    const int count = d->contentModel->count();
    for (int i = 0; i < count; ++i) {
        QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(d->itemAt(i));
        if (!item || item->subMenu() != menu)
            continue;

        removeItem(item);
        break;
    }

    menu->deleteLater();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod Menu QtQuick.Controls::Menu::takeMenu(int index)

    Removes and returns the menu at \a index. The index is within all items in the menu.

    \note The ownership of the menu is transferred to the caller.
*/
QQuickMenu *QQuickMenu::takeMenu(int index)
{
    Q_D(QQuickMenu);
    QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(d->itemAt(index));
    if (!item)
        return nullptr;

    QQuickMenu *subMenu = item->subMenu();
    if (!subMenu)
        return nullptr;

    d->removeItem(index, item);
    item->deleteLater();

    return subMenu;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod Action QtQuick.Controls::Menu::actionAt(int index)

    Returns the action at \a index, or \c null if the index is not valid or
    there is no action at the specified index.
*/
QQuickAction *QQuickMenu::actionAt(int index) const
{
    Q_D(const QQuickMenu);
    if (!const_cast<QQuickMenuPrivate *>(d)->maybeNativeHandle()) {
        QQuickAbstractButton *item = qobject_cast<QQuickAbstractButton *>(d->itemAt(index));
        if (!item)
            return nullptr;

        return item->action();
    } else {
        if (index < 0 || index >= d->nativeItems.size())
            return nullptr;

        return d->nativeItems.at(index)->action();
    }
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::addAction(Action action)

    Adds \a action to the end of this menu.
*/
void QQuickMenu::addAction(QQuickAction *action)
{
    Q_D(QQuickMenu);
    insertAction(d->contentModel->count(), action);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::insertAction(int index, Action action)

    Inserts \a action at \a index. The index is within all items in the menu.
*/
void QQuickMenu::insertAction(int index, QQuickAction *action)
{
    Q_D(QQuickMenu);
    if (!action)
        return;

    insertItem(index, d->createItem(action));
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::removeAction(Action action)

    Removes and destroys the specified \a action.
*/
void QQuickMenu::removeAction(QQuickAction *action)
{
    Q_D(QQuickMenu);
    if (!action)
        return;

    const int count = d->contentModel->count();
    for (int i = 0; i < count; ++i) {
        QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(d->itemAt(i));
        if (!item || item->action() != action)
            continue;

        removeItem(item);
        break;
    }

    action->deleteLater();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod Action QtQuick.Controls::Menu::takeAction(int index)

    Removes and returns the action at \a index. The index is within all items in the menu.

    \note The ownership of the action is transferred to the caller.
*/
QQuickAction *QQuickMenu::takeAction(int index)
{
    Q_D(QQuickMenu);
    QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(d->itemAt(index));
    if (!item)
        return nullptr;

    QQuickAction *action = item->action();
    if (!action)
        return nullptr;

    d->removeItem(index, item);
    item->deleteLater();
    return action;
}

bool QQuickMenu::isVisible() const
{
    Q_D(const QQuickMenu);
    if (d->maybeNativeHandle())
        return d->visible;
    return QQuickPopup::isVisible();
}

void QQuickMenu::setVisible(bool visible)
{
    Q_D(QQuickMenu);
    if (visible == d->visible)
        return;
    if (visible && !parentItem()) {
        qmlWarning(this) << "cannot show menu: parent is null";
        return;
    }
    if (visible) {
        // If a right mouse button event opens a menu, don't synthesize QContextMenuEvent
        // (avoid opening redundant menus, e.g. in parent items).
        Q_ASSERT(window());
        QQuickWindowPrivate::get(window())->rmbContextMenuEventEnabled = false;
    }

    if (visible && ((d->useNativeMenu() && !d->maybeNativeHandle())
            || (!d->useNativeMenu() && d->maybeNativeHandle()))) {
        // We've been made visible, and our actual native state doesn't match our requested state,
        // which means AA_DontUseNativeMenuWindows was set while we were visible or had a parent.
        // Try to sync our state again now that we're about to be re-opened.
        qCDebug(lcNativeMenus) << "setVisible called - useNativeMenu:" << d->useNativeMenu()
            << "maybeNativeHandle:" << d->maybeNativeHandle();
        d->syncWithUseNativeMenu();
    }
    if (d->maybeNativeHandle()) {
        d->setNativeMenuVisible(visible);
        return;
    }

    // Either the native menu wasn't wanted, or it couldn't be created;
    // show the non-native menu.
    QQuickPopup::setVisible(visible);
}

/*!
    \qmlproperty model QtQuick.Controls::Menu::contentModel
    \readonly

    This property holds the model used to display menu items.

    The content model is provided for visualization purposes. It can be assigned
    as a model to a content item that presents the contents of the menu.

    \code
    Menu {
        id: menu
        contentItem: ListView {
            model: menu.contentModel
        }
    }
    \endcode

    The model allows menu items to be statically declared as children of the
    menu.
*/
QVariant QQuickMenu::contentModel() const
{
    Q_D(const QQuickMenu);
    return QVariant::fromValue(d->contentModel);
}

/*!
    \qmlproperty list<QtObject> QtQuick.Controls::Menu::contentData
    \qmldefault

    This property holds the list of content data.

    The list contains all objects that have been declared in QML as children
    of the menu, and also items that have been dynamically added or
    inserted using the \l addItem() and \l insertItem() methods, respectively.

    \note Unlike \c contentChildren, \c contentData does include non-visual QML
    objects. It is not re-ordered when items are inserted or moved.

    \sa Item::data, {Popup::}{contentChildren}
*/
QQmlListProperty<QObject> QQuickMenu::contentData()
{
    Q_D(QQuickMenu);
    if (!d->contentItem)
        QQuickControlPrivate::get(d->popupItem)->executeContentItem();
    return QQmlListProperty<QObject>(this, nullptr,
        QQuickMenuPrivate::contentData_append,
        QQuickMenuPrivate::contentData_count,
        QQuickMenuPrivate::contentData_at,
        QQuickMenuPrivate::contentData_clear);
}

/*!
    \qmlproperty string QtQuick.Controls::Menu::title

    This property holds the title for the menu.

    The title of a menu is often displayed in the text of a menu item when the
    menu is a submenu, and in the text of a tool button when it is in a
    menubar.
*/
QString QQuickMenu::title() const
{
    Q_D(const QQuickMenu);
    return d->title;
}

void QQuickMenu::setTitle(const QString &title)
{
    Q_D(QQuickMenu);
    if (title == d->title)
        return;
    d->title = title;
    if (d->handle)
        d->handle->setText(title);
    emit titleChanged(title);
}

/*!
    \qmlproperty string QtQuick.Controls::Menu::icon.name
    \qmlproperty url QtQuick.Controls::Menu::icon.source
    \qmlproperty int QtQuick.Controls::Menu::icon.width
    \qmlproperty int QtQuick.Controls::Menu::icon.height
    \qmlproperty color QtQuick.Controls::Menu::icon.color
    \qmlproperty bool QtQuick.Controls::Menu::icon.cache

    This property group was added in QtQuick.Controls 6.5.

    \include qquickicon.qdocinc grouped-properties

    \include qquickmenu.qdocinc non-native-only-property

    \sa AbstractButton::text, AbstractButton::display, {Icons in Qt Quick Controls}
*/

QQuickIcon QQuickMenu::icon() const
{
    Q_D(const QQuickMenu);
    return d->icon;
}

void QQuickMenu::setIcon(const QQuickIcon &icon)
{
    Q_D(QQuickMenu);
    if (icon == d->icon)
        return;
    d->icon = icon;
    d->icon.ensureRelativeSourceResolved(this);
    emit iconChanged(icon);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::Menu::cascade

    This property holds whether the menu cascades its sub-menus.

    The default value is platform-specific. Menus are cascading by default on
    desktop platforms that have a mouse cursor available. Non-cascading menus
    are shown one menu at a time, and centered over the parent menu.

    \note Changing the value of the property has no effect while the menu is open.

    \include qquickmenu.qdocinc non-native-only-property

    \sa overlap
*/
bool QQuickMenu::cascade() const
{
    Q_D(const QQuickMenu);
    return d->cascade;
}

void QQuickMenu::setCascade(bool cascade)
{
    Q_D(QQuickMenu);
    if (d->cascade == cascade)
        return;
    d->cascade = cascade;
    if (d->parentMenu)
        d->resolveParentItem();
    emit cascadeChanged(cascade);
}

void QQuickMenu::resetCascade()
{
    Q_D(QQuickMenu);
    if (d->parentMenu)
        setCascade(d->parentMenu->cascade());
    else
        setCascade(shouldCascade());
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty real QtQuick.Controls::Menu::overlap

    This property holds the amount of pixels by which the menu horizontally overlaps its parent menu.

    The property only has effect when the menu is used as a cascading sub-menu.

    The default value is style-specific.

    \note Changing the value of the property has no effect while the menu is open.

    \include qquickmenu.qdocinc non-native-only-property

    \sa cascade
*/
qreal QQuickMenu::overlap() const
{
    Q_D(const QQuickMenu);
    return d->overlap;
}

void QQuickMenu::setOverlap(qreal overlap)
{
    Q_D(QQuickMenu);
    if (d->overlap == overlap)
        return;
    d->overlap = overlap;
    emit overlapChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty Component QtQuick.Controls::Menu::delegate

    This property holds the component that is used to create items
    to present actions.

    \code
    Menu {
        Action { text: "Cut" }
        Action { text: "Copy" }
        Action { text: "Paste" }
    }
    \endcode

    \note delegates will only be visible when using a \l {Menu types}
    {non-native Menu}.

    \sa Action
*/
QQmlComponent *QQuickMenu::delegate() const
{
    Q_D(const QQuickMenu);
    return d->delegate;
}

void QQuickMenu::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickMenu);
    if (d->delegate == delegate)
        return;

    d->delegate = delegate;
    emit delegateChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty int QtQuick.Controls::Menu::currentIndex

    This property holds the index of the currently highlighted item.

    Menu items can be highlighted by mouse hover or keyboard navigation.

    \include qquickmenu.qdocinc non-native-only-property

    \sa MenuItem::highlighted
*/
int QQuickMenu::currentIndex() const
{
    Q_D(const QQuickMenu);
    return d->currentIndex;
}

void QQuickMenu::setCurrentIndex(int index)
{
    Q_D(QQuickMenu);
    d->setCurrentIndex(index, Qt::OtherFocusReason);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty int QtQuick.Controls::Menu::count
    \readonly

    This property holds the number of items.
*/
int QQuickMenu::count() const
{
    Q_D(const QQuickMenu);
    return d->contentModel->count();
}

void QQuickMenuPrivate::popup(QQuickItem *menuItem)
{
    Q_Q(QQuickMenu);
    // No position has been explicitly specified, so position the menu at the mouse cursor
    // on desktop platforms that have a mouse cursor available and support multiple windows.
    QQmlNullableValue<QPointF> pos;
#if QT_CONFIG(cursor)
    if (parentItem && QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows))
        pos = parentItem->mapFromGlobal(QCursor::pos());
#endif

    // As a fallback, center the menu over its parent item.
    if (!pos.isValid() && parentItem)
        pos = QPointF((parentItem->width() - q->width()) / 2, (parentItem->height() - q->height()) / 2);

    q->popup(pos.isValid() ? pos.value() : QPointF(), menuItem);
}

void QQuickMenu::popup(const QPointF &position, QQuickItem *menuItem)
{
    Q_D(QQuickMenu);
    qreal offset = 0;
#if QT_CONFIG(cursor)
    if (menuItem)
        offset = d->popupItem->mapFromItem(menuItem, QPointF(0, 0)).y();
#endif
    setPosition(position - QPointF(0, offset));

    if (menuItem)
        d->setCurrentIndex(d->contentModel->indexOf(menuItem, nullptr), Qt::PopupFocusReason);
    else
        d->setCurrentIndex(-1, Qt::PopupFocusReason);

    open();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::popup(MenuItem item = null)
    \qmlmethod void QtQuick.Controls::Menu::popup(Item parent, MenuItem item = null)

    Opens the menu at the mouse cursor on desktop platforms that have a mouse cursor
    available, and otherwise centers the menu over its \a parent item.

    The menu can be optionally aligned to a specific menu \a item. This item will
    then become \l {currentIndex}{current.} If no \a item is specified, \l currentIndex
    will be set to \c -1.

    \sa Popup::open()
*/

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::popup(point pos, MenuItem item = null)
    \qmlmethod void QtQuick.Controls::Menu::popup(Item parent, point pos, MenuItem item = null)

    Opens the menu at the specified position \a pos in the popups coordinate system,
    that is, a coordinate relative to its \a parent item.

    The menu can be optionally aligned to a specific menu \a item. This item will
    then become \l {currentIndex}{current.} If no \a item is specified, \l currentIndex
    will be set to \c -1.

    \sa Popup::open()
*/

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::popup(real x, real y, MenuItem item = null)
    \qmlmethod void QtQuick.Controls::Menu::popup(Item parent, real x, real y, MenuItem item = null)

    Opens the menu at the specified position \a x, \a y in the popups coordinate system,
    that is, a coordinate relative to its \a parent item.

    The menu can be optionally aligned to a specific menu \a item. This item will
    then become \l {currentIndex}{current.} If no \a item is specified, \l currentIndex
    will be set to \c -1.

    \sa dismiss(), Popup::open()
*/

void QQuickMenu::popup(QQuickItem *parent, qreal x, qreal y, QQuickItem *menuItem)
{
    popup(parent, QPointF {x, y}, menuItem);
}

void QQuickMenu::popup(QQuickItem *parent, const QPointF &position, QQuickItem *menuItem)
{
    Q_D(QQuickMenu);
    if (parent && !d->popupItem->isAncestorOf(parent))
        setParentItem(parent);
    popup(position, menuItem);
}

void QQuickMenu::popup(QQuickItem *parent, QQuickItem *menuItem)
{
    Q_D(QQuickMenu);
    QQuickItem *parentItem = nullptr;
    if (parent && !d->popupItem->isAncestorOf(parent))
        parentItem = parent;
    if (parentItem)
        setParentItem(parentItem);
    d->popup(menuItem);
}

// if a single argument is given, it is treated as both the parent _and_ the menu item
// note: This differs from QQuickMenuPrivate::popup, which doesn't do parent handling
void QQuickMenu::popup(QQuickItem *parent)
{
    Q_D(QQuickMenu);
    QQuickItem *menuItem = nullptr;
    if (parent) {
        if (!d->popupItem->isAncestorOf(parent))
            setParentItem(parent);
        if (d->popupItem->isAncestorOf(parent))
            menuItem = parent;
    }
    d->popup(menuItem);
}

void QQuickMenu::popup(qreal x, qreal y, QQuickItem *menuItem)
{
    popup(QPointF {x, y}, menuItem);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::dismiss()

    Closes all menus in the hierarchy that this menu belongs to.

    \note Unlike \l {Popup::}{close()} that only closes a menu and its
    sub-menus (when using \l {Menu types}{non-native menus}), \c dismiss()
    closes the whole hierarchy of menus, including the parent menus. In
    practice, \c close() is suitable e.g. for implementing navigation in a
    hierarchy of menus, and \c dismiss() is the appropriate method for closing
    the whole hierarchy of menus.

    \sa popup(), Popup::close()
*/
void QQuickMenu::dismiss()
{
    QQuickMenu *menu = this;
    while (menu) {
        menu->close();
        menu = QQuickMenuPrivate::get(menu)->parentMenu;
    }
}

void QQuickMenu::componentComplete()
{
    Q_D(QQuickMenu);
    QQuickPopup::componentComplete();
    d->resizeItems();
    d->updateTextPadding();
    d->syncWithUseNativeMenu();
}

void QQuickMenu::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickMenu);
    QQuickPopup::contentItemChange(newItem, oldItem);

    if (oldItem) {
        QQuickItemPrivate::get(oldItem)->removeItemChangeListener(d, QQuickItemPrivate::Children);
        QQuickItemPrivate::get(oldItem)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
    }
    if (newItem) {
        QQuickItemPrivate::get(newItem)->addItemChangeListener(d, QQuickItemPrivate::Children);
        QQuickItemPrivate::get(newItem)->updateOrAddGeometryChangeListener(d, QQuickGeometryChange::Width);
    }

    d->contentItem = newItem;
}

void QQuickMenu::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    Q_D(QQuickMenu);
    QQuickPopup::itemChange(change, data);

    switch (change) {
    case QQuickItem::ItemVisibleHasChanged:
        if (!data.boolValue && d->cascade) {
            // Ensure that when the menu isn't visible, there's no current item
            // the next time it's opened.
            d->setCurrentIndex(-1, Qt::OtherFocusReason);
        }
        break;
    default:
        break;
    }
}

void QQuickMenu::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickMenu);
    QQuickPopup::keyPressEvent(event);

    // QTBUG-17051
    // Work around the fact that ListView has no way of distinguishing between
    // mouse and keyboard interaction, thanks to the "interactive" bool in Flickable.
    // What we actually want is to have a way to always allow keyboard interaction but
    // only allow flicking with the mouse when there are too many menu items to be
    // shown at once.
    switch (event->key()) {
    case Qt::Key_Up:
        if (!d->activatePreviousItem())
            d->propagateKeyEvent(event);
        break;

    case Qt::Key_Down:
        d->activateNextItem();
        break;

    case Qt::Key_Left:
    case Qt::Key_Right:
        event->ignore();
        if (d->popupItem->isMirrored() == (event->key() == Qt::Key_Right)) {
            if (d->parentMenu && d->currentItem) {
                if (!d->cascade)
                    d->parentMenu->open();
                close();
                event->accept();
            }
        } else {
            if (QQuickMenu *subMenu = d->currentSubMenu()) {
                auto subMenuPrivate = QQuickMenuPrivate::get(subMenu);
                subMenuPrivate->popup(subMenuPrivate->firstEnabledMenuItem());
                event->accept();
            }
        }
        if (!event->isAccepted())
            d->propagateKeyEvent(event);
        break;

#if QT_CONFIG(shortcut)
    case Qt::Key_Alt:
        // If &mnemonic shortcut is enabled, go back to (possibly) the parent
        // menu bar so the shortcut key will be processed by the menu bar.
        if (!QKeySequence::mnemonic(QStringLiteral("&A")).isEmpty())
            close();
        break;
#endif

    default:
        break;
    }

#if QT_CONFIG(shortcut)
    if (event->modifiers() == Qt::NoModifier) {
        for (int i = 0; i < count(); ++i) {
            QQuickAbstractButton *item = qobject_cast<QQuickAbstractButton*>(d->itemAt(i));
            if (!item)
                continue;
            const QKeySequence keySequence = QKeySequence::mnemonic(item->text());
            if (keySequence.isEmpty())
                continue;
            if (keySequence[0].key() == event->key()) {
                item->click();
                break;
            }
        }
    }
#endif
}

void QQuickMenu::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickMenu);
    if (event->timerId() == d->hoverTimer) {
        if (QQuickMenu *subMenu = d->currentSubMenu())
            subMenu->open();
        d->stopHoverTimer();
        return;
    }
    QQuickPopup::timerEvent(event);
}

QFont QQuickMenu::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::Menu);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickMenu::accessibleRole() const
{
    return QAccessible::PopupMenu;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickmenu_p.cpp"
