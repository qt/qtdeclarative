// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpopup_p.h"
#include "qquickpopup_p_p.h"
#include "qquickpopupanchors_p.h"
#include "qquickpopupitem_p_p.h"
#include "qquickpopupwindow_p_p.h"
#include "qquickpopuppositioner_p_p.h"
#include "qquickapplicationwindow_p.h"
#include "qquickoverlay_p_p.h"
#include "qquickcontrol_p_p.h"
#if QT_CONFIG(quicktemplates2_container)
#include "qquickdialog_p.h"
#endif

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcDimmer, "qt.quick.controls.popup.dimmer")
Q_STATIC_LOGGING_CATEGORY(lcQuickPopup, "qt.quick.controls.popup")

/*!
    \qmltype Popup
    \inherits QtObject
//!     \nativetype QQuickPopup
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-popups
    \ingroup qtquickcontrols-focusscopes
    \brief Base type of popup-like user interface controls.

    Popup is the base type of popup-like user interface controls. It can be
    used with \l Window or \l ApplicationWindow.

    \qml
    import QtQuick.Window
    import QtQuick.Controls

    ApplicationWindow {
        id: window
        width: 400
        height: 400
        visible: true

        Button {
            text: "Open"
            onClicked: popup.open()
        }

        Popup {
            id: popup
            x: 100
            y: 100
            width: 200
            height: 300
            modal: true
            focus: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        }
    }
    \endqml

    Popup does not provide a layout of its own, but requires you to position
    its contents, for instance by creating a \l RowLayout or a \l ColumnLayout.

    Items declared as children of a Popup are automatically parented to the
    Popups's \l contentItem. Items created dynamically need to be explicitly
    parented to the contentItem.

    \section1 Popup Layout

    The following diagram illustrates the layout of a popup within a window:

    \image qtquickcontrols-popup.png

    The \l implicitWidth and \l implicitHeight of a popup are typically based
    on the implicit sizes of the background and the content item plus any insets
    and paddings. These properties determine how large the popup will be when no
    explicit \l width or \l height is specified.

    The geometry of the \l contentItem is determined by the padding. The following
    example reserves 10px padding between the boundaries of the popup and its content:

    \code
    Popup {
        padding: 10

        contentItem: Text {
            text: "Content"
        }
    }
    \endcode

    The \l background item fills the entire width and height of the popup,
    unless insets or an explicit size have been given for it.

    Negative insets can be used to make the background larger than the popup.
    The following example uses negative insets to place a shadow outside the
    popup's boundaries:

    \code
    Popup {
        topInset: -2
        leftInset: -2
        rightInset: -6
        bottomInset: -6

        background: BorderImage {
            source: ":/images/shadowed-background.png"
        }
    }
    \endcode

    \section1 Popup type

    Since Qt 6.8, some popups, such as \l Menu, offer three different implementations,
    depending on the platform. You can choose which one you prefer by setting \l popupType.

    Whether a popup will be able to use the preferred type depends on the platform.
    \c Popup.Item is supported on all platforms, but \c Popup.Window and \c Popup.Native
    are normally only supported on desktop platforms. Additionally, if a popup is a
    \l Menu inside a \l {Native menu bars}{native menubar}, the menu will be native as
    well. And if the menu is a sub-menu inside another menu, the parent (or root) menu
    will decide the type.

    \section2 Showing a popup as an item

    By setting \l popupType to \c Popup.Item, the popup will \e not be shown as a separate
    window, but as an item inside the same scene as the parent. This item is parented
    to that scene's \l{Overlay::overlay}{overlay}, and styled to look like an actual window.

    This option is especially useful on platforms that doesn't support multiple windows.
    This was also the only option before Qt 6.8.

    In order to ensure that a popup is displayed above other items in the
    scene, it is recommended to use ApplicationWindow. ApplicationWindow also
    provides background dimming effects.

    \section2 Showing a popup as a separate window

    By setting \l popupType to \c Popup.Window, the popup will be shown inside a top-level
    \l {QQuickWindow}{window} configured with the \l Qt::Popup flag. Using a window to show a
    popup has the advantage that the popup will float on top of the parent window, and
    can be placed outside of its geometry. The popup will otherwise look the same as when
    using \c Popup.Item, that is, it will use the same QML delegates and styling as
    when using \c Popup.Item.

    \note If the platform doesn't support \c Popup.Window, \c Popup.Item will be used as fallback.

    \section2 Showing a native popup

    By setting \l popupType to \c Popup.Native, the popup will be shown using a platform
    native popup window. This window, and all its contents, will be rendered by the
    platform, and not by QML. This means that the QML delegates assigned to the popup
    will \e not be used for rendering. If you for example
    use this option on a \l Menu, it will be implemented using platform-specific
    menu APIs. This will normally make the popup look and feel more native than for example
    \c Popup.Window, but at the same time, suffer from platform limitations and differences
    related to appearance and behavior. Such limitations are documented in more detail
    in the subclasses that are affected, such as for a
    \l {Limitations when using native menus}{Menu}).

    \note If the platform doesn't support \c Popup.Native, \c Popup.Window will be used as fallback.

    \section1 Popup Sizing

    If only a single item is used within a Popup, it will resize to fit the
    implicit size of its contained item. This makes it particularly suitable
    for use together with layouts.

    \code
    Popup {
        ColumnLayout {
            anchors.fill: parent
            CheckBox { text: qsTr("E-mail") }
            CheckBox { text: qsTr("Calendar") }
            CheckBox { text: qsTr("Contacts") }
        }
    }
    \endcode

    Sometimes there might be two items within the popup:

    \code
    Popup {
        SwipeView {
            // ...
        }
        PageIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
        }
    }
    \endcode

    In this case, Popup cannot calculate a sensible implicit size. Since we're
    anchoring the \l PageIndicator over the \l SwipeView, we can simply set the
    content size to the view's implicit size:

    \code
    Popup {
        contentWidth: view.implicitWidth
        contentHeight: view.implicitHeight

        SwipeView {
            id: view
            // ...
        }
        PageIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
        }
     }
    \endcode

    \note When using \l {Showing a popup as an item}{popup items}, the popup's
    \l{contentItem}{content item} gets parented to the \l{Overlay::}{overlay},
    and does not live within the popup's parent. Because of that, a \l{Item::}
    {scale} applied to the tree in which the popup lives does not apply to the
    visual popup. To make the popup of e.g. a \l{ComboBox} follow the scale of
    the combobox, apply the same scale to the \l{Overlay::}{overlay} as well:

    \code
    Window {
        property double scaleFactor: 2.0

        Scale {
            id: scale
            xScale: scaleFactor
            yScale: scaleFactor
        }
        Item {
            id: scaledContent
            transform: scale

            ComboBox {
                id: combobox
                // ...
            }
        }

        Overlay.overlay.transform: scale
    }
    \endcode

    \section1 Popup Positioning

    Similar to items in Qt Quick, Popup's \l x and \l y coordinates are
    relative to its parent. This means that opening a popup that is a
    child of a \l Button, for example, will cause the popup to be positioned
    relative to the button.

    \include qquickoverlay-popup-parent.qdocinc

    Another way to center a popup in the window regardless of its parent item
    is to use \l {anchors.centerIn}:

    \snippet qtquickcontrols-popup.qml centerIn

    To ensure that the popup is positioned within the bounds of the enclosing
    window, the \l margins property can be set to a non-negative value.

    \section1 Showing Non-Child Items in Front of Popup

    In cases where \l {Showing a popup as an item}{popup windows} are not being used,
    Popup sets its contentItem's \l{qtquick-visualcanvas-visualparent.html}{visual parent}
    to be the window's \l{Overlay::overlay}{overlay}, in order to ensure that
    the popup appears in front of everything else in the scene.
    In some cases, it might be useful to put an item in front of a popup,
    such as a \l [QML QtVirtualKeyboard] {InputPanel} {virtual keyboard}.
    This can be done by setting the item's parent to the overlay,
    and giving the item a positive z value. The same result can also be
    achieved by waiting until the popup is opened, before re-parenting the item
    to the overlay.

    \omit
        This shouldn't be a snippet, since we don't want VKB to be a dependency to controls.
    \endomit
    \qml
    Popup {
        id: popup
        visible: true
        anchors.centerIn: parent
        margins: 10
        closePolicy: Popup.CloseOnEscape
        ColumnLayout {
            TextField {
                placeholderText: qsTr("Username")
            }
            TextField {
                placeholderText: qsTr("Password")
                echoMode: TextInput.Password
            }
        }
    }
    InputPanel {
        parent: Overlay.overlay
        width: parent.width
        y: popup.y + popup.topMargin + (window.activeFocusItem?.y ?? 0) + (window.activeFocusItem?.height ?? 0)
        z: 1
    }
    \endqml

    \section1 Popup Transitions

    Since Qt 5.15.3 the following properties are restored to their original values from before
    the enter transition after the exit transition is completed.

    \list
    \li \l opacity
    \li \l scale
    \endlist

    This allows the built-in styles to animate on these properties without losing any explicitly
    defined value.

    \section1 Back/Escape Event Handling

    By default, a Popup will close if:
    \list
    \li It has \l activeFocus,
    \li Its \l closePolicy is \c {Popup.CloseOnEscape}, and
    \li The user presses the key sequence for QKeySequence::Cancel (typically
        the Escape key)
    \endlist

    To prevent this from happening, either:

    \list
    \li Don't give the popup \l focus.
    \li Set the popup's \l closePolicy to a value that does not include
        \c {Popup.CloseOnEscape}.
    \li Handle \l {Keys}' \l {Keys::}{escapePressed} signal in a child item of
        the popup so that it gets the event before the Popup.
    \endlist

    \sa {Popup Controls}, {Customizing Popup}, ApplicationWindow

    \section1 Property Propagation

    Popup inherits fonts, palettes and attached properties through its parent
    window, not its \l {Visual Parent}{object or visual parent}:

    \snippet qtquickcontrols-popup-property-propagation.qml file

    \image qtquickcontrols-basic-popup-property-propagation.png

    In addition, popups do not propagate their properties to child popups. This
    behavior is modelled on Qt Widgets, where a \c Qt::Popup widget is a
    top-level window. Top-level windows do not propagate their properties to
    child windows.

    Certain derived types like ComboBox are typically implemented in such a way
    that the popup is considered an integral part of the control, and as such,
    may inherit things like attached properties. For example, in the
    \l {Material Style}{Material style} ComboBox, the theme and other attached
    properties are explicitly inherited by the Popup from the ComboBox itself:

    \code
    popup: T.Popup {
        // ...

        Material.theme: control.Material.theme
        Material.accent: control.Material.accent
        Material.primary: control.Material.primary
    }
    \endcode

    So, to ensure that a child popup has the same property values as its parent
    popup, explicitly set those properties:

    \code
    Popup {
        id: parentPopup
        // ...

        Popup {
            palette: parentPopup.palette
        }
    }
    \endcode

    \section1 Polish Behavior of Closed Popups

    When a popup is closed, it has no associated window, and neither do its
    child items. This means that any child items will not be
    \l {QQuickItem::polish}{polished} until the popup is shown. For this
    reason, you cannot, for example, rely on a \l ListView within a closed
    \c Popup to update its \c count property:

    \code
    import QtQuick
    import QtQuick.Controls

    ApplicationWindow {
        width: 640
        height: 480
        visible: true

        SomeModel {
            id: someModel
        }

        Button {
            text: view.count
            onClicked: popup.open()
        }

        Popup {
            id: popup
            width: 400
            height: 400
            contentItem: ListView {
                id: view
                model: someModel
                delegate: Label {
                    text: display

                    required property string display
                }
            }
        }
    }
    \endcode

    In the example above, the Button's text will not update when rows are added
    to or removed from \c someModel after \l {Component::completed}{component
    completion} while the popup is closed.

    Instead, a \c count property can be added to \c SomeModel that is updated
    whenever the \l {QAbstractItemModel::}{rowsInserted}, \l
    {QAbstractItemModel::}{rowsRemoved}, and \l
    {QAbstractItemModel::}{modelReset} signals are emitted. The \c Button can
    then bind this property to its \c text.
*/

