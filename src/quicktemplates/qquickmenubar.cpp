// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmenubar_p.h"
#include "qquickmenubar_p_p.h"
#include "qquickmenubaritem_p_p.h"
#include "qquickmenu_p.h"
#include "qquickmenu_p_p.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuBar
    \inherits Container
//!     \nativetype QQuickMenuBar
    \inqmlmodule QtQuick.Controls
    \since 5.10
    \ingroup qtquickcontrols-menus
    \ingroup qtquickcontrols-focusscopes
    \brief Provides a window menu bar.

    \image qtquickcontrols-menubar.png

    MenuBar consists of drop-down menus, and is normally located at the top
    edge of the window.

    \quotefromfile qtquickcontrols-menubar.qml
    \skipuntil begin
    \printto skipfrom
    \skipuntil skipto
    \printto end

    Typically, menus are statically declared as children of the menu bar, but
    MenuBar also provides API to \l {addMenu}{add}, \l {insertMenu}{insert},
    \l {removeMenu}{remove}, and \l {takeMenu}{take} menus dynamically. The
    menus in a menu bar can be accessed using \l menuAt().

    \section1 Native menu bars

    Since Qt 6.8, a MenuBar is implemented as a native menu bar on \macos. As a
    result, all Menus, MenuItems and MenuBarItems within a MenuBar will also be native.
    While this has the advantage that everything will look native, it also comes with the
    disadvantage that the delegates set on the mentioned controls will not be used
    for rendering.
    If a native MenuBar is not wanted, you can set
    \l {Qt::AA_DontUseNativeMenuBar}{QGuiApplication::setAttribute(Qt::AA_DontUseNativeMenuBar)}
    to disable it.

    \sa {Customizing MenuBar}, Menu, MenuBarItem, {Menu Controls},
        {Focus Management in Qt Quick Controls}
*/

Q_STATIC_LOGGING_CATEGORY(lcMenuBar, "qt.quick.controls.menubar")

static const char* kCreatedFromDelegate = "_qt_createdFromDelegate";

QQuickItem *QQuickMenuBarPrivate::createItemFromDelegate()
{
    Q_Q(QQuickMenuBar);
    Q_ASSERT(delegate);
    QQmlContext *context = delegate->creationContext();
    if (!context)
        context = qmlContext(q);

    QObject *object = delegate->beginCreate(context);
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        delete object;
        return nullptr;
    }

    QQml_setParent_noEvent(item, q);
    delegate->completeCreate();

    return item;
}

QQuickMenuBarItem *QQuickMenuBarPrivate::createMenuBarItem(QQuickMenu *menu)
{
    Q_Q(QQuickMenuBar);

    QQuickMenuBarItem *menuBarItem = nullptr;
    if (delegate) {
        QQuickItem *item = createItemFromDelegate();
        menuBarItem = qobject_cast<QQuickMenuBarItem *>(item);
        if (!menuBarItem) {
            qmlWarning(q) << "cannot insert menu: the delegate is not a MenuBarItem.";
            delete item;
        }
    }

    if (!menuBarItem) {
        // When we fail to create a delegate item, create a hidden placeholder
        // instead. This is needed, since we store the menus inside the container
        // using MenuBarItem. And without a MenuBarItem, we would therefore lose
        // the menu, even if the delegate is changed later.
        qCDebug(lcMenuBar) << "creating hidden placeholder MenuBarItem for:" << menu->title();
        menuBarItem = new QQuickMenuBarItem(q);
        menuBarItem->setParentItem(q);
        menuBarItem->setVisible(false);
    }

    menuBarItem->setMenu(menu);

    // Tag the menuBarItem, so that we know which container items to change if the
    // delegate is changed. This is needed since you can add MenuBarItems directly
    // to the menu bar, which should not change when the delegate changes.
    menuBarItem->setProperty(kCreatedFromDelegate, true);

    return menuBarItem;
}

