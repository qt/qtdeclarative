// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickapplicationwindow_p.h"
#include "qquickapplicationwindow_p_p.h"
#include "qquickpopup_p_p.h"
#include "qquickcontrol_p_p.h"
#include "qquicktemplatesutils_p.h"
#include "qquicktoolbar_p.h"
#include <private/qtquicktemplates2-config_p.h>
#if QT_CONFIG(quicktemplates2_container)
#include "qquicktabbar_p.h"
#include "qquickdialogbuttonbox_p.h"
#endif
#include "qquickdeferredexecute_p_p.h"
#include "qquickdeferredpointer_p_p.h"

#include <QtCore/private/qobject_p.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtQml/private/qqmlpropertytopropertybinding_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicksafearea_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquickwindowmodule_p_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \qmltype ApplicationWindow
    \inherits Window
//!     \nativetype QQuickApplicationWindow
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-containers
    \ingroup qtquickcontrols-focusscopes
    \brief Styled top-level window with support for a header and footer.

    ApplicationWindow is a \l Window which makes it convenient to add
    a \l {menuBar}{menu bar}, \l header and \l footer item to the window.

    You can declare ApplicationWindow as the root item of your application,
    and run it by using \l QQmlApplicationEngine.  In this way you can control
    the window's properties, appearance and layout from QML.

    \image qtquickcontrols-applicationwindow-wireframe.png
           {Window layout showing menu bar, header, content area, and footer}

    \qml
    import QtQuick.Controls

    ApplicationWindow {
        visible: true

        menuBar: MenuBar {
            // ...
        }

        header: ToolBar {
            // ...
        }

        footer: TabBar {
            // ...
        }

        StackView {
            anchors.fill: parent
        }
    }
    \endqml

    \note By default, an ApplicationWindow is not visible.

    \section2 Attached ApplicationWindow Properties

    Due to how \l {Scope and Naming Resolution} works in QML, it is possible
    to reference the \c id of the application root element anywhere in its
    child QML objects. Even though this approach is fine for many applications
    and use cases, for a generic QML component it may not be acceptable as it
    creates a dependency to the surrounding environment.

    ApplicationWindow provides a set of attached properties that can be used
    to access the window and its building blocks from places where no direct
    access to the window is available, without creating a dependency to a
    certain window \c id. A QML component that uses the ApplicationWindow
    attached properties works in any window regardless of its \c id.

    \section2 Safe Areas

    Since Qt 6.9 ApplicationWindow will automatically add padding to the
    contentItem for any \l{SafeArea} {safe area margins} reported by the
    window. This ensures that the contentItem stays inside the safe area
    of the window, while the background item covers the entire window.

    If you are manually handing safe area margins in the window's contentItem
    you can override the default via the topPadding, leftPadding, rightPadding
    and bottomPadding properties:

    \snippet qtquickcontrols-appwindow-safeareas.qml 0
    \snippet qtquickcontrols-appwindow-safeareas.qml 1
    \snippet qtquickcontrols-appwindow-safeareas.qml 2

    The \l{header}, \l{footer}, and \l{menuBar} properties do not receive
    any automatic padding for the safe area margins. However, depending on
    the style in use, the style may take safe areas into account in its
    implementation of ToolBar, TabBar, and MenuBar.

    \sa {Customizing ApplicationWindow}, Overlay, Page, {Container Controls},
        {Focus Management in Qt Quick Controls}
*/

static const QQuickItemPrivate::ChangeTypes ItemChanges = QQuickItemPrivate::Visibility
        | QQuickItemPrivate::Geometry | QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight;

static void layoutItem(QQuickItem *item, qreal y, qreal width)
{
    if (!item)
        return;

    item->setY(y);
    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    if (!p->widthValid()) {
        item->setWidth(width);
        p->widthValidFlag = false;
    }
}

void QQuickApplicationWindowPrivate::updateHasBackgroundFlags()
{
    if (!background)
        return;

    QQuickItemPrivate *backgroundPrivate = QQuickItemPrivate::get(background);
    hasBackgroundWidth = backgroundPrivate->widthValid();
    hasBackgroundHeight = backgroundPrivate->heightValid();
}