/*!
    \qmlsignal void QtQuick.Controls::Popup::opened()

    This signal is emitted when the popup is opened.

    \sa aboutToShow()
*/

/*!
    \qmlsignal void QtQuick.Controls::Popup::closed()

    This signal is emitted when the popup is closed.

    \sa aboutToHide()
*/

/*!
    \qmlsignal void QtQuick.Controls::Popup::aboutToShow()

    This signal is emitted when the popup is about to show.

    \sa opened()
*/

/*!
    \qmlsignal void QtQuick.Controls::Popup::aboutToHide()

    This signal is emitted when the popup is about to hide.

    \sa closed()
*/

QQuickItem *QQuickPopup::findParentItem() const
{
    QObject *obj = parent();
    while (obj) {
        QQuickItem *item = qobject_cast<QQuickItem *>(obj);
        if (item)
            return item;
        obj = obj->parent();
    }
    return nullptr;
}

const QQuickPopup::ClosePolicy QQuickPopupPrivate::DefaultClosePolicy = QQuickPopup::CloseOnEscape | QQuickPopup::CloseOnPressOutside;

QQuickPopupPrivate::QQuickPopupPrivate()
    : transitionManager(this)
{
}

void QQuickPopupPrivate::init()
{
    Q_Q(QQuickPopup);
    popupItem = new QQuickPopupItem(q);
    popupItem->setVisible(false);
    QObject::connect(popupItem, &QQuickControl::paddingChanged, q, &QQuickPopup::paddingChanged);
    QObject::connect(popupItem, &QQuickControl::backgroundChanged, q, &QQuickPopup::backgroundChanged);
    QObject::connect(popupItem, &QQuickControl::contentItemChanged, q, &QQuickPopup::contentItemChanged);
    QObject::connect(popupItem, &QQuickControl::implicitContentWidthChanged, q, &QQuickPopup::implicitContentWidthChanged);
    QObject::connect(popupItem, &QQuickControl::implicitContentHeightChanged, q, &QQuickPopup::implicitContentHeightChanged);
    QObject::connect(popupItem, &QQuickControl::implicitBackgroundWidthChanged, q, &QQuickPopup::implicitBackgroundWidthChanged);
    QObject::connect(popupItem, &QQuickControl::implicitBackgroundHeightChanged, q, &QQuickPopup::implicitBackgroundHeightChanged);
}

void QQuickPopupPrivate::closeOrReject()
{
    Q_Q(QQuickPopup);
#if QT_CONFIG(quicktemplates2_container)
    if (QQuickDialog *dialog = qobject_cast<QQuickDialog*>(q))
        dialog->reject();
    else
#endif
        q->close();
    touchId = -1;
}

bool QQuickPopupPrivate::tryClose(const QPointF &pos, QQuickPopup::ClosePolicy flags)
{
    if (!interactive)
        return false;

    static const QQuickPopup::ClosePolicy outsideFlags = QQuickPopup::CloseOnPressOutside | QQuickPopup::CloseOnReleaseOutside;
    static const QQuickPopup::ClosePolicy outsideParentFlags = QQuickPopup::CloseOnPressOutsideParent | QQuickPopup::CloseOnReleaseOutsideParent;

    const bool onOutside = closePolicy & (flags & outsideFlags);
    const bool onOutsideParent = closePolicy & (flags & outsideParentFlags);

    if ((onOutside && outsidePressed) || (onOutsideParent && outsideParentPressed)) {
        if (!contains(pos) && (!dimmer || dimmer->contains(dimmer->mapFromScene(pos)))) {
            if (!onOutsideParent || !parentItem || !parentItem->contains(parentItem->mapFromScene(pos))) {
                closeOrReject();
                return true;
            }
        }
    }
    return false;
}

bool QQuickPopupPrivate::contains(const QPointF &scenePos) const
{
    return popupItem->contains(popupItem->mapFromScene(scenePos));
}

#if QT_CONFIG(quicktemplates2_multitouch)
bool QQuickPopupPrivate::acceptTouch(const QTouchEvent::TouchPoint &point)
{
    if (point.id() == touchId)
        return true;

    if (touchId == -1 && point.state() != QEventPoint::Released) {
        touchId = point.id();
        return true;
    }

    return false;
}
#endif

bool QQuickPopupPrivate::blockInput(QQuickItem *item, const QPointF &point) const
{
    // don't propagate events within the popup beyond the overlay
    if (popupItem->contains(popupItem->mapFromScene(point))
        && item == QQuickOverlay::overlay(window)) {
        return true;
    }

    // don't block presses and releases
    // a) outside a non-modal popup,
    // b) to popup children/content, or
    // c) outside a modal popups's background dimming

    return modal && ((popupItem != item) && !popupItem->isAncestorOf(item)) && (!dimmer || dimmer->contains(dimmer->mapFromScene(point)));
}

bool QQuickPopupPrivate::handlePress(QQuickItem *item, const QPointF &point, ulong timestamp)
{
    Q_UNUSED(timestamp);
    pressPoint = point;
    outsidePressed = !contains(point);

    if (outsidePressed && parentItem) {
        // Note that the parentItem (e.g a menuBarItem, in case of a MenuBar) will
        // live inside another window when using popup windows. We therefore need to
        // map to and from global.
        const QPointF globalPoint = item->mapToGlobal(point);
        const QPointF localPoint = parentItem->mapFromGlobal(globalPoint);
        outsideParentPressed = !parentItem->contains(localPoint);
    }

    tryClose(point, QQuickPopup::CloseOnPressOutside | QQuickPopup::CloseOnPressOutsideParent);
    return blockInput(item, point);
}

bool QQuickPopupPrivate::handleMove(QQuickItem *item, const QPointF &point, ulong timestamp)
{
    Q_UNUSED(timestamp);
    return blockInput(item, point);
}

bool QQuickPopupPrivate::handleRelease(QQuickItem *item, const QPointF &point, ulong timestamp)
{
    Q_UNUSED(timestamp);
    if (item != popupItem && !contains(pressPoint))
        tryClose(point, QQuickPopup::CloseOnReleaseOutside | QQuickPopup::CloseOnReleaseOutsideParent);
    pressPoint = QPointF();
    outsidePressed = false;
    outsideParentPressed = false;
    touchId = -1;
    return blockInput(item, point);
}

void QQuickPopupPrivate::handleUngrab()
{
    Q_Q(QQuickPopup);
    QQuickOverlay *overlay = QQuickOverlay::overlay(window);
    if (overlay) {
        QQuickOverlayPrivate *p = QQuickOverlayPrivate::get(overlay);
        if (p->mouseGrabberPopup == q)
            p->mouseGrabberPopup = nullptr;
    }
    pressPoint = QPointF();
    touchId = -1;
}