void QQuickMenuBarPrivate::openCurrentMenu()
{
    if (!currentItem || currentMenuOpen)
        return;
    QQuickMenu *menu = currentItem->menu();
    if (!menu || menu->isOpened())
        return;

#ifdef Q_OS_MACOS
    // On macOS, the menu should open underneath the MenuBar
    Q_Q(QQuickMenuBar);
    const QPointF posInParentItem = q->mapToItem(currentItem, {currentItem->x(), q->height()});
#else
    // On other platforms, it should open underneath the MenuBarItem
    const QPointF posInParentItem{0, currentItem->y() + currentItem->height()};
#endif

    // Store explicit if the current menu is logically supposed to be open.
    // menu->isVisible() is async when using top-level menus, and will not become
    // "true" before the menu is actually shown by the OS. This will cause us to
    // lose track of if a menu is (supposed to be) open, if relying on menu->isVisible().
    currentMenuOpen = true;

    // The position should be the coordinate system of the parent item. Note that
    // the parentItem() of a menu will be the MenuBarItem (currentItem), and not the
    // MenuBar (even if parent() usually points to the MenuBar).
    menu->popup(posInParentItem);
}

void QQuickMenuBarPrivate::closeCurrentMenu()
{
    if (!currentItem || !currentMenuOpen)
        return;
    currentMenuOpen = false;
    QQuickMenu *menu = currentItem->menu();
    QScopedValueRollback triggerRollback(closingCurrentMenu, true);
    menu->dismiss();
}

void QQuickMenuBarPrivate::activateMenuItem(int index)
{
    if (!currentItem)
        return;
    QQuickMenu *menu = currentItem->menu();
    if (!menu)
        return;
    menu->setCurrentIndex(index);
}

void QQuickMenuBarPrivate::activateItem(QQuickMenuBarItem *item)
{
    if (currentItem == item)
        return;

    const bool stayOpen = currentMenuOpen;

    if (currentItem) {
        currentItem->setHighlighted(false);
        closeCurrentMenu();
    }

    currentItem = item;

    if (currentItem) {
        currentItem->setHighlighted(true);
        if (stayOpen)
            openCurrentMenu();
    }
}

void QQuickMenuBarPrivate::activateNextItem()
{
    int index = currentItem ? contentModel->indexOf(currentItem, nullptr) : -1;
    if (index >= contentModel->count() - 1)
        index = -1;
    activateItem(qobject_cast<QQuickMenuBarItem *>(itemAt(++index)));
}

void QQuickMenuBarPrivate::activatePreviousItem()
{
    int index = currentItem ? contentModel->indexOf(currentItem, nullptr) : contentModel->count();
    if (index <= 0)
        index = contentModel->count();
    activateItem(qobject_cast<QQuickMenuBarItem *>(itemAt(--index)));
}

void QQuickMenuBarPrivate::onItemHovered()
{
    Q_Q(QQuickMenuBar);
    QQuickMenuBarItem *item = qobject_cast<QQuickMenuBarItem *>(q->sender());
    if (!item || item == currentItem || !item->isHovered() || !item->isEnabled() || QQuickMenuBarItemPrivate::get(item)->touchId != -1)
        return;

    activateItem(item);
}

void QQuickMenuBarPrivate::onItemTriggered()
{
    Q_Q(QQuickMenuBar);
    QQuickMenuBarItem *item = qobject_cast<QQuickMenuBarItem *>(q->sender());
    if (!item)
        return;

    if (item == currentItem) {
        if (currentMenuOpen) {
            closeCurrentMenu();
            currentItem->forceActiveFocus();
        } else {
            openCurrentMenu();
        }
    } else {
        activateItem(item);
        openCurrentMenu();
    }
}

void QQuickMenuBarPrivate::onMenuAboutToHide(QQuickMenu *menu)
{
    if (closingCurrentMenu) {
        // We only react on a menu closing if it's
        // initiated from outside of QQuickMenuBar.
        return;
    }

    if (!currentItem || currentItem->menu() != menu)
        return;

    currentMenuOpen = false;

    if (!currentItem->isHighlighted() || currentItem->isHovered())
        return;

    activateItem(nullptr);
}

qreal QQuickMenuBarPrivate::getContentWidth() const
{
    Q_Q(const QQuickMenuBar);
    const int count = contentModel->count();
    qreal totalWidth = qMax(0, count - 1) * spacing;
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = q->itemAt(i);
        if (item)
            totalWidth += item->implicitWidth();
    }
    return totalWidth;
}

