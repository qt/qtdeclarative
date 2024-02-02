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
//!     \instantiates QQuickMenuBar
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

    \sa {Customizing MenuBar}, Menu, MenuBarItem, {Menu Controls},
        {Focus Management in Qt Quick Controls}
*/

Q_LOGGING_CATEGORY(lcMenuBar, "qt.quick.controls.menubar")

QQuickItem *QQuickMenuBarPrivate::beginCreateItem(QQuickMenu *menu)
{
    Q_Q(QQuickMenuBar);
    if (!delegate)
        return nullptr;

    QQmlContext *context = delegate->creationContext();
    if (!context)
        context = qmlContext(q);

    QObject *object = delegate->beginCreate(context);
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        delete object;
        return nullptr;
    }

    if (QQuickMenuBarItem *menuBarItem = qobject_cast<QQuickMenuBarItem *>(item))
        menuBarItem->setMenu(menu);
    QQml_setParent_noEvent(item, q);

    return item;
}

void QQuickMenuBarPrivate::completeCreateItem()
{
    if (!delegate)
        return;

    delegate->completeCreate();
}

QQuickItem *QQuickMenuBarPrivate::createItem(QQuickMenu *menu)
{
    QQuickItem *item = beginCreateItem(menu);
    completeCreateItem();
    return item;
}

void QQuickMenuBarPrivate::toggleCurrentMenu(bool visible, bool activate)
{
    if (!currentItem || visible == popupMode)
        return;

    QQuickMenu *menu = currentItem->menu();

    triggering = true;
    popupMode = visible;
    if (menu)
        menu->setVisible(visible);
    if (!visible)
        currentItem->forceActiveFocus();
    else if (menu && activate)
        menu->setCurrentIndex(0);
    triggering = false;
}

void QQuickMenuBarPrivate::activateItem(QQuickMenuBarItem *item)
{
    if (currentItem == item)
        return;

    if (currentItem) {
        currentItem->setHighlighted(false);
        if (popupMode) {
            if (QQuickMenu *menu = currentItem->menu())
                menu->dismiss();
        }
    }

    if (item) {
        item->setHighlighted(true);
        if (popupMode) {
            if (QQuickMenu *menu = item->menu())
                menu->open();
        }
    }

    currentItem = item;
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
        toggleCurrentMenu(!popupMode, false);
    } else {
        popupMode = true;
        activateItem(item);
    }
}