bool QQuickPopupPrivate::handleMouseEvent(QQuickItem *item, QMouseEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        return handlePress(item, event->scenePosition(), event->timestamp());
    case QEvent::MouseMove:
        return handleMove(item, event->scenePosition(), event->timestamp());
    case QEvent::MouseButtonRelease:
        return handleRelease(item, event->scenePosition(), event->timestamp());
    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

bool QQuickPopupPrivate::handleHoverEvent(QQuickItem *item, QHoverEvent *event)
{
    switch (event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
    case QEvent::HoverLeave:
        return blockInput(item, event->scenePosition());
    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

QMarginsF QQuickPopupPrivate::windowInsets() const
{
    Q_Q(const QQuickPopup);
    // If the popup has negative insets, it means that its background is pushed
    // outside the bounds of the popup. This is fine when the popup is an item in the
    // scene (Popup.Item), but will result in the background being clipped when using
    // a window (Popup.Window). To avoid this, the window will been made bigger than
    // the popup (according to the insets), to also include the part that ends up
    // outside (which is usually a drop-shadow).
    // Note that this also means that we need to take those extra margins into account
    // whenever we resize or position the menu, so that the top-left of the popup ends
    // up at the requested position, and not the top-left of the window.

    const auto *popupItemPrivate = QQuickControlPrivate::get(popupItem);
    if (!usePopupWindow() || (popupItemPrivate->background.isExecuting() && popupItemPrivate->background->clip())) {
        // Items in the scene are allowed to draw out-of-bounds, so we don't
        // need to do anything if we're not using popup windows. The same is
        // also true for popup windows if the background is clipped.
        return {0, 0, 0, 0};
    }

    return {
        q->leftInset() < 0 ? -q->leftInset() : 0,
        q->rightInset() < 0 ? -q->rightInset() : 0,
        q->topInset() < 0 ? -q->topInset() : 0,
        q->bottomInset() < 0 ? -q->bottomInset() : 0
    };
}

QPointF QQuickPopupPrivate::windowInsetsTopLeft() const
{
    const QMarginsF windowMargins = windowInsets();
    return {windowMargins.left(), windowMargins.top()};
}

void QQuickPopupPrivate::setEffectivePosFromWindowPos(const QPointF &windowPos)
{
    // Popup operates internally with three different positions; requested
    // position, effective position, and window position. The first is the
    // position requested by the application, and the second is where the popup
    // is actually placed. The reason for placing it on a different position than
    // the one requested, is to keep it inside the window (in case of Popup.Item),
    // or the screen (in case of Popup.Window).
    // Additionally, since a popup can set Qt::FramelessWindowHint and draw the
    // window frame from the background delegate, the effective position in that
    // case is adjusted to be the top-left corner of the background delegate, rather
    // than the top-left corner of the window. This allowes the background delegate
    // to render a drop-shadow between the edge of the window and the background frame.
    // Finally, the window position is the actual position of the window, including
    // any drop-shadow effects. This posision can be calculated by taking
    // the effective position and subtract the dropShadowOffset().
    Q_Q(QQuickPopup);
    const QPointF oldEffectivePos = effectivePos;
    effectivePos = windowPos + windowInsetsTopLeft();
    if (!qFuzzyCompare(oldEffectivePos.x(), effectivePos.x()))
        emit q->xChanged();
    if (!qFuzzyCompare(oldEffectivePos.y(), effectivePos.y()))
        emit q->yChanged();
}

#if QT_CONFIG(quicktemplates2_multitouch)
bool QQuickPopupPrivate::handleTouchEvent(QQuickItem *item, QTouchEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        for (const QTouchEvent::TouchPoint &point : event->points()) {
            if (event->type() != QEvent::TouchEnd && !acceptTouch(point))
                return blockInput(item, point.position());

            switch (point.state()) {
            case QEventPoint::Pressed:
                return handlePress(item, item->mapToScene(point.position()), event->timestamp());
            case QEventPoint::Updated:
                return handleMove(item, item->mapToScene(point.position()), event->timestamp());
            case QEventPoint::Released:
                return handleRelease(item, item->mapToScene(point.position()), event->timestamp());
            default:
                break;
            }
        }
        break;

    case QEvent::TouchCancel:
        handleUngrab();
        break;

    default:
        break;
    }

    return false;
}
#endif

bool QQuickPopupPrivate::prepareEnterTransition()
{
    Q_Q(QQuickPopup);
    if (!window) {
        qmlWarning(q) << "cannot find any window to open popup in.";
        return false;
    }

    if (transitionState == EnterTransition && transitionManager.isRunning())
        return false;

    if (transitionState != EnterTransition) {
        const QPointer<QQuickItem> lastActiveFocusItem = window->activeFocusItem();
        visible = true;
        adjustPopupItemParentAndWindow();
        if (dim)
            createOverlay();
        showDimmer();
        emit q->aboutToShow();
        transitionState = EnterTransition;
        getPositioner()->setParentItem(parentItem);
        emit q->visibleChanged();

        if (lastActiveFocusItem) {
            if (auto *overlay = QQuickOverlay::overlay(window)) {
                auto *overlayPrivate = QQuickOverlayPrivate::get(overlay);
                if (overlayPrivate->lastActiveFocusItem.isNull() && !popupItem->isAncestorOf(lastActiveFocusItem)) {
                    overlayPrivate->lastActiveFocusItem = lastActiveFocusItem;
                    savedLastActiveFocusItem = true;
                }
            }
        }

        if (focus)
            popupItem->setFocus(true, Qt::PopupFocusReason);
    }
    return true;
}

bool QQuickPopupPrivate::prepareExitTransition()
{
    Q_Q(QQuickPopup);
    if (transitionState == ExitTransition && transitionManager.isRunning())
        return false;

    Q_ASSERT(popupItem);

    // We need to cache the original scale and opacity values so we can reset it after
    // the exit transition is done so they have the original values again
    prevScale = popupItem->scale();
    prevOpacity = popupItem->opacity();

    if (transitionState != ExitTransition) {
        // The setFocus(false) call below removes any active focus before we're
        // able to check it in finalizeExitTransition.
        if (!hadActiveFocusBeforeExitTransition) {
            const auto *da = QQuickItemPrivate::get(popupItem)->deliveryAgentPrivate();
            hadActiveFocusBeforeExitTransition = popupItem->hasActiveFocus() || (da && da->focusTargetItem() == popupItem);
        }

        if (focus)
            popupItem->setFocus(false, Qt::PopupFocusReason);
        transitionState = ExitTransition;
        hideDimmer();
        emit q->aboutToHide();
        emit q->openedChanged();
    }
    return true;
}

void QQuickPopupPrivate::finalizeEnterTransition()
{
    Q_Q(QQuickPopup);
    transitionState = NoTransition;
    reposition();
    emit q->openedChanged();
    opened();
}

void QQuickPopupPrivate::finalizeExitTransition()
{
    Q_Q(QQuickPopup);
    getPositioner()->setParentItem(nullptr);
    if (popupItem) {
        popupItem->setParentItem(nullptr);
        popupItem->setVisible(false);
    }
    destroyDimmer();

    if (hadActiveFocusBeforeExitTransition && window) {
        // restore focus to the next popup in chain, or to the window content if there are no other popups open
        QQuickPopup *nextFocusPopup = nullptr;
        if (QQuickOverlay *overlay = QQuickOverlay::overlay(window)) {
            const auto stackingOrderPopups = QQuickOverlayPrivate::get(overlay)->stackingOrderPopups();
            for (auto popup : stackingOrderPopups) {
                // only pick a popup that is focused but has not already been activated
                if (QQuickPopupPrivate::get(popup)->transitionState != ExitTransition
                        && popup->hasFocus() && !popup->hasActiveFocus()) {
                    nextFocusPopup = popup;
                    break;
                }
            }
        }
        if (nextFocusPopup) {
            nextFocusPopup->forceActiveFocus(Qt::PopupFocusReason);
        } else {
            auto *appWindow = qobject_cast<QQuickApplicationWindow*>(window);
            auto *contentItem = appWindow ? appWindow->contentItem() : window->contentItem();
            auto *overlay = QQuickOverlay::overlay(window);
            auto *overlayPrivate = QQuickOverlayPrivate::get(overlay);
            if (!contentItem->scopedFocusItem()
                && !overlayPrivate->lastActiveFocusItem.isNull()) {
                // The last active focus item may have lost focus not just for
                // itself but for its entire focus chain, so force active focus.
                overlayPrivate->lastActiveFocusItem->forceActiveFocus(Qt::OtherFocusReason);
            } else {
                contentItem->setFocus(true, Qt::PopupFocusReason);
            }
        }
    }

    if (window) {
        auto *overlay = QQuickOverlay::overlay(window);
        auto *overlayPrivate = overlay ? QQuickOverlayPrivate::get(overlay) : nullptr;

        // Clear the overlay's saved focus if this popup was the one that set it
        if (savedLastActiveFocusItem && overlayPrivate)
            overlayPrivate->lastActiveFocusItem = nullptr;
    }

    visible = false;
    adjustPopupItemParentAndWindow();
    transitionState = NoTransition;
    hadActiveFocusBeforeExitTransition = false;
    savedLastActiveFocusItem = false;
    emit q->visibleChanged();
    emit q->closed();
#if QT_CONFIG(accessibility)
    const auto type = q->effectiveAccessibleRole() == QAccessible::PopupMenu
                        ? QAccessible::PopupMenuEnd
                        : QAccessible::DialogEnd;
    QAccessibleEvent ev(q->popupItem(), type);
    QAccessible::updateAccessibility(&ev);
#endif
    if (popupItem) {
        popupItem->setScale(prevScale);
        popupItem->setOpacity(prevOpacity);
    }
}

void QQuickPopupPrivate::opened()
{
    Q_Q(QQuickPopup);
    emit q->opened();
#if QT_CONFIG(accessibility)
    const auto type = q->effectiveAccessibleRole() == QAccessible::PopupMenu
                        ? QAccessible::PopupMenuStart
                        : QAccessible::DialogStart;
    QAccessibleEvent ev(q->popupItem(), type);
    QAccessible::updateAccessibility(&ev);
#endif
}

Qt::WindowFlags QQuickPopupPrivate::popupWindowType() const
{
    return Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint;
}

QMarginsF QQuickPopupPrivate::getMargins() const
{
    Q_Q(const QQuickPopup);
    return QMarginsF(q->leftMargin(), q->topMargin(), q->rightMargin(), q->bottomMargin());
}

void QQuickPopupPrivate::setTopMargin(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldMargin = q->topMargin();
    topMargin = value;
    hasTopMargin = !reset;
    if ((!reset && !qFuzzyCompare(oldMargin, value)) || (reset && !qFuzzyCompare(oldMargin, margins))) {
        emit q->topMarginChanged();
        q->marginsChange(QMarginsF(leftMargin, topMargin, rightMargin, bottomMargin),
                         QMarginsF(leftMargin, oldMargin, rightMargin, bottomMargin));
    }
}

void QQuickPopupPrivate::setLeftMargin(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldMargin = q->leftMargin();
    leftMargin = value;
    hasLeftMargin = !reset;
    if ((!reset && !qFuzzyCompare(oldMargin, value)) || (reset && !qFuzzyCompare(oldMargin, margins))) {
        emit q->leftMarginChanged();
        q->marginsChange(QMarginsF(leftMargin, topMargin, rightMargin, bottomMargin),
                         QMarginsF(oldMargin, topMargin, rightMargin, bottomMargin));
    }
}

void QQuickPopupPrivate::setRightMargin(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldMargin = q->rightMargin();
    rightMargin = value;
    hasRightMargin = !reset;
    if ((!reset && !qFuzzyCompare(oldMargin, value)) || (reset && !qFuzzyCompare(oldMargin, margins))) {
        emit q->rightMarginChanged();
        q->marginsChange(QMarginsF(leftMargin, topMargin, rightMargin, bottomMargin),
                         QMarginsF(leftMargin, topMargin, oldMargin, bottomMargin));
    }
}

void QQuickPopupPrivate::setBottomMargin(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldMargin = q->bottomMargin();
    bottomMargin = value;
    hasBottomMargin = !reset;
    if ((!reset && !qFuzzyCompare(oldMargin, value)) || (reset && !qFuzzyCompare(oldMargin, margins))) {
        emit q->bottomMarginChanged();
        q->marginsChange(QMarginsF(leftMargin, topMargin, rightMargin, bottomMargin),
                         QMarginsF(leftMargin, topMargin, rightMargin, oldMargin));
    }
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty Item QtQuick.Controls::Popup::anchors.centerIn

    Anchors provide a way to position an item by specifying its
    relationship with other items.

    A common use case is to center a popup within its parent. One way to do
    this is with the \l[QtQuick]{Item::}{x} and \l[QtQuick]{Item::}{y} properties. Anchors offer
    a more convenient approach:

    \qml
    Pane {
        // ...

        Popup {
            anchors.centerIn: parent
        }
    }
    \endqml

    It is also possible to center the popup in the window by using \l Overlay:

    \snippet qtquickcontrols-popup.qml centerIn

    This makes it easy to center a popup in the window from any component.

    \note Popups can only be centered within their immediate parent or
    the window overlay; trying to center in other items will produce a warning.

    \sa {Popup Positioning}, {Item::}{anchors}, {Using Qt Quick Controls types
        in property declarations}
*/
QQuickPopupAnchors *QQuickPopupPrivate::getAnchors()
{
    Q_Q(QQuickPopup);
    if (!anchors)
        anchors = new QQuickPopupAnchors(q);
    return anchors;
}

QQuickPopupPositioner *QQuickPopupPrivate::getPositioner()
{
    Q_Q(QQuickPopup);
    if (!positioner)
        positioner = new QQuickPopupPositioner(q);
    return positioner;
}

void QQuickPopupPrivate::setWindow(QQuickWindow *newWindow)
{
    Q_Q(QQuickPopup);
    if (window == newWindow)
        return;

    if (window) {
        QQuickOverlay *overlay = QQuickOverlay::overlay(window);
        if (overlay)
            QQuickOverlayPrivate::get(overlay)->removePopup(q);
    }

    window = newWindow;

    if (newWindow) {
        QQuickOverlay *overlay = QQuickOverlay::overlay(newWindow);
        if (overlay)
            QQuickOverlayPrivate::get(overlay)->addPopup(q);

        QQuickControlPrivate *p = QQuickControlPrivate::get(popupItem);
        p->resolveFont();
        if (QQuickApplicationWindow *appWindow = qobject_cast<QQuickApplicationWindow *>(newWindow))
            p->updateLocale(appWindow->locale(), false); // explicit=false
    }

    emit q->windowChanged(newWindow);

    if (complete && visible && window)
        transitionManager.transitionEnter();
}

void QQuickPopupPrivate::itemDestroyed(QQuickItem *item)
{
    Q_Q(QQuickPopup);
    if (item == parentItem)
        q->setParentItem(nullptr);
}

void QQuickPopupPrivate::reposition()
{
    getPositioner()->reposition();
}

QPalette QQuickPopupPrivate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::System);
}

QQuickPopup::PopupType QQuickPopupPrivate::resolvedPopupType() const
{
    // Whether or not the resolved popup type ends up the same as the preferred popup type
    // depends on platform capabilities, the popup subclass, and sometimes also the location
    // of the popup in the parent hierarchy (menus). This function can therefore be overridden
    // to return the actual popup type that should be used, based on the knowledge the popup
    // has just before it's about to be shown.

    // PopupType::Native is not directly supported by QQuickPopup (only by subclasses).
    // So for that case, we fall back to use PopupType::Window, if supported.
    if (popupType == QQuickPopup::PopupType::Window
        || popupType == QQuickPopup::PopupType::Native) {
        if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::Capability::MultipleWindows))
            return QQuickPopup::PopupType::Window;
    }

    return QQuickPopup::PopupType::Item;
}