qreal QQuickMenuBarPrivate::getContentHeight() const
{
    Q_Q(const QQuickMenuBar);
    const int count = contentModel->count();
    qreal maxHeight = 0;
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = q->itemAt(i);
        if (item)
            maxHeight = qMax(maxHeight, item->implicitHeight());
    }
    return maxHeight;
}

void QQuickMenuBarPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    QQuickContainerPrivate::itemImplicitWidthChanged(item);
    if (item != contentItem)
        updateImplicitContentWidth();
}

void QQuickMenuBarPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    QQuickContainerPrivate::itemImplicitHeightChanged(item);
    if (item != contentItem)
        updateImplicitContentHeight();
}

void QQuickMenuBarPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    auto menuBar = static_cast<QQuickMenuBar *>(prop->object);
    auto menuBarPriv = QQuickMenuBarPrivate::get(menuBar);

    if (auto *menu = qobject_cast<QQuickMenu *>(obj)) {
        QQuickMenuBarItem *delegateItem = menuBarPriv->createMenuBarItem(menu);
        menuBarPriv->insertMenu(menuBar->count(), menu, delegateItem);
        QQuickContainerPrivate::contentData_append(prop, delegateItem);
        return;
    }

    if (auto *menuBarItem = qobject_cast<QQuickMenuBarItem *>(obj)) {
        menuBarPriv->insertMenu(menuBar->count(), menuBarItem->menu(), menuBarItem);
        QQuickContainerPrivate::contentData_append(prop, menuBarItem);
        return;
    }

    QQuickContainerPrivate::contentData_append(prop, obj);
}

void QQuickMenuBarPrivate::menus_append(QQmlListProperty<QQuickMenu> *prop, QQuickMenu *obj)
{
    // This function is only called if the application assigns a list of menus
    // directly to the 'menus' property. Otherwise, contentData_append is used.
    // Since the functions belonging to the 'menus' list anyway returns data from
    // the menuBar, calls such as "menuBar.menus.length" works as expected
    // regardless of how the menus were added.
    QQuickMenuBar *menuBar = static_cast<QQuickMenuBar *>(prop->object);
    menuBar->addMenu(obj);
}

qsizetype QQuickMenuBarPrivate::menus_count(QQmlListProperty<QQuickMenu> *prop)
{
    QQuickMenuBar *menuBar = static_cast<QQuickMenuBar *>(prop->object);
    return menuBar->count();
}

QQuickMenu *QQuickMenuBarPrivate::menus_at(QQmlListProperty<QQuickMenu> *prop, qsizetype index)
{
    QQuickMenuBar *menuBar = static_cast<QQuickMenuBar *>(prop->object);
    return menuBar->menuAt(index);
}

void QQuickMenuBarPrivate::menus_clear(QQmlListProperty<QQuickMenu> *prop)
{
    QQuickMenuBar *menuBar = static_cast<QQuickMenuBar *>(prop->object);
    for (int count = menuBar->count(); count > 0; count = menuBar->count())
        menuBar->takeMenu(count - 1);
}

QPalette QQuickMenuBarPrivate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::MenuBar);
}

QWindow* QQuickMenuBarPrivate::window() const
{
    Q_Q(const QQuickMenuBar);
    QObject *obj = q->parent();
    while (obj) {
        if (QWindow *window = qobject_cast<QWindow *>(obj))
            return window;
        QQuickItem *item = qobject_cast<QQuickItem *>(obj);
        if (item && item->window())
            return item->window();
        obj = obj->parent();
    }
    return nullptr;
}

int QQuickMenuBarPrivate::menuIndex(QQuickMenu *menu) const
{
    Q_Q(const QQuickMenuBar);
    for (int i = 0; i < q->count(); ++i) {
        if (q->menuAt(i) == menu)
            return i;
    }

    return -1;
}

QPlatformMenuBar* QQuickMenuBarPrivate::nativeHandle() const
{
    return handle.get();
}