void QQuickApplicationWindowPrivate::relayout()
{
    Q_Q(QQuickApplicationWindow);
    if (!componentComplete)
        return;

    // Note: We track whether we are inside relayout, but we do
    // allow nested relayouts, as those are necessary to compute
    // the height and position of footers when using safe areas.
    QScopedValueRollback<bool> guard(insideRelayout, true);

    // Re-evaluate component heights for each use, as they
    // may change between each use due to recursive layouts.
    auto menuBarHeight = [this]{ return menuBar && menuBar->isVisible() ? menuBar->height() : 0; };
    auto headerheight = [this]{ return header && header->isVisible() ? header->height() : 0; };
    auto footerHeight = [this]{ return footer && footer->isVisible() ? footer->height() : 0; };

    control->setSize(q->size());

    layoutItem(menuBar, 0, q->width());
    layoutItem(header, menuBarHeight(), q->width());
    layoutItem(footer, control->height() - footerHeight(), q->width());

    if (background) {
        if (!hasBackgroundWidth && qFuzzyIsNull(background->x()))
            background->setWidth(q->width());
        if (!hasBackgroundHeight && qFuzzyIsNull(background->y()))
            background->setHeight(q->height());
    }

    // Install additional margins on the control, which get reflected
    // to the content item, but not to the header/footer/menuBar, as
    // these are siblings of the control
    auto *controlSafeArea = static_cast<QQuickSafeArea*>(qmlAttachedPropertiesObject<QQuickSafeArea>(control));
    auto *windowSafeArea = static_cast<QQuickSafeArea*>(qmlAttachedPropertiesObject<QQuickSafeArea>(q));
    const auto inheritedMargins = windowSafeArea->margins();
    controlSafeArea->setAdditionalMargins(QMarginsF(
        0, (menuBarHeight() + headerheight()) - inheritedMargins.top(),
        0, footerHeight() - inheritedMargins.bottom()));
}

void QQuickApplicationWindowPrivate::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff)
{
    Q_UNUSED(diff);

    if (!insideRelayout && item == background && change.sizeChange()) {
        // Any time the background is resized (excluding our own resizing),
        // we should respect it if it's explicit by storing the values of the flags.
        updateHasBackgroundFlags();
    }

    relayout();
}

void QQuickApplicationWindowPrivate::itemVisibilityChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickApplicationWindowPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickApplicationWindowPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickApplicationWindowPrivate::updateFont(const QFont &f)
{
    Q_Q(QQuickApplicationWindow);
    const bool changed = font != f;
    font = f;

    QQuickControlPrivate::updateFontRecur(q->QQuickWindow::contentItem(), f);

    const QList<QQuickPopup *> popups = q->findChildren<QQuickPopup *>();
    for (QQuickPopup *popup : popups)
        QQuickControlPrivate::get(static_cast<QQuickControl *>(popup->popupItem()))->inheritFont(f);

    if (changed)
        emit q->fontChanged();
}

void QQuickApplicationWindowPrivate::resolveFont()
{
    QFont resolvedFont = font.resolve(QQuickTheme::font(QQuickTheme::System));
    setFont_helper(resolvedFont);
}

static QQuickItem *findActiveFocusControl(QQuickWindow *window)
{
    auto *appWindow = qobject_cast<QQuickApplicationWindow *>(window);
    auto *appWindowPriv = appWindow ? QQuickApplicationWindowPrivate::get(appWindow) : nullptr;

    QQuickItem *item = window->activeFocusItem();
    while (item) {
        // The content control is an implementation detail and if
        // we hit it we've hit the root item, so no controls can exist
        if (appWindow && item == appWindowPriv->control)
            return nullptr;
        if (QQuickTemplatesUtils::isInteractiveControlType(item))
            return item;
        item = item->parentItem();
    }
    return item;
}

void QQuickApplicationWindowPrivate::_q_updateActiveFocus()
{
    Q_Q(QQuickApplicationWindow);
    setActiveFocusControl(findActiveFocusControl(q));
}

void QQuickApplicationWindowPrivate::setActiveFocusControl(QQuickItem *control)
{
    Q_Q(QQuickApplicationWindow);
    if (activeFocusControl != control) {
        activeFocusControl = control;
        emit q->activeFocusControlChanged();
    }
}

void QQuickApplicationWindowPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickItemPrivate::data_append(prop, obj);

    // associate "top-level" popups with the window as soon as they are added to the default property
    if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(obj))
        QQuickPopupPrivate::get(popup)->setWindow(static_cast<QQuickApplicationWindow *>(prop->data));
}

void QQuickApplicationWindowPrivate::cancelBackground()
{
    Q_Q(QQuickApplicationWindow);
    quickCancelDeferred(q, backgroundName());
}

void QQuickApplicationWindowPrivate::executeBackground(bool complete)
{
    Q_Q(QQuickApplicationWindow);
    if (background.wasExecuted())
        return;

    if (!background || complete)
        quickBeginDeferred(q, backgroundName(), background);
    if (complete) {
        quickCompleteDeferred(q, backgroundName(), background);
        // See comment in setBackground for why we do this here.
        updateHasBackgroundFlags();
        relayout();
    }
}

QQuickApplicationWindow::QQuickApplicationWindow(QWindow *parent)
    : QQuickWindowQmlImpl(*(new QQuickApplicationWindowPrivate), parent)
{
    connect(this, SIGNAL(activeFocusItemChanged()), this, SLOT(_q_updateActiveFocus()));
}

QQuickApplicationWindow::~QQuickApplicationWindow()
{
    Q_D(QQuickApplicationWindow);
    d->setActiveFocusControl(nullptr);
    disconnect(this, SIGNAL(activeFocusItemChanged()), this, SLOT(_q_updateActiveFocus()));
    if (d->menuBar)
        QQuickItemPrivate::get(d->menuBar)->removeItemChangeListener(d, ItemChanges);
    if (d->header)
        QQuickItemPrivate::get(d->header)->removeItemChangeListener(d, ItemChanges);
    if (d->footer)
        QQuickItemPrivate::get(d->footer)->removeItemChangeListener(d, ItemChanges);
}