bool QQuickPopupPrivate::usePopupWindow() const
{
    return resolvedPopupType() == QQuickPopup::PopupType::Window;
}

void QQuickPopupPrivate::adjustPopupItemParentAndWindow()
{
    Q_Q(QQuickPopup);
    QQuickOverlay *overlay = QQuickOverlay::overlay(window);

    if (visible && popupWindowDirty) {
        popupItem->setParentItem(overlay);
        if (popupWindow) {
            popupWindow->deleteLater();
            popupWindow = nullptr;
        }
        popupWindowDirty = false;
    }

    if (usePopupWindow()) {
        if (visible) {
            if (!popupWindow) {
                popupWindow = new QQuickPopupWindow(q, window);
                // Changing the visual parent can cause the popup item's implicit width/height to change.
                // We store the initial size here first, to ensure that we're resizing the window to the correct size.
                const qreal initialWidth = popupItem->width() + windowInsets().left() + windowInsets().right();
                const qreal initialHeight = popupItem->height() + windowInsets().top() + windowInsets().bottom();
                popupItem->setParentItem(popupWindow->contentItem());
                popupWindow->resize(initialWidth, initialHeight);
                popupWindow->setModality(modal ? Qt::ApplicationModal : Qt::NonModal);
                popupItem->resetTitle();
                popupWindow->setTitle(title);
            }
            popupItem->setParentItem(popupWindow->contentItem());
            popupItem->forceActiveFocus(Qt::PopupFocusReason);
        }
        if (popupWindow)
            popupWindow->setVisible(visible);
    } else {
        if (visible) {
            popupItem->setParentItem(overlay);
            const auto popupStack = QQuickOverlayPrivate::get(overlay)->stackingOrderPopups();
            // if there is a stack of popups, and the current top popup item belongs to an
            // ancestor of this popup, then make sure that this popup's item is at the top
            // of the stack.
            const QQuickPopup *topPopup = popupStack.isEmpty() ? nullptr : popupStack.first();
            const QObject *ancestor = q;
            while (ancestor && topPopup) {
                if (ancestor == topPopup)
                    break;
                ancestor = ancestor->parent();
            }
            if (topPopup && topPopup != q && ancestor) {
                QQuickItem *topPopupItem = popupStack.first()->popupItem();
                popupItem->stackAfter(topPopupItem);
                // If the popup doesn't have an explicit z value set, set it to be at least as
                // high as the current top popup item so that later opened popups are on top.
                if (!hasZ)
                    popupItem->setZ(qMax(topPopupItem->z(), popupItem->z()));
            }
        }

        popupItem->setTitle(title);
    }
    popupItem->setVisible(visible);
}

QQuickItem *QQuickPopupPrivate::createDimmer(QQmlComponent *component, QQuickPopup *popup, QQuickItem *parent) const
{
    QQuickItem *item = nullptr;
    if (component) {
        QQmlContext *context = component->creationContext();
        if (!context)
            context = qmlContext(popup);
        item = qobject_cast<QQuickItem*>(component->beginCreate(context));
    }

    // when there is no overlay component available (with plain QQuickWindow),
    // use a plain QQuickItem as a fallback to block hover events
    if (!item && popup->isModal())
        item = new QQuickItem;

    if (item) {
        item->setParentItem(parent);
        if (resolvedPopupType() != QQuickPopup::PopupType::Window)
            item->stackBefore(popup->popupItem());
        item->setZ(popup->z());
        // needed for the virtual keyboard to set a containment mask on the dimmer item
        qCDebug(lcDimmer) << "dimmer" << item << "registered with" << parent;
        parent->setProperty("_q_dimmerItem", QVariant::fromValue<QQuickItem*>(item));
        if (popup->isModal()) {
            item->setAcceptedMouseButtons(Qt::AllButtons);
#if QT_CONFIG(cursor)
            item->setCursor(Qt::ArrowCursor);
#endif
#if QT_CONFIG(quicktemplates2_hover)
            // TODO: switch to QStyleHints::useHoverEffects in Qt 5.8
            item->setAcceptHoverEvents(true);
            // item->setAcceptHoverEvents(QGuiApplication::styleHints()->useHoverEffects());
            // connect(QGuiApplication::styleHints(), &QStyleHints::useHoverEffectsChanged, item, &QQuickItem::setAcceptHoverEvents);
#endif
        }
        if (component)
            component->completeCreate();
    }
    qCDebug(lcDimmer) << "finished creating dimmer from component" << component
        << "for popup" << popup << "with parent" << parent << "- item is:" << item;
    return item;
}

void QQuickPopupPrivate::createOverlay()
{
    Q_Q(QQuickPopup);
    QQuickOverlay *overlay = QQuickOverlay::overlay(window);
    if (!overlay)
        return;

    QQmlComponent *component = nullptr;
    QQuickOverlayAttached *overlayAttached = qobject_cast<QQuickOverlayAttached *>(qmlAttachedPropertiesObject<QQuickOverlay>(q, false));
    if (overlayAttached)
        component = modal ? overlayAttached->modal() : overlayAttached->modeless();

    if (!component)
        component = modal ? overlay->modal() : overlay->modeless();

    if (!dimmer) {
        dimmer = createDimmer(component, q, overlay);
        if (!dimmer)
            return;
        // We cannot update explicitDimmerOpacity when dimmer's opacity changes,
        // as it is expected to do so when we fade the dimmer in and out in
        // show/hideDimmer, and any binding of the dimmer's opacity will be
        // implicitly broken anyway.
        explicitDimmerOpacity = dimmer->opacity();
        // initially fully transparent, showDimmer fades the dimmer in.
        dimmer->setOpacity(0);
        if (q->isVisible())
            showDimmer();
    }
    resizeDimmer();
}

void QQuickPopupPrivate::destroyDimmer()
{
    if (dimmer) {
        qCDebug(lcDimmer) << "destroying dimmer" << dimmer;
        if (QObject *dimmerParentItem = dimmer->parentItem()) {
            if (dimmerParentItem->property("_q_dimmerItem").value<QQuickItem*>() == dimmer)
                dimmerParentItem->setProperty("_q_dimmerItem", QVariant());
        }
        dimmer->setParentItem(nullptr);
        dimmer->deleteLater();
        dimmer = nullptr;
    }
}

void QQuickPopupPrivate::toggleOverlay()
{
    destroyDimmer();
    if (dim)
        createOverlay();
}

void QQuickPopupPrivate::updateContentPalettes(const QPalette& parentPalette)
{
    // Inherit parent palette to all child objects
    if (providesPalette())
        inheritPalette(parentPalette);
    // Inherit parent palette to items within popup (such as headers and footers)
    QQuickItemPrivate::get(popupItem)->updateChildrenPalettes(parentPalette);
}

void QQuickPopupPrivate::updateChildrenPalettes(const QPalette& parentPalette)
{
    updateContentPalettes(parentPalette);
}

void QQuickPopupPrivate::showDimmer()
{
    // use QQmlProperty instead of QQuickItem::setOpacity() to trigger QML Behaviors
    if (dim && dimmer)
        QQmlProperty::write(dimmer, QStringLiteral("opacity"), explicitDimmerOpacity);
}

void QQuickPopupPrivate::hideDimmer()
{
    // use QQmlProperty instead of QQuickItem::setOpacity() to trigger QML Behaviors
    if (dim && dimmer)
        QQmlProperty::write(dimmer, QStringLiteral("opacity"), 0.0);
}

void QQuickPopupPrivate::resizeDimmer()
{
    if (!dimmer)
        return;

    const QQuickOverlay *overlay = QQuickOverlay::overlay(window);

    qreal w = overlay ? overlay->width() : 0;
    qreal h = overlay ? overlay->height() : 0;
    dimmer->setSize(QSizeF(w, h));
}

QQuickPopupTransitionManager::QQuickPopupTransitionManager(QQuickPopupPrivate *popup)
    : popup(popup)
{
}

void QQuickPopupTransitionManager::transitionEnter()
{
    if (popup->transitionState == QQuickPopupPrivate::ExitTransition)
        cancel();

    if (!popup->prepareEnterTransition())
        return;

    if (popup->window)
        transition(popup->enterActions, popup->enter, popup->q_func());
    else
        finished();
}

void QQuickPopupTransitionManager::transitionExit()
{
    if (!popup->prepareExitTransition())
        return;

    if (popup->window)
        transition(popup->exitActions, popup->exit, popup->q_func());
    else
        finished();
}

void QQuickPopupTransitionManager::finished()
{
    if (popup->transitionState == QQuickPopupPrivate::EnterTransition)
        popup->finalizeEnterTransition();
    else if (popup->transitionState == QQuickPopupPrivate::ExitTransition)
        popup->finalizeExitTransition();
}

QQuickPopup::QQuickPopup(QObject *parent)
    : QObject(*(new QQuickPopupPrivate), parent)
{
    Q_D(QQuickPopup);
    d->init();
    // By default, allow popup to move beyond window edges
    d->relaxEdgeConstraint = true;
}

QQuickPopup::QQuickPopup(QQuickPopupPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QQuickPopup);
    d->init();
}

QQuickPopup::~QQuickPopup()
{
    Q_D(QQuickPopup);
    d->inDestructor = true;

    QQuickItem *currentContentItem = d->popupItem->d_func()->contentItem.data();
    if (currentContentItem) {
        disconnect(currentContentItem, &QQuickItem::childrenChanged,
                   this, &QQuickPopup::contentChildrenChanged);
    }

    setParentItem(nullptr);

    // If the popup is destroyed before the exit transition finishes,
    // the necessary cleanup (removing modal dimmers that block mouse events,
    // emitting closed signal, etc.) won't happen. That's why we do it manually here.
    if (d->transitionState == QQuickPopupPrivate::ExitTransition && d->transitionManager.isRunning())
        d->finalizeExitTransition();

    delete d->popupItem;
    d->popupItem = nullptr;
    delete d->positioner;
    d->positioner = nullptr;
    if (d->popupWindow)
        delete d->popupWindow;
    d->popupWindow = nullptr;
}

/*!
    \qmlmethod void QtQuick.Controls::Popup::open()

    Opens the popup.

    \sa visible
*/
void QQuickPopup::open()
{
    setVisible(true);
}