void QQuickMenuBarPrivate::insertNativeMenu(QQuickMenu *menu)
{
    Q_Q(QQuickMenuBar);
    Q_ASSERT(handle);
    Q_ASSERT(menu);

    QPlatformMenu *insertBeforeHandle = nullptr;

    // This function assumes that the QQuickMenuBarItem that corresponds to \a menu
    // has already been added to the container at the correct index. So we search for
    // it, to determine where to insert it in the native menubar. Since the QPA API
    // expects a pointer to the QPlatformMenu that comes after it, we need to search
    // for that one as well, since some MenuBarItems in the container can be hidden.
    bool foundInContainer = false;
    for (int i = 0; i < q->count(); ++i) {
        if (q->menuAt(i) != menu)
            continue;
        foundInContainer = true;

        for (int j = i + 1; j < q->count(); ++j) {
            insertBeforeHandle = QQuickMenuPrivate::get(q->menuAt(j))->maybeNativeHandle();
            if (insertBeforeHandle)
                break;
        }

        break;
    }

    Q_ASSERT(foundInContainer);
    QQuickMenuPrivate *menuPrivate = QQuickMenuPrivate::get(menu);
    if (QPlatformMenu *menuHandle = menuPrivate->nativeHandle()) {
        qCDebug(lcMenuBar) << "insert native menu:" << menu->title() << menuHandle << "before:" << insertBeforeHandle;
        handle->insertMenu(menuPrivate->nativeHandle(), insertBeforeHandle);
    } else {
        qmlWarning(q) << "failed to create native menu for:" << menu->title();
    }
}

void QQuickMenuBarPrivate::removeNativeMenu(QQuickMenu *menu)
{
    Q_ASSERT(handle);
    Q_ASSERT(menu);

    QQuickMenuPrivate *menuPrivate = QQuickMenuPrivate::get(menu);
    if (!menuPrivate->maybeNativeHandle())
        return;

    qCDebug(lcMenuBar) << "remove native menu:" << menu << menu->title();
    handle->removeMenu(menuPrivate->nativeHandle());
    menuPrivate->removeNativeMenu();
}

void QQuickMenuBarPrivate::syncMenuBarItemVisibilty(QQuickMenuBarItem *menuBarItem)
{
    if (!handle) {
        // We only need to update visibility on native menu bar items
        return;
    }

    QQuickMenu *menu = menuBarItem->menu();
    if (!menu)
        return;
    QQuickMenuPrivate *menuPrivate = QQuickMenuPrivate::get(menu);

    if (menuBarItem->isVisible()) {
        Q_ASSERT(!menuPrivate->maybeNativeHandle());
        insertNativeMenu(menu);
    } else {
        if (menuPrivate->maybeNativeHandle())
            removeNativeMenu(menu);
    }
}

void QQuickMenuBarPrivate::insertMenu(int index, QQuickMenu *menu, QQuickMenuBarItem *menuBarItem)
{
    Q_Q(QQuickMenuBar);
    if (!menu) {
        qmlWarning(q) << "cannot insert menu: menu is null.";
        return;
    }

    auto menuPrivate = QQuickMenuPrivate::get(menu);
    menuPrivate->menuBar = q;

    QObject::connect(menuBarItem, &QQuickMenuBarItem::visibleChanged, [this, menuBarItem]{
        syncMenuBarItemVisibilty(menuBarItem);
    });

    // Always insert menu into the container, even when using a native
    // menubar, so that container API such as 'count' and 'itemAt'
    // continues to work as expected.
    q->insertItem(index, menuBarItem);

    // Create or remove a native (QPlatformMenu) menu. Note that we should only create
    // a native menu if it's supposed to be visible in the menu bar.
    if (menuBarItem->isVisible()) {
        if (handle)
            insertNativeMenu(menu);
    } else {
        if (menuPrivate->maybeNativeHandle()) {
            // If the menu was added from an explicit call to addMenu(m), it will have been
            // created before we enter here. And in that case, QQuickMenuBar::useNativeMenu(m)
            // was never called, and a QPlatformMenu might have been created for it. In that
            // case, we remove it again now, since the menu is not supposed to be visible in
            // the menu bar.
            menuPrivate->removeNativeMenu();
        }
    }
}