void QQuickMenuBarPrivate::onMenuAboutToHide()
{
    if (triggering || !currentItem || !currentItem->isHighlighted())
        return;

    popupMode = false;
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
        QQuickItem *delegateItem = menuBarPriv->createItem(menu);
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
    QQuickMenuBarPrivate::get(menuBar)->contentModel->clear();
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

QPlatformMenuBar* QQuickMenuBarPrivate::nativeHandle() const
{
    return handle.get();
}

void QQuickMenuBarPrivate::insertNativeMenu(QQuickMenu *menu)
{
    Q_Q(QQuickMenuBar);
    Q_ASSERT(handle);
    Q_ASSERT(menu);

    QQuickMenu *insertBefore = nullptr;
    QPlatformMenu *insertBeforeHandle = nullptr;

    // This function assumes that the QQuickMenuBarItem that
    // corresponds to \a menu has already been added to the container
    // at the correct index. So we search for it, to determine where to
    // insert it in the native menubar.
    bool foundInContainer = false;
    for (int i = 0; i < q->count(); ++i) {
        if (q->menuAt(i) != menu)
            continue;

        foundInContainer = true;
        if (i < q->count() - 1) {
            insertBefore = q->menuAt(i + 1);
            insertBeforeHandle = QQuickMenuPrivate::get(insertBefore)->nativeHandle();
        }
        break;
    }

    Q_ASSERT(foundInContainer);
    qCDebug(lcMenuBar) << "insert native menu:" << menu << menu->title()
                       << "before:" << insertBefore << (insertBefore ? insertBefore->title() : QString{});

    QQuickMenuPrivate *menuPrivate = QQuickMenuPrivate::get(menu);
    handle->insertMenu(menuPrivate->nativeHandle(), insertBeforeHandle);
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
}

void QQuickMenuBarPrivate::insertMenu(int index, QQuickMenu *menu, QQuickItem *delegateItem)
{
    Q_Q(QQuickMenuBar);
    if (!menu) {
        qmlWarning(q) << "cannot insert menu: menu is null.";
        return;
    }
    if (!delegateItem) {
        // To be 100% cross-platform, we require that a delegate item is successfully
        // created in order to add a menu to the menubar, even if we use a native
        // menubar. Otherwise an application can end up having a menubar on platforms
        // where native menubars are available, but fail to show one on other platforms.
        qmlWarning(q) << "cannot insert menu: could not create an item from the delegate.";
        return;
    }
    if (!qobject_cast<QQuickMenuBarItem *>(delegateItem)) {
        // We require the delegate to be a MenuBarItem, since we don't store the
        // menus directly in the container, but indirectly using MenuBarItem.menu.
        qmlWarning(q) << "cannot insert menu: the delegate is not a MenuBarItem.";
        return;
    }

    QQuickMenuPrivate::get(menu)->menuBar = q;

    // Always insert menu into the container, even when using a native
    // menubar, so that container API such as 'count' and 'itemAt'
    // continues to work as expected.
    q->insertItem(index, delegateItem);

    if (handle)
        insertNativeMenu(menu);
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

    if (handle)
        removeNativeMenu(menu);

    removeItem(index, item);

    // Delete the MenuBarItem. This will also cause the menu to be deleted by
    // the garbage collector, unless other QML references are being held to it.
    // Note: We might consider leaving it to the garbage collector to also
    // delete the MenuBarItem in the future.
    item->deleteLater();

    QQuickMenuPrivate::get(menu)->menuBar = nullptr;

    return menu;
}

bool QQuickMenuBarPrivate::useNativeMenuBar() const
{
    return requestNative && !QCoreApplication::testAttribute(Qt::AA_DontUseNativeMenuBar);
}

void QQuickMenuBarPrivate::syncNativeMenuBarVisible()
{
    Q_Q(QQuickMenuBar);
    if (!componentComplete)
        return;

    const bool shouldBeVisible = q->isVisible() && useNativeMenuBar();
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
    // because of the QPA API (insertBefore). Note that the container
    // might also contain other, foreign, items added using the generic
    // container API (e.g addItem(item)), which we just ignore.
    for (int i = q->count() - 1; i >= 0; --i) {
        if (QQuickMenu *menu = q->menuAt(i))
            insertNativeMenu(menu);
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

    // Remove all native menus. Note that the container might
    // also contain other, foreign, items added using the generic
    // container API (e.g addItem(item)), which we just ignore.
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
    d->insertMenu(count(), menu, d->createItem(menu));
}

/*!
    \qmlmethod void QtQuick.Controls::MenuBar::insertMenu(int index, Menu menu)

    Inserts \a menu at \a index.
*/
void QQuickMenuBar::insertMenu(int index, QQuickMenu *menu)
{
    Q_D(QQuickMenuBar);
    d->insertMenu(index, menu, d->createItem(menu));
}

/*!
    \qmlmethod void QtQuick.Controls::MenuBar::removeMenu(Menu menu)

    Removes specified \a menu. If the menu is \l {QQuickMenu::popup(QQmlV4Function *)}{open},
    it will first be \l {QQuickMenu::dismiss()}{dismissed.}
    The \a menu will eventually be deleted by the garbage collector when the
    application no longer holds any QML references to it.
*/
void QQuickMenuBar::removeMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenuBar);
    for (int i = 0; i < count(); ++i) {
        QQuickMenuBarItem *item = qobject_cast<QQuickMenuBarItem *>(itemAt(i));
        if (item && item->menu() == menu) {
            d->takeMenu(i);
            return;
        }
    }
}

/*!
    \qmlmethod Menu QtQuick.Controls::MenuBar::takeMenu(int index)

    Removes and returns the menu at \a index. If the menu is
    \l {QQuickMenu::popup(QQmlV4Function *)}{open}, it will first be
    \l {QQuickMenu::dismiss()}{dismissed.}
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
        d->toggleCurrentMenu(false, false);
        break;

    case Qt::Key_Down:
        d->toggleCurrentMenu(true, true);
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
                        d->toggleCurrentMenu(true, true);
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
    if (!d->popupMode && d->currentItem)
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
            QObjectPrivate::connect(menu, &QQuickPopup::aboutToHide, d, &QQuickMenuBarPrivate::onMenuAboutToHide);
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
            QObjectPrivate::disconnect(menu, &QQuickPopup::aboutToHide, d, &QQuickMenuBarPrivate::onMenuAboutToHide);
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

bool QQuickMenuBar::requestNative() const
{
    return d_func()->requestNative;
}

void QQuickMenuBar::setRequestNative(bool requestNative)
{
    Q_D(QQuickMenuBar);
    if (d->requestNative == requestNative)
        return;

    d->requestNative = requestNative;
    d->syncNativeMenuBarVisible();

    emit requestNativeChanged();
}

void QQuickMenuBar::resetRequestNative()
{
    setRequestNative(false);
}

QT_END_NAMESPACE

#include "moc_qquickmenubar_p.cpp"