/*!
    \qmlmethod void QtQuick.Controls::Popup::close()

    Closes the popup.

    \sa visible
*/
void QQuickPopup::close()
{
    setVisible(false);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::x

    This property holds the x-coordinate of the popup.

    \sa y, z
*/
qreal QQuickPopup::x() const
{
    return d_func()->effectivePos.x();
}

void QQuickPopup::setX(qreal x)
{
    Q_D(QQuickPopup);
    setPosition(QPointF(x, d->y));
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::y

    This property holds the y-coordinate of the popup.

    \sa x, z
*/
qreal QQuickPopup::y() const
{
    return d_func()->effectivePos.y();
}

void QQuickPopup::setY(qreal y)
{
    Q_D(QQuickPopup);
    setPosition(QPointF(d->x, y));
}

QPointF QQuickPopup::position() const
{
    return d_func()->effectivePos;
}

void QQuickPopup::setPosition(const QPointF &pos)
{
    Q_D(QQuickPopup);
    const bool xChange = !qFuzzyCompare(d->x, pos.x());
    const bool yChange = !qFuzzyCompare(d->y, pos.y());
    if (!xChange && !yChange)
        return;

    d->x = pos.x();
    d->y = pos.y();
    if (d->popupItem->isVisible()) {
        d->reposition();
    } else {
        if (xChange)
            emit xChanged();
        if (yChange)
            emit yChanged();
    }
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::z

    This property holds the z-value of the popup. Z-value determines
    the stacking order of popups.

    If two visible popups have the same z-value, the last one that
    was opened will be on top.

    If a popup has no explicitly set z-value when opened, and is a child
    of an already open popup, then it will be stacked on top of its parent.
    This ensures that children are never hidden under their parents.

    If the popup has its own window, the z-value will determine the window
    stacking order instead.

    The default z-value is \c 0.

    \sa x, y
*/
qreal QQuickPopup::z() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->z();
}

void QQuickPopup::setZ(qreal z)
{
    Q_D(QQuickPopup);
    d->hasZ = true;
    bool previousZ = d->popupWindow ? d->popupWindow->z() : d->popupItem->z();
    if (qFuzzyCompare(z, previousZ))
        return;
    if (d->popupWindow)
        d->popupWindow->setZ(z);
    else
        d->popupItem->setZ(z);
    emit zChanged();
}

void QQuickPopup::resetZ()
{
    Q_D(QQuickPopup);
    setZ(0);
    d->hasZ = false;
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::width

    This property holds the width of the popup.
*/
qreal QQuickPopup::width() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->width();
}

void QQuickPopup::setWidth(qreal width)
{
    Q_D(QQuickPopup);
    d->hasWidth = true;

    // QQuickPopupWindow::setWidth() triggers a window resize event.
    // This will cause QQuickPopupWindow::resizeEvent() to resize
    // the popupItem. QQuickPopupItem::geometryChanged() calls QQuickPopup::geometryChange(),
    // which emits widthChanged().

    if (d->popupWindow)
        d->popupWindow->setWidth(width + d->windowInsets().left() + d->windowInsets().right());
    else
        d->popupItem->setWidth(width);
}

void QQuickPopup::resetWidth()
{
    Q_D(QQuickPopup);
    if (!d->hasWidth)
        return;

    d->hasWidth = false;
    d->popupItem->resetWidth();
    if (d->popupItem->isVisible())
        d->reposition();
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::height

    This property holds the height of the popup.
*/
qreal QQuickPopup::height() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->height();
}

void QQuickPopup::setHeight(qreal height)
{
    Q_D(QQuickPopup);
    d->hasHeight = true;

    // QQuickPopupWindow::setHeight() triggers a window resize event.
    // This will cause QQuickPopupWindow::resizeEvent() to resize
    // the popupItem. QQuickPopupItem::geometryChanged() calls QQuickPopup::geometryChange(),
    // which emits heightChanged().

    if (d->popupWindow)
        d->popupWindow->setHeight(height + d->windowInsets().top() + d->windowInsets().bottom());
    else
        d->popupItem->setHeight(height);
}

void QQuickPopup::resetHeight()
{
    Q_D(QQuickPopup);
    if (!d->hasHeight)
        return;

    d->hasHeight = false;
    d->popupItem->resetHeight();
    if (d->popupItem->isVisible())
        d->reposition();
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::implicitWidth

    This property holds the implicit width of the popup.
*/
qreal QQuickPopup::implicitWidth() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->implicitWidth();
}

void QQuickPopup::setImplicitWidth(qreal width)
{
    Q_D(QQuickPopup);
    d->popupItem->setImplicitWidth(width);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::implicitHeight

    This property holds the implicit height of the popup.
*/
qreal QQuickPopup::implicitHeight() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->implicitHeight();
}

void QQuickPopup::setImplicitHeight(qreal height)
{
    Q_D(QQuickPopup);
    d->popupItem->setImplicitHeight(height);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::contentWidth

    This property holds the content width. It is used for calculating the
    total implicit width of the Popup.

    For more information, see \l {Popup Sizing}.

    \sa contentHeight
*/
qreal QQuickPopup::contentWidth() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->contentWidth();
}

void QQuickPopup::setContentWidth(qreal width)
{
    Q_D(QQuickPopup);
    d->popupItem->setContentWidth(width);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::contentHeight

    This property holds the content height. It is used for calculating the
    total implicit height of the Popup.

    For more information, see \l {Popup Sizing}.

    \sa contentWidth
*/
qreal QQuickPopup::contentHeight() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->contentHeight();
}

void QQuickPopup::setContentHeight(qreal height)
{
    Q_D(QQuickPopup);
    d->popupItem->setContentHeight(height);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::availableWidth
    \readonly

    This property holds the width available to the \l contentItem after
    deducting horizontal padding from the \l {Item::}{width} of the popup.

    \sa padding, leftPadding, rightPadding
*/
qreal QQuickPopup::availableWidth() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->availableWidth();
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::availableHeight
    \readonly

    This property holds the height available to the \l contentItem after
    deducting vertical padding from the \l {Item::}{height} of the popup.

    \sa padding, topPadding, bottomPadding
*/
qreal QQuickPopup::availableHeight() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->availableHeight();
}

/*!
    \since QtQuick.Controls 2.1 (Qt 5.8)
    \qmlproperty real QtQuick.Controls::Popup::spacing

    This property holds the spacing.

    Spacing is useful for popups that have multiple or repetitive building
    blocks. For example, some styles use spacing to determine the distance
    between the header, content, and footer of \l Dialog. Spacing is not
    enforced by Popup, so each style may interpret it differently, and some
    may ignore it altogether.
*/
qreal QQuickPopup::spacing() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->spacing();
}

void QQuickPopup::setSpacing(qreal spacing)
{
    Q_D(QQuickPopup);
    d->popupItem->setSpacing(spacing);
}

void QQuickPopup::resetSpacing()
{
    setSpacing(0);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::margins

    This property holds the distance between the edges of the popup and the
    edges of its window.

    A popup with negative margins is not pushed within the bounds
    of the enclosing window. The default value is \c -1.

    \sa topMargin, leftMargin, rightMargin, bottomMargin, {Popup Layout}
*/
qreal QQuickPopup::margins() const
{
    Q_D(const QQuickPopup);
    return d->margins;
}

void QQuickPopup::setMargins(qreal margins)
{
    Q_D(QQuickPopup);
    if (qFuzzyCompare(d->margins, margins))
        return;
    QMarginsF oldMargins(leftMargin(), topMargin(), rightMargin(), bottomMargin());
    d->margins = margins;
    emit marginsChanged();
    QMarginsF newMargins(leftMargin(), topMargin(), rightMargin(), bottomMargin());
    if (!qFuzzyCompare(newMargins.top(), oldMargins.top()))
        emit topMarginChanged();
    if (!qFuzzyCompare(newMargins.left(), oldMargins.left()))
        emit leftMarginChanged();
    if (!qFuzzyCompare(newMargins.right(), oldMargins.right()))
        emit rightMarginChanged();
    if (!qFuzzyCompare(newMargins.bottom(), oldMargins.bottom()))
        emit bottomMarginChanged();
    marginsChange(newMargins, oldMargins);
}

void QQuickPopup::resetMargins()
{
    setMargins(-1);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::topMargin

    This property holds the distance between the top edge of the popup and
    the top edge of its window.

    A popup with a negative top margin is not pushed within the top edge
    of the enclosing window. The default value is \c -1.

    \sa margins, bottomMargin, {Popup Layout}
*/
qreal QQuickPopup::topMargin() const
{
    Q_D(const QQuickPopup);
    if (d->hasTopMargin)
        return d->topMargin;
    return d->margins;
}

void QQuickPopup::setTopMargin(qreal margin)
{
    Q_D(QQuickPopup);
    d->setTopMargin(margin);
}

void QQuickPopup::resetTopMargin()
{
    Q_D(QQuickPopup);
    d->setTopMargin(-1, true);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::leftMargin

    This property holds the distance between the left edge of the popup and
    the left edge of its window.

    A popup with a negative left margin is not pushed within the left edge
    of the enclosing window. The default value is \c -1.

    \sa margins, rightMargin, {Popup Layout}
*/
qreal QQuickPopup::leftMargin() const
{
    Q_D(const QQuickPopup);
    if (d->hasLeftMargin)
        return d->leftMargin;
    return d->margins;
}

void QQuickPopup::setLeftMargin(qreal margin)
{
    Q_D(QQuickPopup);
    d->setLeftMargin(margin);
}

void QQuickPopup::resetLeftMargin()
{
    Q_D(QQuickPopup);
    d->setLeftMargin(-1, true);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::rightMargin

    This property holds the distance between the right edge of the popup and
    the right edge of its window.

    A popup with a negative right margin is not pushed within the right edge
    of the enclosing window. The default value is \c -1.

    \sa margins, leftMargin, {Popup Layout}
*/
qreal QQuickPopup::rightMargin() const
{
    Q_D(const QQuickPopup);
    if (d->hasRightMargin)
        return d->rightMargin;
    return d->margins;
}

void QQuickPopup::setRightMargin(qreal margin)
{
    Q_D(QQuickPopup);
    d->setRightMargin(margin);
}

void QQuickPopup::resetRightMargin()
{
    Q_D(QQuickPopup);
    d->setRightMargin(-1, true);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::bottomMargin

    This property holds the distance between the bottom edge of the popup and
    the bottom edge of its window.

    A popup with a negative bottom margin is not pushed within the bottom edge
    of the enclosing window. The default value is \c -1.

    \sa margins, topMargin, {Popup Layout}
*/
qreal QQuickPopup::bottomMargin() const
{
    Q_D(const QQuickPopup);
    if (d->hasBottomMargin)
        return d->bottomMargin;
    return d->margins;
}

void QQuickPopup::setBottomMargin(qreal margin)
{
    Q_D(QQuickPopup);
    d->setBottomMargin(margin);
}

void QQuickPopup::resetBottomMargin()
{
    Q_D(QQuickPopup);
    d->setBottomMargin(-1, true);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::padding

    This property holds the default padding.

    \include qquickpopup-padding.qdocinc

    \sa availableWidth, availableHeight, topPadding, leftPadding, rightPadding, bottomPadding
*/
qreal QQuickPopup::padding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->padding();
}

void QQuickPopup::setPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setPadding(padding);
}

void QQuickPopup::resetPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetPadding();
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::topPadding

    This property holds the top padding. Unless explicitly set, the value
    is equal to \c verticalPadding.

    \include qquickpopup-padding.qdocinc

    \sa padding, bottomPadding, verticalPadding, availableHeight
*/
qreal QQuickPopup::topPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->topPadding();
}

void QQuickPopup::setTopPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setTopPadding(padding);
}

void QQuickPopup::resetTopPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetTopPadding();
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::leftPadding

    This property holds the left padding. Unless explicitly set, the value
    is equal to \c horizontalPadding.

    \include qquickpopup-padding.qdocinc

    \sa padding, rightPadding, horizontalPadding, availableWidth
*/
qreal QQuickPopup::leftPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->leftPadding();
}

void QQuickPopup::setLeftPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setLeftPadding(padding);
}

void QQuickPopup::resetLeftPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetLeftPadding();
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::rightPadding

    This property holds the right padding. Unless explicitly set, the value
    is equal to \c horizontalPadding.

    \include qquickpopup-padding.qdocinc

    \sa padding, leftPadding, horizontalPadding, availableWidth
*/
qreal QQuickPopup::rightPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->rightPadding();
}

void QQuickPopup::setRightPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setRightPadding(padding);
}