QQuickMenu *QQuickMenuBarPrivate::takeMenu(int index)
{
    Q_Q(QQuickMenuBar);
    QQuickItem *item = q->itemAt(index);
    Q_ASSERT(item);
    QQuickMenuBarItem *menuBarItem = qobject_cast<QQuickMenuBarItem *>(item);
    if (!menuBarItem) {
        qmlWarning(q) << "cannot take/remove menu: item at index " << index << " is not a MenuBarItem.";
        return nullptr;
    }
    QQuickMenu *menu = menuBarItem->menu();
    if (!menu) {
        qmlWarning(q) << "cannot take/remove menu: MenuBarItem.menu at index " << index << " is null.";
        return nullptr;
    }

    // Dismiss the menu if it's open. Otherwise, when we now remove it from
    // the menubar, it will stay open without the user being able to dismiss
    // it (at least if it's non-native).
    menu->dismiss();

    if (item == currentItem)
        activateItem(nullptr);

    if (QQuickMenuPrivate::get(menu)->maybeNativeHandle())
        removeNativeMenu(menu);

    removeItem(index, item);

    // Delete the MenuBarItem. This will also cause the menu to be deleted by
    // the garbage collector, unless other QML references are being held to it.
    // Note: We might consider leaving it to the garbage collector to also
    // delete the MenuBarItem in the future.
    item->deleteLater();

    QQuickMenuPrivate::get(menu)->menuBar = nullptr;
    menuBarItem->disconnect(q);

    return menu;
}

bool QQuickMenuBarPrivate::useNativeMenuBar() const
{
    // We current only use native menu bars on macOS. Especially, the
    // QPA menu bar for Windows is old and unused, and looks broken and non-native.
#ifdef Q_OS_MACOS
    return !QCoreApplication::testAttribute(Qt::AA_DontUseNativeMenuBar);
#else
    return false;
#endif
}

bool QQuickMenuBarPrivate::useNativeMenu(const QQuickMenu *menu) const
{
    Q_Q(const QQuickMenuBar);
    if (!useNativeMenuBar())
        return false;

    // Since we cannot hide a QPlatformMenu, we have to avoid
    // creating it if it shouldn't be visible in the menu bar.
    for (int i = 0; i < q->count(); ++i) {
        if (q->menuAt(i) == menu) {
            QQuickItem *itemAtI = itemAt(i);
            return itemAtI && itemAtI->isVisible();
        }
    }

    return true;
}

void QQuickMenuBarPrivate::syncNativeMenuBarVisible()
{
    Q_Q(QQuickMenuBar);
    if (!componentComplete)
        return;

    const bool shouldBeVisible = q->isVisible() && useNativeMenuBar();
    qCDebug(lcMenuBar) << "syncNativeMenuBarVisible called - q->isVisible()" << q->isVisible()
        << "useNativeMenuBar()" << useNativeMenuBar() << "handle" << handle.get();
    if (shouldBeVisible && !handle)
        createNativeMenuBar();
    else if (!shouldBeVisible && handle)
        removeNativeMenuBar();
}

void QQuickMenuBarPrivate::createNativeMenuBar()
{
    Q_Q(QQuickMenuBar);
    Q_ASSERT(!handle);
    qCDebug(lcMenuBar) << "creating native menubar";

    handle.reset(QGuiApplicationPrivate::platformTheme()->createPlatformMenuBar());
    if (!handle) {
        qCDebug(lcMenuBar) << "QPlatformTheme failed to create a QPlatformMenuBar!";
        return;
    }

    handle->handleReparent(window());
    qCDebug(lcMenuBar) << "native menubar parented to window:" << handle->parentWindow();

    // Add all the native menus. We need to do this right-to-left
    // because of the QPA API (insertBefore).
    for (int i = q->count() - 1; i >= 0; --i) {
        if (QQuickMenu *menu = q->menuAt(i)) {
            if (useNativeMenu(menu))
                insertNativeMenu(menu);
        }
    }

    // Hide the non-native menubar and set it's height to 0. The
    // latter will cause a relayout to happen in ApplicationWindow
    // which effectively removes the menubar from the contentItem.
     setCulled(true);
     q->setHeight(0);
}

void QQuickMenuBarPrivate::removeNativeMenuBar()
{
    Q_Q(QQuickMenuBar);
    Q_ASSERT(handle);
    qCDebug(lcMenuBar) << "removing native menubar";

    // Remove all native menus.
    for (int i = 0; i < q->count(); ++i) {
        if (QQuickMenu *menu = q->menuAt(i))
            removeNativeMenu(menu);
    }

    // Delete the menubar
    handle.reset();

    // Show the non-native menubar and reset it's height. The
    // latter will cause a relayout to happen in ApplicationWindow
    // which will effectively add the menubar to the contentItem.
    setCulled(false);
    q->resetHeight();
}