QQuickApplicationWindowAttached *QQuickApplicationWindow::qmlAttachedProperties(QObject *object)
{
    return new QQuickApplicationWindowAttached(object);
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::background

    This property holds the background item.

    The background item is stacked under the \l {contentItem}{content item},
    but above the \l {Window::color}{background color} of the window.

    The background item is useful for images and gradients, for example,
    but the \l {Window::}{color} property is preferable for solid colors,
    as it doesn't need to create an item.

    \note If the background item has no explicit size specified, it automatically
          follows the control's size. In most cases, there is no need to specify
          width or height for a background item.

    \sa {Customizing ApplicationWindow}, contentItem, header, footer
*/
QQuickItem *QQuickApplicationWindow::background() const
{
    QQuickApplicationWindowPrivate *d = const_cast<QQuickApplicationWindowPrivate *>(d_func());
    if (!d->background)
        d->executeBackground();
    return d->background;
}

void QQuickApplicationWindow::setBackground(QQuickItem *background)
{
    Q_D(QQuickApplicationWindow);
    if (d->background == background)
        return;

    if (!d->background.isExecuting())
        d->cancelBackground();

    if (d->background) {
        d->hasBackgroundWidth = false;
        d->hasBackgroundHeight = false;
    }
    QQuickControlPrivate::hideOldItem(d->background);

    d->background = background;

    if (background) {
        background->setParentItem(QQuickWindow::contentItem());

        if (qFuzzyIsNull(background->z()))
            background->setZ(-1);

        // If the background hasn't finished executing then we don't know if its width and height
        // are valid or not, and so relayout would see that they haven't been set yet and override
        // any bindings the user might have.
        if (!d->background.isExecuting()) {
            d->updateHasBackgroundFlags();

            if (isComponentComplete())
                d->relayout();
        }
    }
    if (!d->background.isExecuting())
        emit backgroundChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::header

    This property holds the window header item. The header item is positioned at the
    top of the window, below the menu bar, and resized to the width of the window.
    The default value is \c null.

    \code
    ApplicationWindow {
        header: TabBar {
            // ...
        }
    }
    \endcode

    \note Assigning a ToolBar, TabBar, or DialogButtonBox as a window header
    automatically sets the respective \l ToolBar::position, \l TabBar::position,
    or \l DialogButtonBox::position property to \c Header.

    \sa menuBar, footer, Page::header
*/
QQuickItem *QQuickApplicationWindow::header() const
{
    Q_D(const QQuickApplicationWindow);
    return d->header;
}

void QQuickApplicationWindow::setHeader(QQuickItem *header)
{
    Q_D(QQuickApplicationWindow);
    if (d->header == header)
        return;

    if (d->header) {
        QQuickItemPrivate::get(d->header)->removeItemChangeListener(d, ItemChanges);
        d->header->setParentItem(nullptr);
    }
    d->header = header;
    if (header) {
        header->setParentItem(QQuickWindow::contentItem());
        QQuickItemPrivate *p = QQuickItemPrivate::get(header);
        p->addItemChangeListener(d, ItemChanges);
        if (qFuzzyIsNull(header->z()))
            header->setZ(1);
        if (QQuickToolBar *toolBar = qobject_cast<QQuickToolBar *>(header))
            toolBar->setPosition(QQuickToolBar::Header);
#if QT_CONFIG(quicktemplates2_container)
        else if (QQuickTabBar *tabBar = qobject_cast<QQuickTabBar *>(header))
            tabBar->setPosition(QQuickTabBar::Header);
        else if (QQuickDialogButtonBox *buttonBox = qobject_cast<QQuickDialogButtonBox *>(header))
            buttonBox->setPosition(QQuickDialogButtonBox::Header);
#endif

        header->stackBefore(d->control);
    }
    if (isComponentComplete())
        d->relayout();
    emit headerChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::footer

    This property holds the window footer item. The footer item is positioned to
    the bottom, and resized to the width of the window. The default value is \c null.

    \code
    ApplicationWindow {
        footer: ToolBar {
            // ...
        }
    }
    \endcode

    \note Assigning a ToolBar, TabBar, or DialogButtonBox as a window footer
    automatically sets the respective \l ToolBar::position, \l TabBar::position,
    or \l DialogButtonBox::position property to \c Footer.

    \sa menuBar, header, Page::footer
*/
QQuickItem *QQuickApplicationWindow::footer() const
{
    Q_D(const QQuickApplicationWindow);
    return d->footer;
}

void QQuickApplicationWindow::setFooter(QQuickItem *footer)
{
    Q_D(QQuickApplicationWindow);
    if (d->footer == footer)
        return;

    if (d->footer) {
        QQuickItemPrivate::get(d->footer)->removeItemChangeListener(d, ItemChanges);
        d->footer->setParentItem(nullptr);
    }
    d->footer = footer;
    if (footer) {
        footer->setParentItem(QQuickWindow::contentItem());
        QQuickItemPrivate *p = QQuickItemPrivate::get(footer);
        p->addItemChangeListener(d, ItemChanges);
        if (qFuzzyIsNull(footer->z()))
            footer->setZ(1);
        if (QQuickToolBar *toolBar = qobject_cast<QQuickToolBar *>(footer))
            toolBar->setPosition(QQuickToolBar::Footer);
#if QT_CONFIG(quicktemplates2_container)
        else if (QQuickTabBar *tabBar = qobject_cast<QQuickTabBar *>(footer))
            tabBar->setPosition(QQuickTabBar::Footer);
        else if (QQuickDialogButtonBox *buttonBox = qobject_cast<QQuickDialogButtonBox *>(footer))
            buttonBox->setPosition(QQuickDialogButtonBox::Footer);

        footer->stackAfter(d->control);
#endif
    }
    if (isComponentComplete())
        d->relayout();
    emit footerChanged();
}

/*!
    \qmlproperty list<QtObject> QtQuick.Controls::ApplicationWindow::contentData
    \qmldefault

    This default property holds the list of all objects declared as children of
    the window.

    The data property allows you to freely mix visual children, resources and
    other windows in an ApplicationWindow.

    If you assign an Item to the contentData list, it becomes a child of the
    window's contentItem, so that it appears inside the window. The item's
    parent will be the window's \l contentItem.

    It should not generally be necessary to refer to the contentData property,
    as it is the default property for ApplicationWindow and thus all child
    items are automatically assigned to this property.

    \sa contentItem
*/
QQmlListProperty<QObject> QQuickApplicationWindowPrivate::contentData()
{
    Q_Q(QQuickApplicationWindow);
    return QQmlListProperty<QObject>(q->contentItem(), q,
                                     QQuickApplicationWindowPrivate::contentData_append,
                                     QQuickItemPrivate::data_count,
                                     QQuickItemPrivate::data_at,
                                     QQuickItemPrivate::data_clear);
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::contentItem
    \readonly

    This property holds the window content item.

    The content item is stacked above the \l background item, and under the
    \l menuBar, \l header, and \l footer items.

    Since Qt 6.9 ApplicationWindow will automatically add padding to the
    contentItem for any \l{SafeArea} {safe area margins} reported by the
    window. To override the padding use the individual padding properties.

    \sa background, menuBar, header, footer,
    topPadding, bottomPadding, leftPadding, rightPadding
*/
QQuickItem *QQuickApplicationWindow::contentItem() const
{
    Q_D(const QQuickApplicationWindow);
    return d->control->contentItem();
}

/*!
    \qmlproperty real QtQuick.Controls::ApplicationWindow::topPadding
    \since 6.9

    This property holds the top padding of the window's content item.
    Unless explicitly set, the value reflects the window's \l{SafeArea}
    {safe area margins}.

    \sa bottomPadding, leftPadding, rightPadding
*/

/*!
    \qmlproperty real QtQuick.Controls::ApplicationWindow::leftPadding
    \since 6.9

    This property holds the left padding of the window's content item.
    Unless explicitly set, the value reflects the window's \l{SafeArea}
    {safe area margins}.

    \sa bottomPadding, topPadding, rightPadding
*/

/*!
    \qmlproperty real QtQuick.Controls::ApplicationWindow::rightPadding
    \since 6.9

    This property holds the right padding of the window's content item.
    Unless explicitly set, the value reflects the window's \l{SafeArea}
    {safe area margins}.

    \sa bottomPadding, leftPadding, topPadding
*/

/*!
    \qmlproperty real QtQuick.Controls::ApplicationWindow::bottomPadding
    \since 6.9

    This property holds the bottom padding of the window's content item.
    Unless explicitly set, the value reflects the window's \l{SafeArea}
    {safe area margins}.

    \sa topPadding, leftPadding, rightPadding
*/

/*!
    \qmlproperty Control QtQuick.Controls::ApplicationWindow::activeFocusControl
    \readonly

    This property holds the control that currently has active focus, or \c null if there is
    no control with active focus.

    The difference between \l Window::activeFocusItem and ApplicationWindow::activeFocusControl
    is that the former may point to a building block of a control, whereas the latter points
    to the enclosing control. For example, when SpinBox has focus, activeFocusItem points to
    the editor and activeFocusControl to the SpinBox itself.

    \sa Window::activeFocusItem
*/
QQuickItem *QQuickApplicationWindow::activeFocusControl() const
{
    Q_D(const QQuickApplicationWindow);
    return d->activeFocusControl;
}

/*!
    \qmlproperty font QtQuick.Controls::ApplicationWindow::font

    This property holds the font currently set for the window.

    The default font depends on the system environment. QGuiApplication maintains a system/theme
    font which serves as a default for all application windows. You can also set the default font
    for windows by passing a custom font to QGuiApplication::setFont(), before loading any QML.
    Finally, the font is matched against Qt's font database to find the best match.

    ApplicationWindow propagates explicit font properties to child controls. If you change a specific
    property on the window's font, that property propagates to all child controls in the window,
    overriding any system defaults for that property.

    \sa Control::font
*/
QFont QQuickApplicationWindow::font() const
{
    Q_D(const QQuickApplicationWindow);
    return d->font;
}

void QQuickApplicationWindow::setFont(const QFont &font)
{
    Q_D(QQuickApplicationWindow);
    if (d->font.resolveMask() == font.resolveMask() && d->font == font)
        return;

    QFont resolvedFont = font.resolve(QQuickTheme::font(QQuickTheme::System));
    d->setFont_helper(resolvedFont);
}

void QQuickApplicationWindow::resetFont()
{
    setFont(QFont());
}

/*!
    \qmlproperty Locale QtQuick.Controls::ApplicationWindow::locale

    This property holds the locale of the window.

    The default locale depends on the system environment. You can set the
    default locale by calling QLocale::setDefault(), before loading any QML.

    ApplicationWindow propagates the locale to child controls. If you change
    the window's locale, that locale propagates to all child controls in the
    window, overriding the system default locale.

    \sa Control::locale
*/
QLocale QQuickApplicationWindow::locale() const
{
    Q_D(const QQuickApplicationWindow);
    return d->locale;
}

void QQuickApplicationWindow::setLocale(const QLocale &locale)
{
    Q_D(QQuickApplicationWindow);
    if (d->locale == locale)
        return;

    d->locale = locale;
    QQuickControlPrivate::updateLocaleRecur(QQuickWindow::contentItem(), locale);

    // TODO: internal QQuickPopupManager that provides reliable access to all QQuickPopup instances
    const QList<QQuickPopup *> popups = QQuickWindow::contentItem()->findChildren<QQuickPopup *>();
    for (QQuickPopup *popup : popups)
        QQuickControlPrivate::get(static_cast<QQuickControl *>(popup->popupItem()))->updateLocale(locale, false); // explicit=false

    emit localeChanged();
}

void QQuickApplicationWindow::resetLocale()
{
    setLocale(QLocale());
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::menuBar

    This property holds the window menu bar. The menu bar is positioned at the
    top of the window, above the header, and resized to the width of the window.
    The default value is \c null.

    \code
    ApplicationWindow {
        menuBar: MenuBar {
            // ...
        }
    }
    \endcode

    \sa header, footer, MenuBar
*/
QQuickItem *QQuickApplicationWindow::menuBar() const
{
    Q_D(const QQuickApplicationWindow);
    return d->menuBar;
}

void QQuickApplicationWindow::setMenuBar(QQuickItem *menuBar)
{
    Q_D(QQuickApplicationWindow);
    if (d->menuBar == menuBar)
        return;

    if (d->menuBar) {
        QQuickItemPrivate::get(d->menuBar)->removeItemChangeListener(d, ItemChanges);
        d->menuBar->setParentItem(nullptr);
    }
    d->menuBar = menuBar;
    if (menuBar) {
        menuBar->setParentItem(QQuickWindow::contentItem());
        QQuickItemPrivate *p = QQuickItemPrivate::get(menuBar);
        p->addItemChangeListener(d, ItemChanges);
        if (qFuzzyIsNull(menuBar->z()))
            menuBar->setZ(2);

        if (header())
            menuBar->stackBefore(header());
        else
            menuBar->stackBefore(d->control);
    }
    if (isComponentComplete())
        d->relayout();
    emit menuBarChanged();
}

bool QQuickApplicationWindow::isComponentComplete() const
{
    Q_D(const QQuickApplicationWindow);
    return d->componentComplete;
}

void QQuickApplicationWindow::classBegin()
{
    Q_D(QQuickApplicationWindow);
    d->componentComplete = false;
    QQuickWindowQmlImpl::classBegin();
    d->resolveFont();

    /*
        Create the control up front, rather than lazily, as we have
        to set the default padding before any user-bindings override
        them.

        The hierarchy is:

            contentItem (QQuickRootItem)
            ├── menuBar
            ├── header
            ├── control (QQuickControl)
            │   └── control->contentItem() (QQuickContentItem)
            ├── footer
            └── background
     */
    d->control = new QQuickControl(QQuickWindow::contentItem());
    d->control->setObjectName("ApplicationWindowContentControl");
    auto *contentItem = new QQuickContentItem(this, d->control);
    // The content item can't be its own focus scope here, as that
    // will detach focus of items inside the content item from focus
    // in the menubar, header and footer. Nor can set set the content
    // item as focused, as that will prevent child items of the content
    // item from getting the initial focus when they are reparented
    // into the content item.
    d->control->setContentItem(contentItem);

    auto *context = qmlContext(this);
    auto installPropertyBinding = [&](QObject *targetObject, const QString &targetPropertyName,
                                      QObject *sourceObject, const QString &sourcePropertyName) {
        const QQmlProperty targetProperty(targetObject, targetPropertyName);
        QQmlAnyBinding binding = QQmlPropertyToPropertyBinding::create(
                context->engine(), QQmlProperty(sourceObject, sourcePropertyName),  targetProperty);
        binding.installOn(targetProperty);
    };

    // Pick up safe area margins from the control, which reflects both
    // margins coming from the QWindow, additional margins added by the
    // user to the QQuickWindow's content item, as well the additional
    // margins we add to the control directly to account for the header,
    // footer and menu bar.
    auto *controlSafeArea = qmlAttachedPropertiesObject<QQuickSafeArea>(d->control);
    installPropertyBinding(this, "leftPadding"_L1, controlSafeArea, "margins.left"_L1);
    installPropertyBinding(this, "topPadding"_L1, controlSafeArea, "margins.top"_L1);
    installPropertyBinding(this, "rightPadding"_L1, controlSafeArea, "margins.right"_L1);
    installPropertyBinding(this, "bottomPadding"_L1, controlSafeArea, "margins.bottom"_L1);

    // The additional margins for our header, footer and menu bar depend on the window margins
    auto *windowSafeArea = static_cast<QQuickSafeArea*>(qmlAttachedPropertiesObject<QQuickSafeArea>(this));
    QObject::connect(windowSafeArea, &QQuickSafeArea::marginsChanged, this, [d]{
        d->relayout();
    });
}

void QQuickApplicationWindow::componentComplete()
{
    Q_D(QQuickApplicationWindow);
    d->componentComplete = true;
    QQuickWindow::contentItem()->setObjectName(QQmlMetaType::prettyTypeName(this));
    d->executeBackground(true);
    QQuickWindowQmlImpl::componentComplete();
    d->relayout();
}

void QQuickApplicationWindow::resizeEvent(QResizeEvent *event)
{
    Q_D(QQuickApplicationWindow);
    QQuickWindowQmlImpl::resizeEvent(event);
    d->relayout();
}

class QQuickApplicationWindowAttachedPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickApplicationWindowAttached)

    void windowChange(QQuickWindow *wnd);
    void activeFocusChange();

    QQuickWindow *window = nullptr;
    QQuickItem *activeFocusControl = nullptr;
};

void QQuickApplicationWindowAttachedPrivate::windowChange(QQuickWindow *wnd)
{
    Q_Q(QQuickApplicationWindowAttached);
    if (window == wnd)
        return;

    QQuickApplicationWindow *oldWindow = qobject_cast<QQuickApplicationWindow *>(window);
    if (oldWindow && !QQuickApplicationWindowPrivate::get(oldWindow))
        oldWindow = nullptr; // being deleted (QTBUG-52731)

    if (oldWindow) {
        disconnect(oldWindow, &QQuickApplicationWindow::activeFocusControlChanged,
                   this, &QQuickApplicationWindowAttachedPrivate::activeFocusChange);
        QObject::disconnect(oldWindow, &QQuickApplicationWindow::menuBarChanged,
                            q, &QQuickApplicationWindowAttached::menuBarChanged);
        QObject::disconnect(oldWindow, &QQuickApplicationWindow::headerChanged,
                            q, &QQuickApplicationWindowAttached::headerChanged);
        QObject::disconnect(oldWindow, &QQuickApplicationWindow::footerChanged,
                            q, &QQuickApplicationWindowAttached::footerChanged);
    } else if (window) {
        disconnect(window, &QQuickWindow::activeFocusItemChanged,
                   this, &QQuickApplicationWindowAttachedPrivate::activeFocusChange);
    }

    QQuickApplicationWindow *newWindow = qobject_cast<QQuickApplicationWindow *>(wnd);
    if (newWindow) {
        connect(newWindow, &QQuickApplicationWindow::activeFocusControlChanged,
                this, &QQuickApplicationWindowAttachedPrivate::activeFocusChange);
        QObject::connect(newWindow, &QQuickApplicationWindow::menuBarChanged,
                         q, &QQuickApplicationWindowAttached::menuBarChanged);
        QObject::connect(newWindow, &QQuickApplicationWindow::headerChanged,
                         q, &QQuickApplicationWindowAttached::headerChanged);
        QObject::connect(newWindow, &QQuickApplicationWindow::footerChanged,
                         q, &QQuickApplicationWindowAttached::footerChanged);
    } else if (wnd) {
        connect(wnd, &QQuickWindow::activeFocusItemChanged,
                this, &QQuickApplicationWindowAttachedPrivate::activeFocusChange);
    }

    window = wnd;
    emit q->windowChanged();
    emit q->contentItemChanged();

    activeFocusChange();
    if ((oldWindow && oldWindow->menuBar()) || (newWindow && newWindow->menuBar()))
        emit q->menuBarChanged();
    if ((oldWindow && oldWindow->header()) || (newWindow && newWindow->header()))
        emit q->headerChanged();
    if ((oldWindow && oldWindow->footer()) || (newWindow && newWindow->footer()))
        emit q->footerChanged();
}

void QQuickApplicationWindowAttachedPrivate::activeFocusChange()
{
    Q_Q(QQuickApplicationWindowAttached);
    QQuickItem *control = nullptr;
    if (QQuickApplicationWindow *appWindow = qobject_cast<QQuickApplicationWindow *>(window))
        control = appWindow->activeFocusControl();
    else if (window)
        control = findActiveFocusControl(window);
    if (activeFocusControl == control)
        return;

    activeFocusControl = control;
    emit q->activeFocusControlChanged();
}

QQuickApplicationWindowAttached::QQuickApplicationWindowAttached(QObject *parent)
    : QObject(*(new QQuickApplicationWindowAttachedPrivate), parent)
{
    Q_D(QQuickApplicationWindowAttached);
    if (QQuickItem *item = qobject_cast<QQuickItem *>(parent)) {
        d->windowChange(item->window());
        QObjectPrivate::connect(item, &QQuickItem::windowChanged, d, &QQuickApplicationWindowAttachedPrivate::windowChange);
        if (!d->window) {
            QQuickItem *p = item;
            while (p) {
                if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(p->parent())) {
                    d->windowChange(popup->window());
                    QObjectPrivate::connect(popup, &QQuickPopup::windowChanged, d, &QQuickApplicationWindowAttachedPrivate::windowChange);
                }
                p = p->parentItem();
            }
        }
    } else if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(parent)) {
        d->windowChange(popup->window());
        QObjectPrivate::connect(popup, &QQuickPopup::windowChanged, d, &QQuickApplicationWindowAttachedPrivate::windowChange);
    }
}

/*!
    \qmlattachedproperty ApplicationWindow QtQuick.Controls::ApplicationWindow::window
    \readonly

    This attached property holds the application window. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickApplicationWindow *QQuickApplicationWindowAttached::window() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return qobject_cast<QQuickApplicationWindow *>(d->window);
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::contentItem
    \readonly

    This attached property holds the window content item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::contentItem() const
{
    Q_D(const QQuickApplicationWindowAttached);
    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(d->window))
        return window->contentItem();
    return nullptr;
}

/*!
    \qmlattachedproperty Control QtQuick.Controls::ApplicationWindow::activeFocusControl
    \readonly

    This attached property holds the control that currently has active focus, or \c null
    if there is no control with active focus. The property can be attached to any item.
    The value is \c null if the item is not in a window, or the window has no active focus.

    \sa Window::activeFocusItem, {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::activeFocusControl() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return d->activeFocusControl;
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::header
    \readonly

    This attached property holds the window header item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow, or
    the window has no header item.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::header() const
{
    Q_D(const QQuickApplicationWindowAttached);
    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(d->window))
        return window->header();
    return nullptr;
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::footer
    \readonly

    This attached property holds the window footer item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow, or
    the window has no footer item.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::footer() const
{
    Q_D(const QQuickApplicationWindowAttached);
    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(d->window))
        return window->footer();
    return nullptr;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::menuBar
    \readonly

    This attached property holds the window menu bar. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow, or
    the window has no menu bar.

    \sa {Attached ApplicationWindow Properties}
*/
QQuickItem *QQuickApplicationWindowAttached::menuBar() const
{
    Q_D(const QQuickApplicationWindowAttached);
    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(d->window))
        return window->menuBar();
    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qquickapplicationwindow_p.cpp"