void QQuickPopup::resetRightPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetRightPadding();
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::bottomPadding

    This property holds the bottom padding. Unless explicitly set, the value
    is equal to \c verticalPadding.

    \include qquickpopup-padding.qdocinc

    \sa padding, topPadding, verticalPadding, availableHeight
*/
qreal QQuickPopup::bottomPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->bottomPadding();
}

void QQuickPopup::setBottomPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setBottomPadding(padding);
}

void QQuickPopup::resetBottomPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetBottomPadding();
}

/*!
    \qmlproperty Locale QtQuick.Controls::Popup::locale

    This property holds the locale of the popup.

    \sa mirrored, {LayoutMirroring}{LayoutMirroring}
*/
QLocale QQuickPopup::locale() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->locale();
}

void QQuickPopup::setLocale(const QLocale &locale)
{
    Q_D(QQuickPopup);
    d->popupItem->setLocale(locale);
}

void QQuickPopup::resetLocale()
{
    Q_D(QQuickPopup);
    d->popupItem->resetLocale();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::Popup::mirrored
    \readonly

    This property holds whether the popup is mirrored.

    This property is provided for convenience. A popup is considered mirrored
    when its visual layout direction is right-to-left; that is, when using a
    right-to-left locale.

    \sa locale, {Right-to-left User Interfaces}
*/
bool QQuickPopup::isMirrored() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->isMirrored();
}

/*!
    \qmlproperty font QtQuick.Controls::Popup::font

    This property holds the font currently set for the popup.

    Popup propagates explicit font properties to its children. If you change a specific
    property on a popup's font, that property propagates to all of the popup's children,
    overriding any system defaults for that property.

    \code
    Popup {
        font.family: "Courier"

        Column {
            Label {
                text: qsTr("This will use Courier...")
            }

            Switch {
                text: qsTr("... and so will this")
            }
        }
    }
    \endcode

    \sa Control::font, ApplicationWindow::font
*/
QFont QQuickPopup::font() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->font();
}

void QQuickPopup::setFont(const QFont &font)
{
    Q_D(QQuickPopup);
    d->popupItem->setFont(font);
}

void QQuickPopup::resetFont()
{
    Q_D(QQuickPopup);
    d->popupItem->resetFont();
}

QQuickWindow *QQuickPopup::window() const
{
    Q_D(const QQuickPopup);
    return d->window;
}

QQuickItem *QQuickPopup::popupItem() const
{
    Q_D(const QQuickPopup);
    return d->popupItem;
}

/*!
    \qmlproperty Item QtQuick.Controls::Popup::parent

    This property holds the parent item.
*/
QQuickItem *QQuickPopup::parentItem() const
{
    Q_D(const QQuickPopup);
    return d->parentItem;
}

void QQuickPopup::setParentItem(QQuickItem *parent)
{
    Q_D(QQuickPopup);
    if (d->parentItem == parent)
        return;

    if (d->parentItem) {
        QObjectPrivate::disconnect(d->parentItem, &QQuickItem::windowChanged, d, &QQuickPopupPrivate::setWindow);
        QQuickItemPrivate::get(d->parentItem)->removeItemChangeListener(d, QQuickItemPrivate::Destroyed);
    }
    d->parentItem = parent;
    QQuickPopupPositioner *positioner = d->getPositioner();
    if (positioner->parentItem())
        positioner->setParentItem(parent);
    if (parent) {
        QObjectPrivate::connect(parent, &QQuickItem::windowChanged, d, &QQuickPopupPrivate::setWindow);
        QQuickItemPrivate::get(d->parentItem)->addItemChangeListener(d, QQuickItemPrivate::Destroyed);
    } else if (d->inDestructor) {
        d->destroyDimmer();
    } else {
        // Reset transition manager state when its parent window destroyed
        if (!d->window && d->transitionManager.isRunning()) {
            if (d->transitionState == QQuickPopupPrivate::EnterTransition)
                d->finalizeEnterTransition();
            else if (d->transitionState == QQuickPopupPrivate::ExitTransition)
                d->finalizeExitTransition();
        }
        // NOTE: if setParentItem is called from the dtor, this bypasses virtual dispatch and calls
        // QQuickPopup::close() directly
        close();
    }
    d->setWindow(parent ? parent->window() : nullptr);
    emit parentChanged();
}

void QQuickPopup::resetParentItem()
{
    if (QQuickWindow *window = qobject_cast<QQuickWindow *>(parent()))
        setParentItem(window->contentItem());
    else
        setParentItem(findParentItem());
}

/*!
    \qmlproperty Item QtQuick.Controls::Popup::background

    This property holds the background item.

    \note If the background item has no explicit size specified, it automatically
          follows the popup's size. In most cases, there is no need to specify
          width or height for a background item.

    \note Most popups use the implicit size of the background item to calculate
    the implicit size of the popup itself. If you replace the background item
    with a custom one, you should also consider providing a sensible implicit
    size for it (unless it is an item like \l Image which has its own implicit
    size).

    \sa {Customizing Popup}
*/
QQuickItem *QQuickPopup::background() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->background();
}

void QQuickPopup::setBackground(QQuickItem *background)
{
    Q_D(QQuickPopup);
    // The __notCustomizable property won't be on "this" when the popup item's setBackground function
    // is called, so it won't warn. That's why we do a check here.
    QQuickControlPrivate::warnIfCustomizationNotSupported(this, background, QStringLiteral("background"));
    d->popupItem->setBackground(background);
}

/*!
    \qmlproperty Item QtQuick.Controls::Popup::contentItem

    This property holds the content item of the popup.

    The content item is the visual implementation of the popup. When the
    popup is made visible, the content item is automatically reparented to
    the \l {Overlay::overlay}{overlay item}.

    \note The content item is automatically resized to fit within the
    \l padding of the popup.

    \note Most popups use the implicit size of the content item to calculate
    the implicit size of the popup itself. If you replace the content item
    with a custom one, you should also consider providing a sensible implicit
    size for it (unless it is an item like \l Text which has its own implicit
    size).

    \sa {Customizing Popup}
*/
QQuickItem *QQuickPopup::contentItem() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->contentItem();
}

void QQuickPopup::setContentItem(QQuickItem *item)
{
    Q_D(QQuickPopup);
    // See comment in setBackground for why we do this.
    QQuickControlPrivate::warnIfCustomizationNotSupported(this, item, QStringLiteral("contentItem"));
    QQuickItem *oldContentItem = d->complete ? d->popupItem->d_func()->contentItem.data()
                                             : nullptr;
    if (oldContentItem)
        disconnect(oldContentItem, &QQuickItem::childrenChanged, this, &QQuickPopup::contentChildrenChanged);
    d->popupItem->setContentItem(item);
    if (d->complete) {
        QQuickItem *newContentItem = d->popupItem->d_func()->contentItem.data();
        connect(newContentItem, &QQuickItem::childrenChanged, this, &QQuickPopup::contentChildrenChanged);
        if (oldContentItem != newContentItem)
            emit contentChildrenChanged();
    }
}

/*!
    \qmlproperty list<QtObject> QtQuick.Controls::Popup::contentData
    \qmldefault

    This property holds the list of content data.

    The list contains all objects that have been declared in QML as children
    of the popup.

    \note Unlike \c contentChildren, \c contentData does include non-visual QML
    objects.

    \sa Item::data, contentChildren
*/
QQmlListProperty<QObject> QQuickPopupPrivate::contentData()
{
    QQuickControlPrivate *p = QQuickControlPrivate::get(popupItem);
    if (!p->contentItem)
        p->executeContentItem();
    return QQmlListProperty<QObject>(popupItem->contentItem(), nullptr,
                                     QQuickItemPrivate::data_append,
                                     QQuickItemPrivate::data_count,
                                     QQuickItemPrivate::data_at,
                                     QQuickItemPrivate::data_clear);
}

/*!
    \qmlproperty list<Item> QtQuick.Controls::Popup::contentChildren

    This property holds the list of content children.

    The list contains all items that have been declared in QML as children
    of the popup.

    \note Unlike \c contentData, \c contentChildren does not include non-visual
    QML objects.

    \sa Item::children, contentData
*/
QQmlListProperty<QQuickItem> QQuickPopupPrivate::contentChildren()
{
    return QQmlListProperty<QQuickItem>(popupItem->contentItem(), nullptr,
                                        QQuickItemPrivate::children_append,
                                        QQuickItemPrivate::children_count,
                                        QQuickItemPrivate::children_at,
                                        QQuickItemPrivate::children_clear);
}

/*!
    \qmlproperty bool QtQuick.Controls::Popup::clip

    This property holds whether clipping is enabled. The default value is \c false.
    Clipping only works when the popup isn't in its own window.
*/
bool QQuickPopup::clip() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->clip() && !d->usePopupWindow();
}