QQuickMenuBar::QQuickMenuBar(QQuickItem *parent)
    : QQuickContainer(*(new QQuickMenuBarPrivate), parent)
{
    Q_D(QQuickMenuBar);
    d->changeTypes |= QQuickItemPrivate::Geometry;
    setFlag(ItemIsFocusScope);
    setFocusPolicy(Qt::ClickFocus);
}

QQuickMenuBar::~QQuickMenuBar()
{
    Q_D(QQuickMenuBar);
    if (d->handle)
        d->removeNativeMenuBar();
}

/*!
    \qmlproperty Component QtQuick.Controls::MenuBar::delegate

    This property holds the component that is used to create menu bar
    items to present menus in the menu bar.

    \sa MenuBarItem
*/
QQmlComponent *QQuickMenuBar::delegate() const
{
    Q_D(const QQuickMenuBar);
    return d->delegate;
}

void QQuickMenuBar::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickMenuBar);
    if (d->delegate == delegate)
        return;

    d->delegate = delegate;

    for (int i = count() - 1; i >= 0; --i) {
        auto item = itemAt(i);
        if (!item || !item->property(kCreatedFromDelegate).toBool())
            continue;

        QQuickMenuBarItem *menuBarItem = static_cast<QQuickMenuBarItem *>(item);
        if (QQuickMenu *menu = menuBarItem->menu()) {
            removeMenu(menu);
            d->insertMenu(i, menu, d->createMenuBarItem(menu));
        } else {
            removeItem(menuBarItem);
        }
    }

    emit delegateChanged();
}

/*!
    \qmlmethod Menu QtQuick.Controls::MenuBar::menuAt(int index)

    Returns the menu at \a index, or \c null if it does not exist.
*/
QQuickMenu *QQuickMenuBar::menuAt(int index) const
{
    Q_D(const QQuickMenuBar);
    QQuickMenuBarItem *item = qobject_cast<QQuickMenuBarItem *>(d->itemAt(index));
    if (!item)
        return nullptr;
    return item->menu();
}

/*!
    \qmlmethod void QtQuick.Controls::MenuBar::addMenu(Menu menu)

    Adds \a menu to the end of the list of menus.
*/
void QQuickMenuBar::addMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenuBar);
    if (d->menuIndex(menu) >= 0) {
        qmlWarning(this) << "cannot add menu: '" << menu->title() << "' is already in the MenuBar.";
        return;
    }

    d->insertMenu(count(), menu, d->createMenuBarItem(menu));
}

/*!
    \qmlmethod void QtQuick.Controls::MenuBar::insertMenu(int index, Menu menu)

    Inserts \a menu at \a index.
*/
void QQuickMenuBar::insertMenu(int index, QQuickMenu *menu)
{
    Q_D(QQuickMenuBar);
    if (d->menuIndex(menu) >= 0) {
        qmlWarning(this) << "cannot insert menu: '" << menu->title() << "' is already in the MenuBar.";
        return;
    }

    d->insertMenu(index, menu, d->createMenuBarItem(menu));
}

/*!
    \qmlmethod void QtQuick.Controls::MenuBar::removeMenu(Menu menu)

    Removes specified \a menu. If the menu is \l {Menu::popup()}{open},
    it will first be \l {Menu::dismiss()}{dismissed}.
    The \a menu will eventually be deleted by the garbage collector when the
    application no longer holds any QML references to it.
*/
void QQuickMenuBar::removeMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenuBar);
    const int index = d->menuIndex(menu);
    if (index < 0) {
        qmlWarning(this) << "cannot remove menu: '" << menu->title() << "' is not in the MenuBar.";
        return;
    }

    d->takeMenu(index);
}