void QQuickPopup::setClip(bool clip)
{
    Q_D(QQuickPopup);
    if (clip == d->popupItem->clip() || d->usePopupWindow())
        return;
    d->popupItem->setClip(clip);
    emit clipChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Popup::focus

    This property holds whether the popup wants focus.

    When the popup actually receives focus, \l activeFocus will be \c true.
    For more information, see \l {Keyboard Focus in Qt Quick}.

    The default value is \c false.

    \sa activeFocus
*/
bool QQuickPopup::hasFocus() const
{
    Q_D(const QQuickPopup);
    return d->focus;
}

void QQuickPopup::setFocus(bool focus)
{
    Q_D(QQuickPopup);
    if (d->focus == focus)
        return;
    d->focus = focus;
    emit focusChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Popup::activeFocus
    \readonly

    This property holds whether the popup has active focus.

    \sa focus, {Keyboard Focus in Qt Quick}
*/
bool QQuickPopup::hasActiveFocus() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->hasActiveFocus();
}

/*!
    \qmlproperty bool QtQuick.Controls::Popup::modal

    This property holds whether the popup is modal.

    Modal popups often have a distinctive background dimming effect defined
    in \l {Overlay::modal}{Overlay.modal}, and do not allow press
    or release events through to items beneath them. For example, if the user
    accidentally clicks outside of a popup, any item beneath that popup at
    the location of the click will not receive the event.

    On desktop platforms, it is common for modal popups to be closed only when
    the escape key is pressed. To achieve this behavior, set
    \l closePolicy to \c Popup.CloseOnEscape. By default, \c closePolicy
    is set to \c {Popup.CloseOnEscape | Popup.CloseOnPressOutside}, which
    means that clicking outside of a modal popup will close it.

    The default value is \c false.

    \sa dim
*/
bool QQuickPopup::isModal() const
{
    Q_D(const QQuickPopup);
    return d->modal;
}

void QQuickPopup::setModal(bool modal)
{
    Q_D(QQuickPopup);
    if (d->modal == modal)
        return;
    d->modal = modal;
    d->popupWindowDirty = true;
    if (d->complete && d->visible)
        d->toggleOverlay();
    emit modalChanged();

    if (!d->hasDim) {
        setDim(modal);
        d->hasDim = false;
    }
}

/*!
    \qmlproperty bool QtQuick.Controls::Popup::dim

    This property holds whether the popup dims the background.

    Unless explicitly set, this property follows the value of \l modal. To
    return to the default value, set this property to \c undefined.

    \sa modal, {Overlay::modeless}{Overlay.modeless}
*/
bool QQuickPopup::dim() const
{
    Q_D(const QQuickPopup);
    return d->dim;
}

void QQuickPopup::setDim(bool dim)
{
    Q_D(QQuickPopup);
    d->hasDim = true;

    if (d->dim == dim)
        return;

    d->dim = dim;
    if (d->complete && d->visible)
        d->toggleOverlay();
    emit dimChanged();
}

void QQuickPopup::resetDim()
{
    Q_D(QQuickPopup);
    if (!d->hasDim)
        return;

    setDim(d->modal);
    d->hasDim = false;
}

/*!
    \qmlproperty bool QtQuick.Controls::Popup::visible

    This property holds whether the popup is visible. The default value is \c false.

    \sa open(), close(), opened
*/
bool QQuickPopup::isVisible() const
{
    Q_D(const QQuickPopup);
    return d->visible && d->popupItem->isVisible();
}

void QQuickPopup::setVisible(bool visible)
{
    Q_D(QQuickPopup);
    // During an exit transition, d->visible == true until the transition has completed.
    // Therefore, this guard must not return early if setting visible to true while
    // d->visible is true.
    if (d->visible && visible && d->transitionState != QQuickPopupPrivate::ExitTransition)
        return;
    if (!d->visible && !visible)
        return;

    if (!d->complete || (visible && !d->window)) {
        d->visible = visible;
        return;
    }

    if (visible)
        d->transitionManager.transitionEnter();
    else
        d->transitionManager.transitionExit();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::Popup::enabled

    This property holds whether the popup is enabled. The default value is \c true.

    \sa visible, Item::enabled
*/
bool QQuickPopup::isEnabled() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->isEnabled();
}

void QQuickPopup::setEnabled(bool enabled)
{
    Q_D(QQuickPopup);
    d->popupItem->setEnabled(enabled);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::Popup::opened

    This property holds whether the popup is fully open. The popup is considered opened
    when it's visible and neither the \l enter nor \l exit transitions are running.

    \sa open(), close(), visible
*/
bool QQuickPopup::isOpened() const
{
    Q_D(const QQuickPopup);
    return d->transitionState == QQuickPopupPrivate::NoTransition && isVisible();
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::opacity

    This property holds the opacity of the popup. Opacity is specified as a number between
    \c 0.0 (fully transparent) and \c 1.0 (fully opaque). The default value is \c 1.0.

    \sa visible
*/
qreal QQuickPopup::opacity() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->opacity();
}

void QQuickPopup::setOpacity(qreal opacity)
{
    Q_D(QQuickPopup);
    d->popupItem->setOpacity(opacity);
}

/*!
    \qmlproperty real QtQuick.Controls::Popup::scale

    This property holds the scale factor of the popup. The default value is \c 1.0.

    A scale of less than \c 1.0 causes the popup to be rendered at a smaller size,
    and a scale greater than \c 1.0 renders the popup at a larger size. Negative
    scales are not supported.
*/
qreal QQuickPopup::scale() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->scale();
}

void QQuickPopup::setScale(qreal scale)
{
    Q_D(QQuickPopup);
    if (qFuzzyCompare(scale, d->popupItem->scale()))
        return;
    d->popupItem->setScale(scale);
    emit scaleChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Popup::closePolicy

    This property determines the circumstances under which the popup closes.
    The flags can be combined to allow several ways of closing the popup.

    The available values are:
    \value Popup.NoAutoClose The popup will only close when manually instructed to do so.
    \value Popup.CloseOnPressOutside The popup will close when the mouse is pressed outside of it.
    \value Popup.CloseOnPressOutsideParent The popup will close when the mouse is pressed outside of its parent.
    \value Popup.CloseOnReleaseOutside The popup will close when the mouse is released outside of it.
    \value Popup.CloseOnReleaseOutsideParent The popup will close when the mouse is released outside of its parent.
    \value Popup.CloseOnEscape The popup will close when the escape key is pressed while the popup
        has active focus.

    The \c {CloseOnPress*} and \c {CloseOnRelease*} policies only apply for events
    outside of popups. That is, if there are two popups open and the first has
    \c Popup.CloseOnPressOutside as its policy, clicking on the second popup will
    not result in the first closing.

    The default value is \c {Popup.CloseOnEscape | Popup.CloseOnPressOutside}.

    \note There is a known limitation that the \c Popup.CloseOnReleaseOutside
        and \c Popup.CloseOnReleaseOutsideParent policies only work with
        \l modal popups.
*/
QQuickPopup::ClosePolicy QQuickPopup::closePolicy() const
{
    Q_D(const QQuickPopup);
    return d->closePolicy;
}

void QQuickPopup::setClosePolicy(ClosePolicy policy)
{
    Q_D(QQuickPopup);
    d->hasClosePolicy = true;
    if (d->closePolicy == policy)
        return;
    d->closePolicy = policy;
    emit closePolicyChanged();
}

void QQuickPopup::resetClosePolicy()
{
    Q_D(QQuickPopup);
    setClosePolicy(QQuickPopupPrivate::DefaultClosePolicy);
    d->hasClosePolicy = false;
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Popup::transformOrigin

    This property holds the origin point for transformations in enter and exit transitions.

    Nine transform origins are available, as shown in the image below.
    The default transform origin is \c Popup.Center.

    \image qtquickcontrols-popup-transformorigin.png

    \sa enter, exit, Item::transformOrigin
*/
QQuickPopup::TransformOrigin QQuickPopup::transformOrigin() const
{
    Q_D(const QQuickPopup);
    return static_cast<TransformOrigin>(d->popupItem->transformOrigin());
}

void QQuickPopup::setTransformOrigin(TransformOrigin origin)
{
    Q_D(QQuickPopup);
    d->popupItem->setTransformOrigin(static_cast<QQuickItem::TransformOrigin>(origin));
}

/*!
    \qmlproperty Transition QtQuick.Controls::Popup::enter

    This property holds the transition that is applied to the popup item
    when the popup is opened and enters the screen.

    The following example animates the opacity of the popup when it enters
    the screen:
    \code
    Popup {
        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
        }
    }
    \endcode

    \sa exit
*/
QQuickTransition *QQuickPopup::enter() const
{
    Q_D(const QQuickPopup);
    return d->enter;
}

void QQuickPopup::setEnter(QQuickTransition *transition)
{
    Q_D(QQuickPopup);
    if (d->enter == transition)
        return;
    d->enter = transition;
    emit enterChanged();
}

/*!
    \qmlproperty Transition QtQuick.Controls::Popup::exit

    This property holds the transition that is applied to the popup item
    when the popup is closed and exits the screen.

    The following example animates the opacity of the popup when it exits
    the screen:
    \code
    Popup {
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0 }
        }
    }
    \endcode

    \sa enter
*/
QQuickTransition *QQuickPopup::exit() const
{
    Q_D(const QQuickPopup);
    return d->exit;
}

void QQuickPopup::setExit(QQuickTransition *transition)
{
    Q_D(QQuickPopup);
    if (d->exit == transition)
        return;
    d->exit = transition;
    emit exitChanged();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::horizontalPadding

    This property holds the horizontal padding. Unless explicitly set, the value
    is equal to \c padding.

    \include qquickpopup-padding.qdocinc

    \sa padding, leftPadding, rightPadding, verticalPadding
*/
qreal QQuickPopup::horizontalPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->horizontalPadding();
}

void QQuickPopup::setHorizontalPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setHorizontalPadding(padding);
}

void QQuickPopup::resetHorizontalPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetHorizontalPadding();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::verticalPadding

    This property holds the vertical padding. Unless explicitly set, the value
    is equal to \c padding.

    \include qquickpopup-padding.qdocinc

    \sa padding, topPadding, bottomPadding, horizontalPadding
*/
qreal QQuickPopup::verticalPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->verticalPadding();
}

void QQuickPopup::setVerticalPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setVerticalPadding(padding);
}

void QQuickPopup::resetVerticalPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetVerticalPadding();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::implicitContentWidth
    \readonly

    This property holds the implicit content width.

    The value is calculated based on the content children.

    \sa implicitContentHeight, implicitBackgroundWidth
*/
qreal QQuickPopup::implicitContentWidth() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->implicitContentWidth();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::implicitContentHeight
    \readonly

    This property holds the implicit content height.

    The value is calculated based on the content children.

    \sa implicitContentWidth, implicitBackgroundHeight
*/
qreal QQuickPopup::implicitContentHeight() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->implicitContentHeight();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::implicitBackgroundWidth
    \readonly

    This property holds the implicit background width.

    The value is equal to \c {background ? background.implicitWidth : 0}.

    \sa implicitBackgroundHeight, implicitContentWidth
*/
qreal QQuickPopup::implicitBackgroundWidth() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->implicitBackgroundWidth();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::implicitBackgroundHeight
    \readonly

    This property holds the implicit background height.

    The value is equal to \c {background ? background.implicitHeight : 0}.

    \sa implicitBackgroundWidth, implicitContentHeight
*/
qreal QQuickPopup::implicitBackgroundHeight() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->implicitBackgroundHeight();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::topInset

    This property holds the top inset for the background.

    \sa {Popup Layout}, bottomInset
*/
qreal QQuickPopup::topInset() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->topInset();
}

void QQuickPopup::setTopInset(qreal inset)
{
    Q_D(QQuickPopup);
    d->popupItem->setTopInset(inset);
}

void QQuickPopup::resetTopInset()
{
    Q_D(QQuickPopup);
    d->popupItem->resetTopInset();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::leftInset

    This property holds the left inset for the background.

    \sa {Popup Layout}, rightInset
*/
qreal QQuickPopup::leftInset() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->leftInset();
}

void QQuickPopup::setLeftInset(qreal inset)
{
    Q_D(QQuickPopup);
    d->popupItem->setLeftInset(inset);
}

void QQuickPopup::resetLeftInset()
{
    Q_D(QQuickPopup);
    d->popupItem->resetLeftInset();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::rightInset

    This property holds the right inset for the background.

    \sa {Popup Layout}, leftInset
*/
qreal QQuickPopup::rightInset() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->rightInset();
}

void QQuickPopup::setRightInset(qreal inset)
{
    Q_D(QQuickPopup);
    d->popupItem->setRightInset(inset);
}

void QQuickPopup::resetRightInset()
{
    Q_D(QQuickPopup);
    d->popupItem->resetRightInset();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Popup::bottomInset

    This property holds the bottom inset for the background.

    \sa {Popup Layout}, topInset
*/
qreal QQuickPopup::bottomInset() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->bottomInset();
}

void QQuickPopup::setBottomInset(qreal inset)
{
    Q_D(QQuickPopup);
    d->popupItem->setBottomInset(inset);
}

void QQuickPopup::resetBottomInset()
{
    Q_D(QQuickPopup);
    d->popupItem->resetBottomInset();
}


/*!
    \qmlproperty enumeration QtQuick.Controls::Popup::popupType
    \since 6.8

    This property determines the type of popup that is preferred.

    Available options:
    \value Item         The popup will be embedded into the
                        \l{Showing a popup as an item}{same scene as the parent},
                        without the use of a separate window.
    \value Window       The popup will be presented in a \l {Popup type}
                        {separate window}. If the platform doesn't support
                        multiple windows, \c Popup.Item will be used instead.
    \value Native       The popup will be native to the platform. If the
                        platform doesn't support native popups, \c Popup.Window
                        will be used instead.

    Whether a popup will be able to use the preferred type depends on the platform.
    \c Popup.Item is supported on all platforms, but \c Popup.Window and \c Popup.Native
    are normally only supported on desktop platforms. Additionally, if a popup is a
    \l Menu inside a \l {Native menu bars}{native menubar}, the menu will be native as
    well. And if the menu is a sub-menu inside another menu, the parent (or root) menu
    will decide the type.

    The default value is usually \c Popup.Item, with some exceptions, mentioned above.
    This might change in future versions of Qt, for certain styles and platforms that benefit
    from using other popup types.
    If you always want to use native menus for all styles on macOS, for example, you can do:

    \code
    Menu {
        popupType: Qt.platform.os === "osx" ? Popup.Native : Popup.Window
    }
    \endcode

    Also, if you choose to customize a popup (by for example changing any of the
    delegates), you should consider setting the popup type to be \c Popup.Window
    as well. This will ensure that your changes will be visible on all platforms
    and for all styles. Otherwise, when native menus are being used, the delegates
    will \e not be used for rendering.

    \sa {Popup type}
*/
QQuickPopup::PopupType QQuickPopup::popupType() const
{
    Q_D(const QQuickPopup);
    return d->popupType;
}