/*!
    \qmlmethod Menu QtQuick.Controls::MenuBar::takeMenu(int index)

    Removes and returns the menu at \a index. If the menu is
    \l {Menu::popup()}{open}, it will first be
    \l {Menu::dismiss()}{dismissed}.
    The menu will eventually be deleted by the garbage collector when the
    application no longer holds any QML references to it.
*/
QQuickMenu *QQuickMenuBar::takeMenu(int index)
{
    Q_D(QQuickMenuBar);
    if (index < 0 || index > count() - 1) {
        qmlWarning(this) << "index out of range: " << index;
        return nullptr;
    }

    return d->takeMenu(index);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty real QtQuick.Controls::MenuBar::contentWidth

    This property holds the content width. It is used for calculating the total
    implicit width of the menu bar.

    \note This property is available in MenuBar since QtQuick.Controls 2.3 (Qt 5.10),
    but it was promoted to the Container base type in QtQuick.Controls 2.5 (Qt 5.12).

    \sa Container::contentWidth
*/

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty real QtQuick.Controls::MenuBar::contentHeight

    This property holds the content height. It is used for calculating the total
    implicit height of the menu bar.

    \note This property is available in MenuBar since QtQuick.Controls 2.3 (Qt 5.10),
    but it was promoted to the Container base type in QtQuick.Controls 2.5 (Qt 5.12).

    \sa Container::contentHeight
*/

/*!
    \qmlproperty list<Menu> QtQuick.Controls::MenuBar::menus

    This property holds the list of menus.

    The list contains all menus that have been declared in QML as children
    of the menu bar, and also menus that have been dynamically added or
    inserted using the \l addMenu() and \l insertMenu() methods, respectively.
*/
QQmlListProperty<QQuickMenu> QQuickMenuBarPrivate::menus()
{
    Q_Q(QQuickMenuBar);
    return QQmlListProperty<QQuickMenu>(q, nullptr,
                                        QQuickMenuBarPrivate::menus_append,
                                        QQuickMenuBarPrivate::menus_count,
                                        QQuickMenuBarPrivate::menus_at,
                                        QQuickMenuBarPrivate::menus_clear);
}

QQmlListProperty<QObject> QQuickMenuBarPrivate::contentData()
{
    Q_Q(QQuickMenuBar);
    return QQmlListProperty<QObject>(q, nullptr,
                                     QQuickMenuBarPrivate::contentData_append,
                                     QQuickContainerPrivate::contentData_count,
                                     QQuickContainerPrivate::contentData_at,
                                     QQuickContainerPrivate::contentData_clear);
}

bool QQuickMenuBar::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QQuickMenuBar);

    if (d->altPressed) {
        switch (event->type()) {
        case QEvent::KeyRelease: {
            const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
            if ((keyEvent->key() == Qt::Key_Alt || keyEvent->key() == Qt::Key_Meta)
                && keyEvent->modifiers() == Qt::NoModifier) {
                for (int i = 0; i < count(); ++i) {
                    if (auto *item = qobject_cast<QQuickMenuBarItem *>(d->itemAt(i))) {
                        d->activateItem(item);
                        setFocusReason(Qt::MenuBarFocusReason);
                        setFocus(true);
                        break;
                    }
                }
            }
            Q_FALLTHROUGH();
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::TabletPress:
        case QEvent::TabletMove:
        case QEvent::TabletRelease:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::ActivationChange:
        case QEvent::Shortcut:
            d->altPressed = false;
            qApp->removeEventFilter(this);
            break;
        default:
            break;
        }
    } else if (isVisible() && event->type() == QEvent::ShortcutOverride) {
        const bool altKeyNavigation = QGuiApplicationPrivate::platformTheme()
                                    ->themeHint(QPlatformTheme::MenuBarFocusOnAltPressRelease).toBool();
        if (altKeyNavigation) {
            const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
            if ((keyEvent->key() == Qt::Key_Alt || keyEvent->key() == Qt::Key_Meta)
                && keyEvent->modifiers() == Qt::AltModifier) {
                d->altPressed = true;
                qApp->installEventFilter(this);
            }
        }
    }
    return QObject::eventFilter(object, event);
}

void QQuickMenuBar::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::keyReleaseEvent(event);

    switch (event->key()) {
    case Qt::Key_Up:
        d->closeCurrentMenu();
        break;

    case Qt::Key_Down:
        d->openCurrentMenu();
        d->activateMenuItem(0);
        break;

    case Qt::Key_Left:
    case Qt::Key_Right:
        if (isMirrored() == (event->key() == Qt::Key_Left))
            d->activateNextItem();
        else
            d->activatePreviousItem();
        break;
    // This is triggered when no popup is open but a menu bar item is highlighted and has focus.
    case Qt::Key_Escape:
        if (d->currentItem) {
            d->activateItem(nullptr);
            setFocus(false);
        }
        break;
    default:
#if QT_CONFIG(shortcut)
        if (!event->text().isEmpty() && event->modifiers() == Qt::NoModifier) {
            const QKeyCombination mnemonic(Qt::AltModifier, Qt::Key(event->key()));
            for (int i = 0; i < count(); ++i) {
                if (auto *item = qobject_cast<QQuickMenuBarItem *>(d->itemAt(i))) {
                    if (item->shortcut() == mnemonic) {
                        d->activateItem(item);
                        d->openCurrentMenu();
                        d->activateMenuItem(0);
                    }
                }
            }
        }
#endif
        break;
    }
}

void QQuickMenuBar::keyReleaseEvent(QKeyEvent *event)
{
    QQuickContainer::keyReleaseEvent(event);

    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Escape:
        event->accept();
        break;

    default:
        event->ignore();
        break;
    }
}

void QQuickMenuBar::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::hoverLeaveEvent(event);
    if (!d->currentMenuOpen && d->currentItem)
        d->activateItem(nullptr);
}

bool QQuickMenuBar::isContent(QQuickItem *item) const
{
    return qobject_cast<QQuickMenuBarItem *>(item);
}

void QQuickMenuBar::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::itemChange(change, value);
    switch (change) {
    case ItemSceneChange:
        if (d->windowContentItem)
            d->windowContentItem->removeEventFilter(this);
        if (value.window) {
            d->windowContentItem = value.window->contentItem();
            if (d->windowContentItem)
                d->windowContentItem->installEventFilter(this);
        }
        break;
    case ItemVisibleHasChanged:
        qCDebug(lcMenuBar) << "visibility of" << this << "changed to" << isVisible();
        d->syncNativeMenuBarVisible();
        break;
    default:
        break;
    }
}

void QQuickMenuBar::itemAdded(int index, QQuickItem *item)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::itemAdded(index, item);
    if (QQuickMenuBarItem *menuBarItem = qobject_cast<QQuickMenuBarItem *>(item)) {
        QQuickMenuBarItemPrivate::get(menuBarItem)->setMenuBar(this);
        QObjectPrivate::connect(menuBarItem, &QQuickControl::hoveredChanged, d, &QQuickMenuBarPrivate::onItemHovered);
        QObjectPrivate::connect(menuBarItem, &QQuickMenuBarItem::triggered, d, &QQuickMenuBarPrivate::onItemTriggered);
        if (QQuickMenu *menu = menuBarItem->menu())
            connect(menu, &QQuickPopup::aboutToHide, [this, menu]{ d_func()->onMenuAboutToHide(menu); });
    }
    d->updateImplicitContentSize();
    emit menusChanged();
}

void QQuickMenuBar::itemMoved(int index, QQuickItem *item)
{
    QQuickContainer::itemMoved(index, item);
    emit menusChanged();
}

void QQuickMenuBar::itemRemoved(int index, QQuickItem *item)
{
    Q_D(QQuickMenuBar);
    QQuickContainer::itemRemoved(index, item);
    if (QQuickMenuBarItem *menuBarItem = qobject_cast<QQuickMenuBarItem *>(item)) {
        QQuickMenuBarItemPrivate::get(menuBarItem)->setMenuBar(nullptr);
        QObjectPrivate::disconnect(menuBarItem, &QQuickControl::hoveredChanged, d, &QQuickMenuBarPrivate::onItemHovered);
        QObjectPrivate::disconnect(menuBarItem, &QQuickMenuBarItem::triggered, d, &QQuickMenuBarPrivate::onItemTriggered);
        if (QQuickMenu *menu = menuBarItem->menu())
            menu->disconnect(this);
    }
    d->updateImplicitContentSize();
    emit menusChanged();
}

void QQuickMenuBar::componentComplete()
{
    Q_D(QQuickMenuBar);
    QQuickContainer::componentComplete();
    d->syncNativeMenuBarVisible();
}

QFont QQuickMenuBar::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::MenuBar);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickMenuBar::accessibleRole() const
{
    return QAccessible::MenuBar;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickmenubar_p.cpp"