void QQuickPopup::setPopupType(PopupType popupType)
{
    Q_D(QQuickPopup);
    if (d->popupType == popupType)
        return;

    d->popupType = popupType;

    emit popupTypeChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty palette QtQuick.Controls::Popup::palette

    This property holds the palette currently set for the popup.

    Popup propagates explicit palette properties to its children. If you change a specific
    property on a popup's palette, that property propagates to all of the popup's children,
    overriding any system defaults for that property.

    \code
    Popup {
        palette.text: "red"

        Column {
            Label {
                text: qsTr("This will use red color...")
            }

            Switch {
                text: qsTr("... and so will this")
            }
        }
    }
    \endcode

    \b {See also}: \l Item::palette, \l Window::palette, \l ColorGroup,
       \l [QML] {Palette}
*/

bool QQuickPopup::filtersChildMouseEvents() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->filtersChildMouseEvents();
}

void QQuickPopup::setFiltersChildMouseEvents(bool filter)
{
    Q_D(QQuickPopup);
    d->popupItem->setFiltersChildMouseEvents(filter);
}

/*!
    \qmlmethod QtQuick.Controls::Popup::forceActiveFocus(enumeration reason = Qt.OtherFocusReason)

    Forces active focus on the popup with the given \a reason.

    This method sets focus on the popup and ensures that all ancestor
    \l FocusScope objects in the object hierarchy are also given \l focus.

    \sa activeFocus, Qt::FocusReason
*/
void QQuickPopup::forceActiveFocus(Qt::FocusReason reason)
{
    Q_D(QQuickPopup);
    d->popupItem->forceActiveFocus(reason);
}

void QQuickPopup::classBegin()
{
    Q_D(QQuickPopup);
    d->complete = false;
    QQmlContext *context = qmlContext(this);
    if (context)
        QQmlEngine::setContextForObject(d->popupItem, context);
    d->popupItem->classBegin();
}

void QQuickPopup::componentComplete()
{
    Q_D(QQuickPopup);
    qCDebug(lcQuickPopup) << "componentComplete" << this;
    if (!parentItem())
        resetParentItem();

    if (d->visible && d->window)
        d->transitionManager.transitionEnter();

    d->complete = true;
    d->popupItem->setObjectName(QQmlMetaType::prettyTypeName(this));
    d->popupItem->componentComplete();

    if (auto currentContentItem = d->popupItem->d_func()->contentItem.data()) {
        connect(currentContentItem, &QQuickItem::childrenChanged,
            this, &QQuickPopup::contentChildrenChanged);
    }
}

bool QQuickPopup::isComponentComplete() const
{
    Q_D(const QQuickPopup);
    return d->complete;
}

bool QQuickPopup::childMouseEventFilter(QQuickItem *child, QEvent *event)
{
    Q_UNUSED(child);
    Q_UNUSED(event);
    return false;
}

void QQuickPopup::focusInEvent(QFocusEvent *event)
{
    event->accept();
}

void QQuickPopup::focusOutEvent(QFocusEvent *event)
{
    event->accept();
}

void QQuickPopup::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickPopup);
    if (!hasActiveFocus())
        return;

#if QT_CONFIG(shortcut)
    if (d->closePolicy.testFlag(QQuickPopup::CloseOnEscape)
        && (event->matches(QKeySequence::Cancel)
#if defined(Q_OS_ANDROID)
        || event->key() == Qt::Key_Back
#endif
        )) {
        event->accept();
        if (d->interactive)
            d->closeOrReject();
        return;
    }
#endif

    if (hasActiveFocus() && (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab)) {
        event->accept();
        QQuickItemPrivate::focusNextPrev(d->popupItem, event->key() == Qt::Key_Tab);
    }
}

void QQuickPopup::keyReleaseEvent(QKeyEvent *event)
{
    event->accept();
}

void QQuickPopup::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickPopup);
    event->setAccepted(d->handleMouseEvent(d->popupItem, event));
}

void QQuickPopup::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickPopup);
    event->setAccepted(d->handleMouseEvent(d->popupItem, event));
}

void QQuickPopup::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickPopup);
    event->setAccepted(d->handleMouseEvent(d->popupItem, event));
}

void QQuickPopup::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();
}

void QQuickPopup::mouseUngrabEvent()
{
    Q_D(QQuickPopup);
    d->handleUngrab();
}

/*!
    \internal

    Called whenever the window receives a Wheel/Hover/Mouse/Touch event,
    and has an active popup (with popupType: Popup.Item) in its scene.

    The purpose is to close popups when the press/release event happened outside of it,
    and the closePolicy allows for it to happen.

    If the function is called from childMouseEventFilter, then the return value of this
    function will determine whether the event will be filtered, or delivered to \a item.
*/
bool QQuickPopup::overlayEvent(QQuickItem *item, QEvent *event)
{
    Q_D(QQuickPopup);
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseMove:
    case QEvent::Wheel:
        if (d->modal)
            event->accept();
        return d->modal;

#if QT_CONFIG(quicktemplates2_multitouch)
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        return d->handleTouchEvent(item, static_cast<QTouchEvent *>(event));
#endif
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
    case QEvent::HoverLeave:
        return d->handleHoverEvent(item, static_cast<QHoverEvent *>(event));

    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        return d->handleMouseEvent(item, static_cast<QMouseEvent *>(event));

    default:
        return false;
    }
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickPopup::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickPopup);
    d->handleTouchEvent(d->popupItem, event);
}

void QQuickPopup::touchUngrabEvent()
{
    Q_D(QQuickPopup);
    d->handleUngrab();
}
#endif

#if QT_CONFIG(wheelevent)
void QQuickPopup::wheelEvent(QWheelEvent *event)
{
    event->accept();
}
#endif

void QQuickPopup::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_UNUSED(newItem);
    Q_UNUSED(oldItem);
}

void QQuickPopup::contentSizeChange(const QSizeF &newSize, const QSizeF &oldSize)
{
    qCDebug(lcQuickPopup) << "contentSizeChange called on" << this << "with newSize" << newSize << "oldSize" << oldSize;
    if (!qFuzzyCompare(newSize.width(), oldSize.width()))
        emit contentWidthChanged();
    if (!qFuzzyCompare(newSize.height(), oldSize.height()))
        emit contentHeightChanged();
}

void QQuickPopup::fontChange(const QFont &newFont, const QFont &oldFont)
{
    Q_UNUSED(newFont);
    Q_UNUSED(oldFont);
    emit fontChanged();
}

void QQuickPopup::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickPopup);
    qCDebug(lcQuickPopup) << "geometryChange called on" << this << "with newGeometry" << newGeometry << "oldGeometry" << oldGeometry;
    if (!d->usePopupWindow())
        d->reposition();
    if (!qFuzzyCompare(newGeometry.width(), oldGeometry.width())) {
        emit widthChanged();
        emit availableWidthChanged();
    }
    if (!qFuzzyCompare(newGeometry.height(), oldGeometry.height())) {
        emit heightChanged();
        emit availableHeightChanged();
    }
}

void QQuickPopup::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &)
{
    switch (change) {
    case QQuickItem::ItemActiveFocusHasChanged:
        emit activeFocusChanged();
        break;
    case QQuickItem::ItemOpacityHasChanged:
        emit opacityChanged();
        break;
    default:
        break;
    }
}

void QQuickPopup::localeChange(const QLocale &newLocale, const QLocale &oldLocale)
{
    Q_UNUSED(newLocale);
    Q_UNUSED(oldLocale);
    emit localeChanged();
}

void QQuickPopup::marginsChange(const QMarginsF &newMargins, const QMarginsF &oldMargins)
{
    Q_D(QQuickPopup);
    Q_UNUSED(newMargins);
    Q_UNUSED(oldMargins);
    d->reposition();
}

void QQuickPopup::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    const bool tp = !qFuzzyCompare(newPadding.top(), oldPadding.top());
    const bool lp = !qFuzzyCompare(newPadding.left(), oldPadding.left());
    const bool rp = !qFuzzyCompare(newPadding.right(), oldPadding.right());
    const bool bp = !qFuzzyCompare(newPadding.bottom(), oldPadding.bottom());

    if (tp)
        emit topPaddingChanged();
    if (lp)
        emit leftPaddingChanged();
    if (rp)
        emit rightPaddingChanged();
    if (bp)
        emit bottomPaddingChanged();

    if (lp || rp) {
        emit horizontalPaddingChanged();
        emit availableWidthChanged();
    }
    if (tp || bp) {
        emit verticalPaddingChanged();
        emit availableHeightChanged();
    }
}

void QQuickPopup::spacingChange(qreal newSpacing, qreal oldSpacing)
{
    Q_UNUSED(newSpacing);
    Q_UNUSED(oldSpacing);
    emit spacingChanged();
}

void QQuickPopup::insetChange(const QMarginsF &newInset, const QMarginsF &oldInset)
{
    if (!qFuzzyCompare(newInset.top(), oldInset.top()))
        emit topInsetChanged();
    if (!qFuzzyCompare(newInset.left(), oldInset.left()))
        emit leftInsetChanged();
    if (!qFuzzyCompare(newInset.right(), oldInset.right()))
        emit rightInsetChanged();
    if (!qFuzzyCompare(newInset.bottom(), oldInset.bottom()))
        emit bottomInsetChanged();
}

QFont QQuickPopup::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::System);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickPopup::effectiveAccessibleRole() const
{
    auto *attached = qmlAttachedPropertiesObject<QQuickAccessibleAttached>(this, false);

    auto role = QAccessible::NoRole;
    if (auto *accessibleAttached = qobject_cast<QQuickAccessibleAttached *>(attached))
        role = accessibleAttached->role();
    if (role == QAccessible::NoRole)
        role = accessibleRole();

    return role;
}

QAccessible::Role QQuickPopup::accessibleRole() const
{
    return QAccessible::Dialog;
}

void QQuickPopup::accessibilityActiveChanged(bool active)
{
    Q_UNUSED(active);
}
#endif

QString QQuickPopup::accessibleName() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->accessibleName();
}

void QQuickPopup::maybeSetAccessibleName(const QString &name)
{
    Q_D(QQuickPopup);
    d->popupItem->maybeSetAccessibleName(name);
}

QVariant QQuickPopup::accessibleProperty(const char *propertyName)
{
    Q_D(const QQuickPopup);
    return d->popupItem->accessibleProperty(propertyName);
}

bool QQuickPopup::setAccessibleProperty(const char *propertyName, const QVariant &value)
{
    Q_D(QQuickPopup);
    return d->popupItem->setAccessibleProperty(propertyName, value);
}

QQuickItem *QQuickPopup::safeAreaAttachmentItem()
{
    return popupItem();
}

QT_END_NAMESPACE

#include "moc_qquickpopup_p.cpp"
