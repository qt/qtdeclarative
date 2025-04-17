// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcontrol_p.h"
#include "qquickcontrol_p_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include "qquicklabel_p.h"
#include "qquicklabel_p_p.h"
#include "qquicktemplatesutils_p.h"
#include "qquicktextarea_p.h"
#include "qquicktextarea_p_p.h"
#include "qquicktextfield_p.h"
#include "qquicktextfield_p_p.h"
#include "qquickpopup_p.h"
#include "qquickpopupitem_p_p.h"
#include "qquickapplicationwindow_p.h"
#include "qquickapplicationwindow_p_p.h"
#include "qquickdeferredexecute_p_p.h"
#include "qquickcontentitem_p.h"

#if QT_CONFIG(accessibility)
#include <QtQuick/private/qquickaccessibleattached_p.h>
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcItemManagement, "qt.quick.controls.control.itemmanagement")

/*!
    \qmltype Control
    \inherits Item
//!     \nativetype QQuickControl
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \brief Abstract base type providing functionality common to all controls.

    Control is the base type of user interface controls.  It receives input
    events from the window system, and paints a representation of itself on
    the screen.

    \section1 Control Layout

    The following diagram illustrates the layout of a typical control:

    \image qtquickcontrols-control.png

    The \l {Item::}{implicitWidth} and \l {Item::}{implicitHeight} of a control
    are typically based on the implicit sizes of the background and the content
    item plus any insets and paddings. These properties determine how large
    the control will be when no explicit \l {Item::}{width} or
    \l {Item::}{height} is specified.

    The geometry of the \l {Control::}{contentItem} is determined by the padding.
    The following example reserves 10px padding between the boundaries of the
    control and its content:

    \code
    Control {
        padding: 10

        contentItem: Text {
            text: "Content"
        }
    }
    \endcode

    The \l {Control::}{background} item fills the entire width and height of the
    control, unless insets or an explicit size have been given for it. Background
    insets are useful for extending the touchable/interactive area of a control
    without affecting its visual size. This is often used on touch devices to
    ensure that a control is not too small to be interacted with by the user.
    Insets affect the size of the control, and hence will affect how much space
    they take up in a layout, for example.

    Negative insets can be used to make the background larger than the control.
    The following example uses negative insets to place a shadow outside the
    control's boundaries:

    \code
    Control {
        topInset: -2
        leftInset: -2
        rightInset: -6
        bottomInset: -6

        background: BorderImage {
            source: ":/images/shadowed-background.png"
        }
    }
    \endcode

    \section1 Event Handling

    All controls, except non-interactive indicators, do not let clicks and
    touches through to items below them. For example, the \c console.log()
    call in the example below will never be executed when clicking on the
    Pane, because the \l MouseArea is below it in the scene:

    \code
    MouseArea {
        anchors.fill: parent
        onClicked: console.log("MouseArea was clicked")

        Pane {
            anchors.fill: parent
        }
    }
    \endcode

    Wheel events are consumed by controls if \l wheelEnabled is \c true.

    \sa ApplicationWindow, Container, {Using Qt Quick Controls types in
        property declarations}
*/

const QQuickItemPrivate::ChangeTypes QQuickControlPrivate::ImplicitSizeChanges = QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight | QQuickItemPrivate::Destroyed;

static bool isKeyFocusReason(Qt::FocusReason reason)
{
    return reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason || reason == Qt::ShortcutFocusReason;
}

QQuickControlPrivate::QQuickControlPrivate()
{
#if QT_CONFIG(accessibility)
    QAccessible::installActivationObserver(this);
#endif
}

QQuickControlPrivate::~QQuickControlPrivate()
{
}

void QQuickControlPrivate::init()
{
    Q_Q(QQuickControl);
    QObject::connect(q, &QQuickItem::baselineOffsetChanged, q, &QQuickControl::baselineOffsetChanged);
}

#if QT_CONFIG(quicktemplates2_multitouch)
bool QQuickControlPrivate::acceptTouch(const QTouchEvent::TouchPoint &point)
{
    if (point.id() == touchId)
        return true;

    if (touchId == -1 && point.state() == QEventPoint::Pressed) {
        touchId = point.id();
        return true;
    }

    return false;
}
#endif

bool QQuickControlPrivate::handlePress(const QPointF &, ulong)
{
    return true;
}

bool QQuickControlPrivate::handleMove(const QPointF &point, ulong)
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_Q(QQuickControl);
    q->setHovered(hoverEnabled && q->contains(point));
#else
    Q_UNUSED(point);
#endif
    return true;
}

bool QQuickControlPrivate::handleRelease(const QPointF &, ulong)
{
    touchId = -1;
    return true;
}

void QQuickControlPrivate::handleUngrab()
{
    touchId = -1;
}

void QQuickControlPrivate::mirrorChange()
{
    Q_Q(QQuickControl);
    q->mirrorChange();
}

void QQuickControlPrivate::setTopPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldPadding = getPadding();
    extra.value().topPadding = value;
    extra.value().hasTopPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding.top(), value)) || (reset && !qFuzzyCompare(oldPadding.top(), getVerticalPadding()))) {
        emit q->topPaddingChanged();
        emit q->availableHeightChanged();
        q->paddingChange(getPadding(), oldPadding);
    }
}

void QQuickControlPrivate::setLeftPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldPadding = getPadding();
    extra.value().leftPadding = value;
    extra.value().hasLeftPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding.left(), value)) || (reset && !qFuzzyCompare(oldPadding.left(), getHorizontalPadding()))) {
        emit q->leftPaddingChanged();
        emit q->availableWidthChanged();
        q->paddingChange(getPadding(), oldPadding);
    }
}

void QQuickControlPrivate::setRightPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldPadding = getPadding();
    extra.value().rightPadding = value;
    extra.value().hasRightPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding.right(), value)) || (reset && !qFuzzyCompare(oldPadding.right(), getHorizontalPadding()))) {
        emit q->rightPaddingChanged();
        emit q->availableWidthChanged();
        q->paddingChange(getPadding(), oldPadding);
    }
}

void QQuickControlPrivate::setBottomPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldPadding = getPadding();
    extra.value().bottomPadding = value;
    extra.value().hasBottomPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding.bottom(), value)) || (reset && !qFuzzyCompare(oldPadding.bottom(), getVerticalPadding()))) {
        emit q->bottomPaddingChanged();
        emit q->availableHeightChanged();
        q->paddingChange(getPadding(), oldPadding);
    }
}

void QQuickControlPrivate::setHorizontalPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldPadding = getPadding();
    const qreal oldHorizontalPadding = getHorizontalPadding();
    horizontalPadding = value;
    hasHorizontalPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldHorizontalPadding, value)) || (reset && !qFuzzyCompare(oldHorizontalPadding, padding))) {
        const QMarginsF newPadding = getPadding();
        if (!qFuzzyCompare(newPadding.left(), oldPadding.left()))
            emit q->leftPaddingChanged();
        if (!qFuzzyCompare(newPadding.right(), oldPadding.right()))
            emit q->rightPaddingChanged();
        emit q->horizontalPaddingChanged();
        emit q->availableWidthChanged();
        q->paddingChange(newPadding, oldPadding);
    }
}

void QQuickControlPrivate::setVerticalPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldPadding = getPadding();
    const qreal oldVerticalPadding = getVerticalPadding();
    verticalPadding = value;
    hasVerticalPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldVerticalPadding, value)) || (reset && !qFuzzyCompare(oldVerticalPadding, padding))) {
        const QMarginsF newPadding = getPadding();
        if (!qFuzzyCompare(newPadding.top(), oldPadding.top()))
            emit q->topPaddingChanged();
        if (!qFuzzyCompare(newPadding.bottom(), oldPadding.bottom()))
            emit q->bottomPaddingChanged();
        emit q->verticalPaddingChanged();
        emit q->availableHeightChanged();
        q->paddingChange(newPadding, oldPadding);
    }
}

void QQuickControlPrivate::setTopInset(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldInset = getInset();
    extra.value().topInset = value;
    extra.value().hasTopInset = !reset;
    if (!qFuzzyCompare(oldInset.top(), value)) {
        emit q->topInsetChanged();
        q->insetChange(getInset(), oldInset);
    }
}

void QQuickControlPrivate::setLeftInset(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldInset = getInset();
    extra.value().leftInset = value;
    extra.value().hasLeftInset = !reset;
    if (!qFuzzyCompare(oldInset.left(), value)) {
        emit q->leftInsetChanged();
        q->insetChange(getInset(), oldInset);
    }
}

void QQuickControlPrivate::setRightInset(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldInset = getInset();
    extra.value().rightInset = value;
    extra.value().hasRightInset = !reset;
    if (!qFuzzyCompare(oldInset.right(), value)) {
        emit q->rightInsetChanged();
        q->insetChange(getInset(), oldInset);
    }
}

void QQuickControlPrivate::setBottomInset(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    const QMarginsF oldInset = getInset();
    extra.value().bottomInset = value;
    extra.value().hasBottomInset = !reset;
    if (!qFuzzyCompare(oldInset.bottom(), value)) {
        emit q->bottomInsetChanged();
        q->insetChange(getInset(), oldInset);
    }
}

void QQuickControlPrivate::resizeBackground()
{
    if (!background)
        return;

    resizingBackground = true;

    QQuickItemPrivate *p = QQuickItemPrivate::get(background);
    bool changeWidth = false;
    bool changeHeight = false;
    if (((!p->widthValid() || !extra.isAllocated() || !extra->hasBackgroundWidth) && qFuzzyIsNull(background->x()))
            || (extra.isAllocated() && (extra->hasLeftInset || extra->hasRightInset))) {
        const auto leftInset = getLeftInset();
        if (!qt_is_nan(leftInset) && p->x.valueBypassingBindings() != leftInset) {
            // We bypass the binding here to prevent it from being removed
            p->x.setValueBypassingBindings(leftInset);
            p->dirty(DirtyType::Position);
        }
        changeWidth = !p->width.hasBinding();
    }
    if (((!p->heightValid() || !extra.isAllocated() || !extra->hasBackgroundHeight) && qFuzzyIsNull(background->y()))
            || (extra.isAllocated() && (extra->hasTopInset || extra->hasBottomInset))) {
        const auto topInset = getTopInset();
        if (!qt_is_nan(topInset) && p->y.valueBypassingBindings() != topInset) {
            // We bypass the binding here to prevent it from being removed
            p->y.setValueBypassingBindings(topInset);
            p->dirty(DirtyType::Position);
        }
        changeHeight = !p->height.hasBinding();
    }
    if (changeHeight || changeWidth) {
        auto newWidth = changeWidth ?
            width.valueBypassingBindings() - getLeftInset() - getRightInset() :
            p->width.valueBypassingBindings();
        auto newHeight = changeHeight ?
            height.valueBypassingBindings() - getTopInset() - getBottomInset() :
            p->height.valueBypassingBindings();
        background->setSize({newWidth, newHeight});
    }

    resizingBackground = false;
}

void QQuickControlPrivate::resizeContent()
{
    Q_Q(QQuickControl);
    if (contentItem) {
        contentItem->setPosition(QPointF(q->leftPadding(), q->topPadding()));
        contentItem->setSize(QSizeF(q->availableWidth(), q->availableHeight()));
    }
}

QQuickItem *QQuickControlPrivate::getContentItem()
{
    if (!contentItem)
        executeContentItem();
    return contentItem;
}

void QQuickControlPrivate::setContentItem_helper(QQuickItem *item, bool notify)
{
    Q_Q(QQuickControl);
    if (contentItem == item)
        return;

    if (notify)
        warnIfCustomizationNotSupported(q, item, QStringLiteral("contentItem"));

    if (!contentItem.isExecuting())
        cancelContentItem();

    QQuickItem *oldContentItem = contentItem;
    if (oldContentItem) {
        disconnect(oldContentItem, &QQuickItem::baselineOffsetChanged, this, &QQuickControlPrivate::updateBaselineOffset);
        QQuickItemPrivate::get(oldContentItem)->removeItemChangeListener(this, QQuickControlPrivate::Focus);
        removeImplicitSizeListener(oldContentItem);
    }

    contentItem = item;
    q->contentItemChange(item, oldContentItem);
    QQuickControlPrivate::hideOldItem(oldContentItem);

    if (item) {
        connect(contentItem.data(), &QQuickItem::baselineOffsetChanged, this, &QQuickControlPrivate::updateBaselineOffset);
        // We need to update the control's focusReason when the contentItem receives or loses focus. Since focusPolicy
        // (or other properties impacting focus handling, like QQuickItem::activeFocusOnTab) might change later, and
        // since the content item might also change focus programmatically, we always have to listen for those events.
        QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickControlPrivate::Focus);
        if (!item->parentItem())
            item->setParentItem(q);
        if (componentComplete)
            resizeContent();
        addImplicitSizeListener(contentItem);
    }

    updateImplicitContentSize();
    updateBaselineOffset();

    if (notify && !contentItem.isExecuting())
        emit q->contentItemChanged();
}

qreal QQuickControlPrivate::getContentWidth() const
{
    return contentItem ? contentItem->implicitWidth() : 0;
}

qreal QQuickControlPrivate::getContentHeight() const
{
    return contentItem ? contentItem->implicitHeight() : 0;
}

void QQuickControlPrivate::updateImplicitContentWidth()
{
    Q_Q(QQuickControl);
    const qreal oldWidth = implicitContentWidth;
    implicitContentWidth = getContentWidth();
    if (!qFuzzyCompare(implicitContentWidth, oldWidth))
        emit q->implicitContentWidthChanged();
}

void QQuickControlPrivate::updateImplicitContentHeight()
{
    Q_Q(QQuickControl);
    const qreal oldHeight = implicitContentHeight;
    implicitContentHeight = getContentHeight();
    if (!qFuzzyCompare(implicitContentHeight, oldHeight))
        emit q->implicitContentHeightChanged();
}

void QQuickControlPrivate::updateImplicitContentSize()
{
    Q_Q(QQuickControl);
    const qreal oldWidth = implicitContentWidth;
    const qreal oldHeight = implicitContentHeight;
    implicitContentWidth = getContentWidth();
    implicitContentHeight = getContentHeight();
    if (!qFuzzyCompare(implicitContentWidth, oldWidth))
        emit q->implicitContentWidthChanged();
    if (!qFuzzyCompare(implicitContentHeight, oldHeight))
        emit q->implicitContentHeightChanged();
}

QPalette QQuickControlPrivate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::System);
}

#if QT_CONFIG(accessibility)
void QQuickControlPrivate::accessibilityActiveChanged(bool active)
{
    Q_Q(QQuickControl);
    return q->accessibilityActiveChanged(active);
}

QAccessible::Role QQuickControlPrivate::accessibleRole() const
{
    Q_Q(const QQuickControl);
    return q->accessibleRole();
}

QQuickAccessibleAttached *QQuickControlPrivate::accessibleAttached(const QObject *object)
{
    if (!QAccessible::isActive())
        return nullptr;
    return QQuickAccessibleAttached::attachedProperties(object);
}
#endif

/*!
    \internal

    Returns the font that the control \a item inherits from its ancestors and
    QGuiApplication::font.
*/
QFont QQuickControlPrivate::parentFont(const QQuickItem *item)
{
    QQuickItem *p = item->parentItem();
    while (p) {
        if (QQuickControl *control = qobject_cast<QQuickControl *>(p))
            return QQuickControlPrivate::get(control)->resolvedFont;
        else if (QQuickLabel *label = qobject_cast<QQuickLabel *>(p))
            return label->QQuickText::font();
        else if (QQuickTextField *textField = qobject_cast<QQuickTextField *>(p))
            return textField->QQuickTextInput::font();
        else if (QQuickTextArea *textArea = qobject_cast<QQuickTextArea *>(p))
            return textArea->QQuickTextEdit::font();

        p = p->parentItem();
    }

    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(item->window()))
        return window->font();

    return QQuickTheme::font(QQuickTheme::System);
}

/*!
    \internal

    Determine which font is implicitly imposed on this control by its ancestors
    and QGuiApplication::font, resolve this against its own font (attributes from
    the implicit font are copied over). Then propagate this font to this
    control's children.
*/
void QQuickControlPrivate::resolveFont()
{
    Q_Q(QQuickControl);
    inheritFont(parentFont(q));
}

void QQuickControlPrivate::inheritFont(const QFont &font)
{
    Q_Q(QQuickControl);
    QFont parentFont = extra.isAllocated() ? extra->requestedFont.resolve(font) : font;
    parentFont.setResolveMask(extra.isAllocated() ? extra->requestedFont.resolveMask() | font.resolveMask() : font.resolveMask());

    const QFont defaultFont = q->defaultFont();
    QFont resolvedFont = parentFont.resolve(defaultFont);

    setFont_helper(resolvedFont);
}

/*!
    \internal

    Assign \a font to this control, and propagate it to all children.
*/
void QQuickControlPrivate::updateFont(const QFont &font)
{
    Q_Q(QQuickControl);
    QFont oldFont = resolvedFont;
    resolvedFont = font;

    if (oldFont != font)
        q->fontChange(font, oldFont);

    QQuickControlPrivate::updateFontRecur(q, font);

    if (oldFont != font)
        emit q->fontChanged();
}

void QQuickControlPrivate::updateFontRecur(QQuickItem *item, const QFont &font)
{
    const auto childItems = item->childItems();
    for (QQuickItem *child : childItems) {
        if (QQuickControl *control = qobject_cast<QQuickControl *>(child))
            QQuickControlPrivate::get(control)->inheritFont(font);
        else if (QQuickLabel *label = qobject_cast<QQuickLabel *>(child))
            QQuickLabelPrivate::get(label)->inheritFont(font);
        else if (QQuickTextArea *textArea = qobject_cast<QQuickTextArea *>(child))
            QQuickTextAreaPrivate::get(textArea)->inheritFont(font);
        else if (QQuickTextField *textField = qobject_cast<QQuickTextField *>(child))
            QQuickTextFieldPrivate::get(textField)->inheritFont(font);
        else
            QQuickControlPrivate::updateFontRecur(child, font);
    }
}

QLocale QQuickControlPrivate::calcLocale(const QQuickItem *item)
{
    for (const QQuickItem *p = item; p; p = p->parentItem())
        if (const QQuickControl *control = qobject_cast<const QQuickControl *>(p))
            return control->locale();

    if (item)
        if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(item->window()))
            return window->locale();

    return QLocale();
}

/*!
    \internal

    Warns if \a control has a \c __notCustomizable property which is set to \c true,
    unless \a item has an \c __ignoreNotCustomizable property.

    If \c __notCustomizable is \c true, it means that the style that provides the
    control does not support customization.  If \c __ignoreNotCustomizable is true,
    it means that the item is an internal implementation detail and shouldn't be
    subject to the warning.

    We take a QObject for \c control instead of QQuickControl or QQuickItem
    because not all relevant types derive from QQuickControl - e.g. TextField,
    TextArea, QQuickIndicatorButton, etc.
*/
void QQuickControlPrivate::warnIfCustomizationNotSupported(QObject *control, QQuickItem *item, const QString &propertyName)
{
    static const bool ignoreWarnings = [](){
        return qEnvironmentVariableIntValue("QT_QUICK_CONTROLS_IGNORE_CUSTOMIZATION_WARNINGS");
    }();
    if (ignoreWarnings)
        return;

    if (!control->property("__notCustomizable").toBool()
            || (item && item->property("__ignoreNotCustomizable").toBool()))
        return;

    qmlWarning(item ? item : control).nospace() << "The current style does not support customization of this control "
        << "(property: " << propertyName << " item: " << item << "). "
        "Please customize a non-native style (such as Basic, Fusion, Material, etc). For more information, see: "
        "https://doc.qt.io/qt-6/qtquickcontrols2-customize.html#customization-reference";
}

void QQuickControlPrivate::updateLocale(const QLocale &l, bool e)
{
    Q_Q(QQuickControl);
    if (!e && hasLocale)
        return;

    QLocale old = q->locale();
    hasLocale = e;
    if (old != l) {
        locale = l;
        q->localeChange(l, old);
        QQuickControlPrivate::updateLocaleRecur(q, l);
        emit q->localeChanged();
    }
}

void QQuickControlPrivate::updateLocaleRecur(QQuickItem *item, const QLocale &l)
{
    const auto childItems = item->childItems();
    for (QQuickItem *child : childItems) {
        if (QQuickControl *control = qobject_cast<QQuickControl *>(child))
            QQuickControlPrivate::get(control)->updateLocale(l, false);
        else
            updateLocaleRecur(child, l);
    }
}

#if QT_CONFIG(quicktemplates2_hover)
void QQuickControlPrivate::updateHoverEnabled(bool enabled, bool xplicit)
{
    Q_Q(QQuickControl);
    if (!xplicit && explicitHoverEnabled)
        return;

    bool wasEnabled = q->isHoverEnabled();
    explicitHoverEnabled = xplicit;
    if (wasEnabled != enabled) {
        q->setAcceptHoverEvents(enabled);
        QQuickControlPrivate::updateHoverEnabledRecur(q, enabled);
        emit q->hoverEnabledChanged();
    }
}

void QQuickControlPrivate::updateHoverEnabledRecur(QQuickItem *item, bool enabled)
{
    const auto childItems = item->childItems();
    for (QQuickItem *child : childItems) {
        if (QQuickControl *control = qobject_cast<QQuickControl *>(child))
            QQuickControlPrivate::get(control)->updateHoverEnabled(enabled, false);
        else
            updateHoverEnabledRecur(child, enabled);
    }
}

bool QQuickControlPrivate::calcHoverEnabled(const QQuickItem *item)
{
    const QQuickItem *p = item;
    while (p) {
        // QQuickPopupItem accepts hover events to avoid leaking them through.
        // Don't inherit that to the children of the popup, but fallback to the
        // environment variable or style hint.
        if (qobject_cast<const QQuickPopupItem *>(p))
            break;

        auto *applicationWindow = qobject_cast<QQuickApplicationWindow *>(p->window());
        if (applicationWindow) {
            const auto *applicationWindowPrivate = QQuickApplicationWindowPrivate::get(applicationWindow);
            if (p == applicationWindowPrivate->control) {
                // Don't let the next check get hit, because it will return
                // false since it's a plain QQuickControl. Instead, skip to the
                // global flags.
                break;
            }
        }

        if (QQuickTemplatesUtils::isInteractiveControlType(p)) {
            const QVariant hoverEnabledProperty = p->property("hoverEnabled");
            Q_ASSERT(hoverEnabledProperty.isValid());
            Q_ASSERT(hoverEnabledProperty.userType() == QMetaType::Bool);
            return hoverEnabledProperty.toBool();
        }

        p = p->parentItem();
    }

    bool ok = false;
    int env = qEnvironmentVariableIntValue("QT_QUICK_CONTROLS_HOVER_ENABLED", &ok);
    if (ok)
        return env != 0;

    // TODO: QQuickApplicationWindow::isHoverEnabled()

    return QGuiApplication::styleHints()->useHoverEffects();
}
#endif

static inline QString contentItemName() { return QStringLiteral("contentItem"); }

void QQuickControlPrivate::cancelContentItem()
{
    Q_Q(QQuickControl);
    quickCancelDeferred(q, contentItemName());
}

void QQuickControlPrivate::executeContentItem(bool complete)
{
    Q_Q(QQuickControl);
    if (contentItem.wasExecuted())
        return;

    if (!contentItem || complete)
        quickBeginDeferred(q, contentItemName(), contentItem);
    if (complete)
        quickCompleteDeferred(q, contentItemName(), contentItem);
}

void QQuickControlPrivate::cancelBackground()
{
    Q_Q(QQuickControl);
    quickCancelDeferred(q, backgroundName());
}

void QQuickControlPrivate::executeBackground(bool complete)
{
    Q_Q(QQuickControl);
    if (background.wasExecuted())
        return;

    if (!background || complete)
        quickBeginDeferred(q, backgroundName(), background);
    if (complete)
        quickCompleteDeferred(q, backgroundName(), background);
}

/*
    \internal

    Hides an item that was replaced by a newer one, rather than
    deleting it, as the item is typically created in QML and hence
    we don't own it.
*/
void QQuickControlPrivate::hideOldItem(QQuickItem *item)
{
    if (!item)
        return;

    qCDebug(lcItemManagement) << "hiding old item" << item;

    item->setVisible(false);
    item->setParentItem(nullptr);

#if QT_CONFIG(accessibility)
    // Remove the item from the accessibility tree.
    QQuickAccessibleAttached *accessible = accessibleAttached(item);
    if (accessible)
        accessible->setIgnored(true);
#endif
}

/*
    \internal

    Named "unhide" because it's used for cases where an item
    that was previously hidden by \l hideOldItem() wants to be
    shown by a control again, such as a ScrollBar in ScrollView.

    \a visibility controls the visibility of \a item, as there
    may have been bindings that controlled visibility, such as
    with a typical ScrollBar.qml implementation:

    \code
        visible: control.policy !== T.ScrollBar.AlwaysOff
    \endcode

    In the future we could try to save the binding for the visible
    property (using e.g. QQmlAnyBinding::takeFrom), but for now we
    keep it simple and just allow restoring an equivalent literal value.
*/
void QQuickControlPrivate::unhideOldItem(QQuickControl *control, QQuickItem *item, UnhideVisibility visibility)
{
    Q_ASSERT(item);
    qCDebug(lcItemManagement) << "unhiding old item" << item;

    item->setVisible(visibility == UnhideVisibility::Show);
    item->setParentItem(control);

#if QT_CONFIG(accessibility)
    // Add the item back in to the accessibility tree.
    QQuickAccessibleAttached *accessible = accessibleAttached(item);
    if (accessible)
        accessible->setIgnored(false);
#endif
}

void QQuickControlPrivate::updateBaselineOffset()
{
    Q_Q(QQuickControl);
    if (extra.isAllocated() && extra.value().hasBaselineOffset)
        return;

    if (!contentItem)
        q->QQuickItem::setBaselineOffset(0);
    else
        q->QQuickItem::setBaselineOffset(getTopPadding() + contentItem->baselineOffset());
}

void QQuickControlPrivate::addImplicitSizeListener(QQuickItem *item, ChangeTypes changes)
{
    addImplicitSizeListener(item, this, changes);
}

void QQuickControlPrivate::removeImplicitSizeListener(QQuickItem *item, ChangeTypes changes)
{
    removeImplicitSizeListener(item, this, changes);
}

void QQuickControlPrivate::addImplicitSizeListener(QQuickItem *item, QQuickItemChangeListener *listener, ChangeTypes changes)
{
    if (!item || !listener)
        return;
    QQuickItemPrivate::get(item)->addItemChangeListener(listener, changes);
}

void QQuickControlPrivate::removeImplicitSizeListener(QQuickItem *item, QQuickItemChangeListener *listener, ChangeTypes changes)
{
    if (!item || !listener)
        return;
    QQuickItemPrivate::get(item)->removeItemChangeListener(listener, changes);
}

void QQuickControlPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_Q(QQuickControl);
    if (item == background)
        emit q->implicitBackgroundWidthChanged();
    else if (item == contentItem)
        updateImplicitContentWidth();
}

void QQuickControlPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_Q(QQuickControl);
    if (item == background)
        emit q->implicitBackgroundHeightChanged();
    else if (item == contentItem)
        updateImplicitContentHeight();
}

void QQuickControlPrivate::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff)
{
    Q_UNUSED(diff);
    if (resizingBackground || item != background || !change.sizeChange())
        return;

    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    // Only set hasBackgroundWidth/Height if it was a width/height change,
    // otherwise we're prevented from setting a width/height in the future.
    if (change.widthChange())
        extra.value().hasBackgroundWidth = p->widthValid();
    if (change.heightChange())
        extra.value().hasBackgroundHeight = p->heightValid();
    resizeBackground();
}

void QQuickControlPrivate::itemDestroyed(QQuickItem *item)
{
    Q_Q(QQuickControl);
    if (item == background) {
        background = nullptr;
        emit q->implicitBackgroundWidthChanged();
        emit q->implicitBackgroundHeightChanged();
    } else if (item == contentItem) {
        contentItem = nullptr;
        updateImplicitContentSize();
    }
}

void QQuickControlPrivate::itemFocusChanged(QQuickItem *item, Qt::FocusReason reason)
{
    Q_Q(QQuickControl);
    if (item == contentItem || item == q)
        setLastFocusChangeReason(reason);
}

bool QQuickControlPrivate::setLastFocusChangeReason(Qt::FocusReason reason)
{
    Q_Q(QQuickControl);
    Qt::FocusReason oldReason = static_cast<Qt::FocusReason>(focusReason);
    const auto focusReasonChanged = QQuickItemPrivate::setLastFocusChangeReason(reason);
    if (focusReasonChanged)
        emit q->focusReasonChanged();
    if (isKeyFocusReason(oldReason) != isKeyFocusReason(reason))
        emit q->visualFocusChanged();

    return focusReasonChanged;
}

QQuickControl::QQuickControl(QQuickItem *parent)
    : QQuickItem(*(new QQuickControlPrivate), parent)
{
    Q_D(QQuickControl);
    d->init();
}

QQuickControl::QQuickControl(QQuickControlPrivate &dd, QQuickItem *parent)
    : QQuickItem(dd, parent)
{
    Q_D(QQuickControl);
    d->init();
}

QQuickControl::~QQuickControl()
{
    Q_D(QQuickControl);
    d->removeImplicitSizeListener(d->background, QQuickControlPrivate::ImplicitSizeChanges | QQuickItemPrivate::Geometry);
    d->removeImplicitSizeListener(d->contentItem);
    if (d->contentItem)
        QQuickItemPrivate::get(d->contentItem)->removeItemChangeListener(d, QQuickItemPrivate::Focus);
#if QT_CONFIG(accessibility)
    QAccessible::removeActivationObserver(d);
#endif
}

void QQuickControl::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_D(QQuickControl);
    QQuickItem::itemChange(change, value);
    switch (change) {
    case ItemEnabledHasChanged:
        enabledChange();
        break;
    case ItemVisibleHasChanged:
#if QT_CONFIG(quicktemplates2_hover)
        if (!value.boolValue)
            setHovered(false);
#endif
        break;
    case ItemSceneChange:
    case ItemParentHasChanged:
        if ((change == ItemParentHasChanged && value.item) || (change == ItemSceneChange && value.window)) {
            d->resolveFont();
            if (!d->hasLocale)
                d->updateLocale(QQuickControlPrivate::calcLocale(d->parentItem), false); // explicit=false
#if QT_CONFIG(quicktemplates2_hover)
            if (!d->explicitHoverEnabled)
                d->updateHoverEnabled(QQuickControlPrivate::calcHoverEnabled(d->parentItem), false); // explicit=false
#endif
        }
        break;
    case ItemActiveFocusHasChanged:
        if (isKeyFocusReason(static_cast<Qt::FocusReason>(d->focusReason)))
            emit visualFocusChanged();
        break;
    default:
        break;
    }
}

/*!
    \qmlproperty font QtQuick.Controls::Control::font

    This property holds the font currently set for the control.

    This property describes the control's requested font. The font is used by the control's
    style when rendering standard components, and is available as a means to ensure that custom
    controls can maintain consistency with the native platform's native look and feel. It's common
    that different platforms, or different styles, define different fonts for an application.

    The default font depends on the system environment. ApplicationWindow maintains a system/theme
    font which serves as a default for all controls. There may also be special font defaults for
    certain types of controls. You can also set the default font for controls by either:

    \list
    \li passing a custom font to QGuiApplication::setFont(), before loading the QML; or
    \li specifying the fonts in the \l {Qt Quick Controls 2 Configuration File}{qtquickcontrols2.conf file}.
    \endlist

    Finally, the font is matched against Qt's font database to find the best match.

    Control propagates explicit font properties from parent to children. If you change a specific
    property on a control's font, that property propagates to all of the control's children,
    overriding any system defaults for that property.

    \code
    Page {
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

    For the full list of available font properties, see the
    \l [QtQuick]{font}{font QML Value Type} documentation.
*/
QFont QQuickControl::font() const
{
    Q_D(const QQuickControl);
    QFont font = d->resolvedFont;
    // The resolveMask should inherit from the requestedFont
    font.setResolveMask(d->extra.value().requestedFont.resolveMask());
    return font;
}

void QQuickControl::setFont(const QFont &font)
{
    Q_D(QQuickControl);
    if (d->extra.value().requestedFont.resolveMask() == font.resolveMask() && d->extra.value().requestedFont == font)
        return;

    d->extra.value().requestedFont = font;
    d->resolveFont();
}

void QQuickControl::resetFont()
{
    setFont(QFont());
}

/*!
    \qmlproperty real QtQuick.Controls::Control::availableWidth
    \readonly

    This property holds the width available to the \l contentItem after
    deducting horizontal padding from the \l {Item::}{width} of the control.

    \sa {Control Layout}, padding, leftPadding, rightPadding
*/
qreal QQuickControl::availableWidth() const
{
    return qMax<qreal>(0.0, width() - leftPadding() - rightPadding());
}

/*!
    \qmlproperty real QtQuick.Controls::Control::availableHeight
    \readonly

    This property holds the height available to the \l contentItem after
    deducting vertical padding from the \l {Item::}{height} of the control.

    \sa {Control Layout}, padding, topPadding, bottomPadding
*/
qreal QQuickControl::availableHeight() const
{
    return qMax<qreal>(0.0, height() - topPadding() - bottomPadding());
}

/*!
    \qmlproperty real QtQuick.Controls::Control::padding

    This property holds the default padding.

    Padding adds a space between each edge of the content item and the
    background item, effectively controlling the size of the content item. To
    specify a padding value for a specific edge of the control, set its
    relevant property:

    \list
    \li \l {Control::}{leftPadding}
    \li \l {Control::}{rightPadding}
    \li \l {Control::}{topPadding}
    \li \l {Control::}{bottomPadding}
    \endlist

    \note Different styles may specify the default padding for certain controls
    in different ways, and these ways may change over time as the design
    guidelines that the style is based on evolve. To ensure that these changes
    don't affect the padding values you have specified, it is best to use the
    most specific properties available. For example, rather than setting
    the \l padding property:

    \code
    padding: 0
    \endcode

    set each specific property instead:

    \code
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    \endcode

    \sa {Control Layout}, availableWidth, availableHeight, topPadding, leftPadding, rightPadding, bottomPadding
*/
qreal QQuickControl::padding() const
{
    Q_D(const QQuickControl);
    return d->padding;
}

void QQuickControl::setPadding(qreal padding)
{
    Q_D(QQuickControl);
    if (qFuzzyCompare(d->padding, padding))
        return;

    const QMarginsF oldPadding = d->getPadding();
    const qreal oldVerticalPadding = d->getVerticalPadding();
    const qreal oldHorizontalPadding = d->getHorizontalPadding();

    d->padding = padding;
    emit paddingChanged();

    const QMarginsF newPadding = d->getPadding();
    const qreal newVerticalPadding = d->getVerticalPadding();
    const qreal newHorizontalPadding = d->getHorizontalPadding();

    if (!qFuzzyCompare(newPadding.top(), oldPadding.top()))
        emit topPaddingChanged();
    if (!qFuzzyCompare(newPadding.left(), oldPadding.left()))
        emit leftPaddingChanged();
    if (!qFuzzyCompare(newPadding.right(), oldPadding.right()))
        emit rightPaddingChanged();
    if (!qFuzzyCompare(newPadding.bottom(), oldPadding.bottom()))
        emit bottomPaddingChanged();
    if (!qFuzzyCompare(newVerticalPadding, oldVerticalPadding))
        emit verticalPaddingChanged();
    if (!qFuzzyCompare(newHorizontalPadding, oldHorizontalPadding))
        emit horizontalPaddingChanged();
    if (!qFuzzyCompare(newPadding.top(), oldPadding.top()) || !qFuzzyCompare(newPadding.bottom(), oldPadding.bottom()))
        emit availableHeightChanged();
    if (!qFuzzyCompare(newPadding.left(), oldPadding.left()) || !qFuzzyCompare(newPadding.right(), oldPadding.right()))
        emit availableWidthChanged();

    paddingChange(newPadding, oldPadding);
}

void QQuickControl::resetPadding()
{
    setPadding(0);
}

/*!
    \qmlproperty real QtQuick.Controls::Control::topPadding

    This property holds the top padding. Unless explicitly set, the value
    is equal to \c verticalPadding.

    \sa {Control Layout}, padding, bottomPadding, verticalPadding, availableHeight
*/
qreal QQuickControl::topPadding() const
{
    Q_D(const QQuickControl);
    return d->getTopPadding();
}

void QQuickControl::setTopPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setTopPadding(padding);
}

void QQuickControl::resetTopPadding()
{
    Q_D(QQuickControl);
    d->setTopPadding(0, true);
}

/*!
    \qmlproperty real QtQuick.Controls::Control::leftPadding

    This property holds the left padding. Unless explicitly set, the value
    is equal to \c horizontalPadding.

    \sa {Control Layout}, padding, rightPadding, horizontalPadding, availableWidth
*/
qreal QQuickControl::leftPadding() const
{
    Q_D(const QQuickControl);
    return d->getLeftPadding();
}

void QQuickControl::setLeftPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setLeftPadding(padding);
}

void QQuickControl::resetLeftPadding()
{
    Q_D(QQuickControl);
    d->setLeftPadding(0, true);
}

/*!
    \qmlproperty real QtQuick.Controls::Control::rightPadding

    This property holds the right padding. Unless explicitly set, the value
    is equal to \c horizontalPadding.

    \sa {Control Layout}, padding, leftPadding, horizontalPadding, availableWidth
*/
qreal QQuickControl::rightPadding() const
{
    Q_D(const QQuickControl);
    return d->getRightPadding();
}

void QQuickControl::setRightPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setRightPadding(padding);
}

void QQuickControl::resetRightPadding()
{
    Q_D(QQuickControl);
    d->setRightPadding(0, true);
}

/*!
    \qmlproperty real QtQuick.Controls::Control::bottomPadding

    This property holds the bottom padding. Unless explicitly set, the value
    is equal to \c verticalPadding.

    \sa {Control Layout}, padding, topPadding, verticalPadding, availableHeight
*/
qreal QQuickControl::bottomPadding() const
{
    Q_D(const QQuickControl);
    return d->getBottomPadding();
}

void QQuickControl::setBottomPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setBottomPadding(padding);
}

void QQuickControl::resetBottomPadding()
{
    Q_D(QQuickControl);
    d->setBottomPadding(0, true);
}

/*!
    \qmlproperty real QtQuick.Controls::Control::spacing

    This property holds the spacing.

    Spacing is useful for controls that have multiple or repetitive building
    blocks. For example, some styles use spacing to determine the distance
    between the text and indicator of \l CheckBox. Spacing is not enforced by
    Control, so each style may interpret it differently, and some may ignore it
    altogether.
*/
qreal QQuickControl::spacing() const
{
    Q_D(const QQuickControl);
    return d->spacing;
}

void QQuickControl::setSpacing(qreal spacing)
{
    Q_D(QQuickControl);
    if (qFuzzyCompare(d->spacing, spacing))
        return;

    qreal oldSpacing = d->spacing;
    d->spacing = spacing;
    emit spacingChanged();
    spacingChange(spacing, oldSpacing);
}

void QQuickControl::resetSpacing()
{
    setSpacing(0);
}

/*!
    \qmlproperty Locale QtQuick.Controls::Control::locale

    This property holds the locale of the control.

    It contains locale specific properties for formatting data and numbers.
    Unless a special locale has been set, this is either the parent's locale
    or the default locale.

    Control propagates the locale from parent to children. If you change the
    control's locale, that locale propagates to all of the control's children,
    overriding the system default locale.

    \sa mirrored
*/
QLocale QQuickControl::locale() const
{
    Q_D(const QQuickControl);
    return d->locale;
}

void QQuickControl::setLocale(const QLocale &locale)
{
    Q_D(QQuickControl);
    if (d->hasLocale && d->locale == locale)
        return;

    d->updateLocale(locale, true); // explicit=true
}

void QQuickControl::resetLocale()
{
    Q_D(QQuickControl);
    if (!d->hasLocale)
        return;

    d->hasLocale = false;
    d->updateLocale(QQuickControlPrivate::calcLocale(d->parentItem), false); // explicit=false
}

/*!
    \qmlproperty bool QtQuick.Controls::Control::mirrored
    \readonly

    This property holds whether the control is mirrored.

    This property is provided for convenience. A control is considered mirrored
    when its visual layout direction is right-to-left; that is, when
    \l {LayoutMirroring::enabled}{LayoutMirroring.enabled} is \c true.

    As of Qt 6.2, the \l locale property no longer affects this property.

    \sa {LayoutMirroring}{LayoutMirroring}, {Right-to-left User Interfaces}
*/
bool QQuickControl::isMirrored() const
{
    Q_D(const QQuickControl);
    return d->isMirrored();
}

// ### Qt 7: replace with signal parameter: QTBUG-117596
/*!
    \qmlproperty enumeration QtQuick.Controls::Control::focusReason

    This property holds the reason of the last focus change.

    The value of this property is modified by Qt whenever focus is transferred,
    and you should never have to set this property yourself.

    \note This property does not indicate whether the item has \l {Item::activeFocus}
        {active focus}, but the reason why the item either gained or lost focus.

    \value Qt.MouseFocusReason         A mouse action occurred.
    \value Qt.TabFocusReason           The Tab key was pressed.
    \value Qt.BacktabFocusReason       A Backtab occurred. The input for this may include the Shift or Control keys; e.g. Shift+Tab.
    \value Qt.ActiveWindowFocusReason  The window system made this window either active or inactive.
    \value Qt.PopupFocusReason         The application opened/closed a pop-up that grabbed/released the keyboard focus.
    \value Qt.ShortcutFocusReason      The user typed a label's buddy shortcut
    \value Qt.MenuBarFocusReason       The menu bar took focus.
    \value Qt.OtherFocusReason         Another reason, usually application-specific.

    \sa Item::activeFocus

    \sa visualFocus
*/
Qt::FocusReason QQuickControl::focusReason() const
{
    Q_D(const QQuickControl);
    return d->lastFocusChangeReason();
}

void QQuickControl::setFocusReason(Qt::FocusReason reason)
{
    Q_D(QQuickControl);
    d->setLastFocusChangeReason(reason);
}

/*!
    \qmlproperty bool QtQuick.Controls::Control::visualFocus
    \readonly

    This property holds whether the control has visual focus. This property
    is \c true when the control has active focus and the focus reason is either
    \c Qt.TabFocusReason, \c Qt.BacktabFocusReason, or \c Qt.ShortcutFocusReason.

    In general, for visualizing key focus, this property is preferred over
    \l Item::activeFocus. This ensures that key focus is only visualized when
    interacting with keys - not when interacting via touch or mouse.

    \sa focusReason, Item::activeFocus
*/
bool QQuickControl::hasVisualFocus() const
{
    Q_D(const QQuickControl);
    return d->activeFocus && isKeyFocusReason(static_cast<Qt::FocusReason>(d->focusReason));
}

/*!
    \qmlproperty bool QtQuick.Controls::Control::hovered
    \readonly

    This property holds whether the control is hovered.

    \sa hoverEnabled
*/
bool QQuickControl::isHovered() const
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(const QQuickControl);
    return d->hovered;
#else
    return false;
#endif
}

void QQuickControl::setHovered(bool hovered)
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(QQuickControl);
    if (hovered == d->hovered)
        return;

    d->hovered = hovered;
    emit hoveredChanged();
    hoverChange();
#else
    Q_UNUSED(hovered);
#endif
}

/*!
    \qmlproperty bool QtQuick.Controls::Control::hoverEnabled

    This property determines whether the control accepts hover events. The default value
    is \c Application.styleHints.useHoverEffects.

    Setting this property propagates the value to all child controls that do not have
    \c hoverEnabled explicitly set.

    You can also enable or disable hover effects for all Qt Quick Controls applications
    by setting the \c QT_QUICK_CONTROLS_HOVER_ENABLED \l {Supported Environment Variables
    in Qt Quick Controls}{environment variable}.

    \sa hovered
*/
bool QQuickControl::isHoverEnabled() const
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(const QQuickControl);
    return d->hoverEnabled;
#else
    return false;
#endif
}

void QQuickControl::setHoverEnabled(bool enabled)
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(QQuickControl);
    if (d->explicitHoverEnabled && enabled == d->hoverEnabled)
        return;

    d->updateHoverEnabled(enabled, true); // explicit=true
#else
    Q_UNUSED(enabled);
#endif
}

void QQuickControl::resetHoverEnabled()
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(QQuickControl);
    if (!d->explicitHoverEnabled)
        return;

    d->explicitHoverEnabled = false;
    d->updateHoverEnabled(QQuickControlPrivate::calcHoverEnabled(d->parentItem), false); // explicit=false
#endif
}

/*!
    \qmlproperty bool QtQuick.Controls::Control::wheelEnabled

    This property determines whether the control handles wheel events. The default value is \c false.

    \note Care must be taken when enabling wheel events for controls within scrollable items such
    as \l Flickable, as the control will consume the events and hence interrupt scrolling of the
    Flickable.
*/
bool QQuickControl::isWheelEnabled() const
{
    Q_D(const QQuickControl);
    return d->wheelEnabled;
}

void QQuickControl::setWheelEnabled(bool enabled)
{
    Q_D(QQuickControl);
    if (d->wheelEnabled == enabled)
        return;

    d->wheelEnabled = enabled;
    emit wheelEnabledChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::Control::background

    This property holds the background item.

    \code
    Button {
        id: control
        text: qsTr("Button")
        background: Rectangle {
            implicitWidth: 100
            implicitHeight: 40
            opacity: enabled ? 1 : 0.3
            color: control.down ? "#d0d0d0" : "#e0e0e0"
        }
    }
    \endcode

    \input qquickcontrol-background.qdocinc notes

    \sa {Control Layout}
*/
QQuickItem *QQuickControl::background() const
{
    QQuickControlPrivate *d = const_cast<QQuickControlPrivate *>(d_func());
    if (!d->background)
        d->executeBackground();
    return d->background;
}

void QQuickControl::setBackground(QQuickItem *background)
{
    Q_D(QQuickControl);
    if (d->background == background)
        return;

    QQuickControlPrivate::warnIfCustomizationNotSupported(this, background, QStringLiteral("background"));

    if (!d->background.isExecuting())
        d->cancelBackground();

    const qreal oldImplicitBackgroundWidth = implicitBackgroundWidth();
    const qreal oldImplicitBackgroundHeight = implicitBackgroundHeight();

    if (d->extra.isAllocated()) {
        d->extra.value().hasBackgroundWidth = false;
        d->extra.value().hasBackgroundHeight = false;
    }

    d->removeImplicitSizeListener(d->background, QQuickControlPrivate::ImplicitSizeChanges | QQuickItemPrivate::Geometry);
    QQuickControlPrivate::hideOldItem(d->background);
    d->background = background;

    if (background) {
        background->setParentItem(this);
        if (qFuzzyIsNull(background->z()))
            background->setZ(-1);
        QQuickItemPrivate *p = QQuickItemPrivate::get(background);
        if (p->widthValid() || p->heightValid()) {
            d->extra.value().hasBackgroundWidth = p->widthValid();
            d->extra.value().hasBackgroundHeight = p->heightValid();
        }
        if (isComponentComplete())
            d->resizeBackground();
        d->addImplicitSizeListener(background, QQuickControlPrivate::ImplicitSizeChanges | QQuickItemPrivate::Geometry);
    }

    if (!qFuzzyCompare(oldImplicitBackgroundWidth, implicitBackgroundWidth()))
        emit implicitBackgroundWidthChanged();
    if (!qFuzzyCompare(oldImplicitBackgroundHeight, implicitBackgroundHeight()))
        emit implicitBackgroundHeightChanged();
    if (!d->background.isExecuting())
        emit backgroundChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::Control::contentItem

    This property holds the visual content item.

    \code
    Button {
        id: control
        text: qsTr("Button")
        contentItem: Label {
            text: control.text
            verticalAlignment: Text.AlignVCenter
        }
    }
    \endcode

    \note The content item is automatically positioned and resized to fit
    within the \l padding of the control. Bindings to the
    \l[QtQuick]{Item::}{x}, \l[QtQuick]{Item::}{y},
    \l[QtQuick]{Item::}{width}, and \l[QtQuick]{Item::}{height}
    properties of the contentItem are not respected.

    \note Most controls use the implicit size of the content item to calculate
    the implicit size of the control itself. If you replace the content item
    with a custom one, you should also consider providing a sensible implicit
    size for it (unless it is an item like \l Text which has its own implicit
    size).

    \sa {Control Layout}, padding
*/
QQuickItem *QQuickControl::contentItem() const
{
    QQuickControlPrivate *d = const_cast<QQuickControlPrivate *>(d_func());
    if (!d->contentItem)
        d->setContentItem_helper(d->getContentItem(), false);
    return d->contentItem;
}

void QQuickControl::setContentItem(QQuickItem *item)
{
    Q_D(QQuickControl);
    d->setContentItem_helper(item, true);
}

qreal QQuickControl::baselineOffset() const
{
    Q_D(const QQuickControl);
    return d->baselineOffset;
}

void QQuickControl::setBaselineOffset(qreal offset)
{
    Q_D(QQuickControl);
    d->extra.value().hasBaselineOffset = true;
    QQuickItem::setBaselineOffset(offset);
}

void QQuickControl::resetBaselineOffset()
{
    Q_D(QQuickControl);
    if (!d->extra.isAllocated() || !d->extra.value().hasBaselineOffset)
        return;

    if (d->extra.isAllocated())
        d->extra.value().hasBaselineOffset = false;
    d->updateBaselineOffset();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::horizontalPadding

    This property holds the horizontal padding. Unless explicitly set, the value
    is equal to \c padding.

    \sa {Control Layout}, padding, leftPadding, rightPadding, verticalPadding
*/
qreal QQuickControl::horizontalPadding() const
{
    Q_D(const QQuickControl);
    return d->getHorizontalPadding();
}

void QQuickControl::setHorizontalPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setHorizontalPadding(padding);
}

void QQuickControl::resetHorizontalPadding()
{
    Q_D(QQuickControl);
    d->setHorizontalPadding(0, true);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::verticalPadding

    This property holds the vertical padding. Unless explicitly set, the value
    is equal to \c padding.

    \sa {Control Layout}, padding, topPadding, bottomPadding, horizontalPadding
*/
qreal QQuickControl::verticalPadding() const
{
    Q_D(const QQuickControl);
    return d->getVerticalPadding();
}

void QQuickControl::setVerticalPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setVerticalPadding(padding);
}

void QQuickControl::resetVerticalPadding()
{
    Q_D(QQuickControl);
    d->setVerticalPadding(0, true);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::implicitContentWidth
    \readonly

    This property holds the implicit content width.

    For basic controls, the value is equal to \c {contentItem ? contentItem.implicitWidth : 0}.
    For types that inherit Container or Pane, the value is calculated based on the content children.

    This is typically used, together with \l implicitBackgroundWidth, to calculate
    the \l {Item::}{implicitWidth}:

    \code
    Control {
        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)
    }
    \endcode

    \sa implicitContentHeight, implicitBackgroundWidth
*/
qreal QQuickControl::implicitContentWidth() const
{
    Q_D(const QQuickControl);

    if (auto *safeArea = static_cast<QQuickSafeArea*>(
        qmlAttachedPropertiesObject<QQuickSafeArea>(this, false))) {
        // If the control's padding is tied to the safe area we may in
        // some cases end up with a binding loop if the implicit size
        // moves the control further into the non-safe area. Detect this
        // and break the binding loop by returning a constrained content
        // size based on an earlier known good implicit size.
        static constexpr auto kLastKnownGoodImplicitWidth = "_q_lastKnownGoodImplicitWidth";
        if (safeArea->detectedPossibleBindingLoop) {
            const auto lastImplicitWidth = safeArea->property(kLastKnownGoodImplicitWidth).value<int>();
            return lastImplicitWidth - leftPadding() - rightPadding();
        } else {
            safeArea->setProperty(kLastKnownGoodImplicitWidth, implicitWidth());
        }
    }

    return d->implicitContentWidth;
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::implicitContentHeight
    \readonly

    This property holds the implicit content height.

    For basic controls, the value is equal to \c {contentItem ? contentItem.implicitHeight : 0}.
    For types that inherit Container or Pane, the value is calculated based on the content children.

    This is typically used, together with \l implicitBackgroundHeight, to calculate
    the \l {Item::}{implicitHeight}:

    \code
    Control {
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 implicitContentHeight + topPadding + bottomPadding)
    }
    \endcode

    \sa implicitContentWidth, implicitBackgroundHeight
*/
qreal QQuickControl::implicitContentHeight() const
{
    Q_D(const QQuickControl);

    if (auto *safeArea = static_cast<QQuickSafeArea*>(
        qmlAttachedPropertiesObject<QQuickSafeArea>(this, false))) {
        // If the control's padding is tied to the safe area we may in
        // some cases end up with a binding loop if the implicit size
        // moves the control further into the non-safe area. Detect this
        // and break the binding loop by returning a constrained content
        // size based on an earlier known good implicit size.
        static constexpr auto kLastKnownGoodImplicitHeight = "_q_lastKnownGoodImplicitHeight";
        if (safeArea->detectedPossibleBindingLoop) {
            const auto lastImplicitHeight = safeArea->property(kLastKnownGoodImplicitHeight).value<int>();
            return lastImplicitHeight - topPadding() - bottomPadding();
        } else {
            safeArea->setProperty(kLastKnownGoodImplicitHeight, implicitHeight());
        }
    }

    return d->implicitContentHeight;
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::implicitBackgroundWidth
    \readonly

    This property holds the implicit background width.

    The value is equal to \c {background ? background.implicitWidth : 0}.

    This is typically used, together with \l implicitContentWidth, to calculate
    the \l {Item::}{implicitWidth}:

    \code
    Control {
        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)
    }
    \endcode

    \sa implicitBackgroundHeight, implicitContentWidth
*/
qreal QQuickControl::implicitBackgroundWidth() const
{
    Q_D(const QQuickControl);
    if (!d->background)
        return 0;
    return d->background->implicitWidth();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::implicitBackgroundHeight
    \readonly

    This property holds the implicit background height.

    The value is equal to \c {background ? background.implicitHeight : 0}.

    This is typically used, together with \l implicitContentHeight, to calculate
    the \l {Item::}{implicitHeight}:

    \code
    Control {
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 implicitContentHeight + topPadding + bottomPadding)
    }
    \endcode

    \sa implicitBackgroundWidth, implicitContentHeight
*/
qreal QQuickControl::implicitBackgroundHeight() const
{
    Q_D(const QQuickControl);
    if (!d->background)
        return 0;
    return d->background->implicitHeight();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::topInset

    This property holds the top inset for the background.

    \sa {Control Layout}, bottomInset
*/
qreal QQuickControl::topInset() const
{
    Q_D(const QQuickControl);
    return d->getTopInset();
}

void QQuickControl::setTopInset(qreal inset)
{
    Q_D(QQuickControl);
    d->setTopInset(inset);
}

void QQuickControl::resetTopInset()
{
    Q_D(QQuickControl);
    d->setTopInset(0, true);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::leftInset

    This property holds the left inset for the background.

    \sa {Control Layout}, rightInset
*/
qreal QQuickControl::leftInset() const
{
    Q_D(const QQuickControl);
    return d->getLeftInset();
}

void QQuickControl::setLeftInset(qreal inset)
{
    Q_D(QQuickControl);
    d->setLeftInset(inset);
}

void QQuickControl::resetLeftInset()
{
    Q_D(QQuickControl);
    d->setLeftInset(0, true);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::rightInset

    This property holds the right inset for the background.

    \sa {Control Layout}, leftInset
*/
qreal QQuickControl::rightInset() const
{
    Q_D(const QQuickControl);
    return d->getRightInset();
}

void QQuickControl::setRightInset(qreal inset)
{
    Q_D(QQuickControl);
    d->setRightInset(inset);
}

void QQuickControl::resetRightInset()
{
    Q_D(QQuickControl);
    d->setRightInset(0, true);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::Control::bottomInset

    This property holds the bottom inset for the background.

    \sa {Control Layout}, topInset
*/
qreal QQuickControl::bottomInset() const
{
    Q_D(const QQuickControl);
    return d->getBottomInset();
}

void QQuickControl::setBottomInset(qreal inset)
{
    Q_D(QQuickControl);
    d->setBottomInset(inset);
}

void QQuickControl::resetBottomInset()
{
    Q_D(QQuickControl);
    d->setBottomInset(0, true);
}

void QQuickControl::classBegin()
{
    Q_D(QQuickControl);
    QQuickItem::classBegin();
    d->resolveFont();
}

void QQuickControl::componentComplete()
{
    Q_D(QQuickControl);
    d->executeBackground(true);
    d->executeContentItem(true);
    QQuickItem::componentComplete();
    d->resizeBackground();
    d->resizeContent();
    d->updateBaselineOffset();
    if (!d->hasLocale)
        d->locale = QQuickControlPrivate::calcLocale(d->parentItem);
#if QT_CONFIG(quicktemplates2_hover)
    if (!d->explicitHoverEnabled)
        setAcceptHoverEvents(QQuickControlPrivate::calcHoverEnabled(d->parentItem));
#endif
#if QT_CONFIG(accessibility)
    if (QAccessible::isActive())
        accessibilityActiveChanged(true);
#endif
}

QFont QQuickControl::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::System);
}

void QQuickControl::focusInEvent(QFocusEvent *event)
{
    QQuickItem::focusInEvent(event);
}

void QQuickControl::focusOutEvent(QFocusEvent *event)
{
    QQuickItem::focusOutEvent(event);
}

#if QT_CONFIG(quicktemplates2_hover)
void QQuickControl::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(QQuickControl);
    setHovered(d->hoverEnabled);
    event->ignore();
}

void QQuickControl::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(QQuickControl);
    setHovered(d->hoverEnabled && contains(event->position()));
    event->ignore();
}

void QQuickControl::hoverLeaveEvent(QHoverEvent *event)
{
    setHovered(false);
    event->ignore();
}
#endif

void QQuickControl::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickControl);
    event->setAccepted(d->handlePress(event->position(), event->timestamp()));
}

void QQuickControl::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickControl);
    event->setAccepted(d->handleMove(event->position(), event->timestamp()));
}

void QQuickControl::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickControl);
    event->setAccepted(d->handleRelease(event->position(), event->timestamp()));
}

void QQuickControl::mouseUngrabEvent()
{
    Q_D(QQuickControl);
    d->handleUngrab();
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickControl::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickControl);
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        for (const QTouchEvent::TouchPoint &point : event->points()) {
            if (!d->acceptTouch(point))
                continue;

            switch (point.state()) {
            case QEventPoint::Pressed:
                d->handlePress(point.position(), event->timestamp());
                break;
            case QEventPoint::Updated:
                d->handleMove(point.position(), event->timestamp());
                break;
            case QEventPoint::Released:
                d->handleRelease(point.position(), event->timestamp());
                break;
            default:
                break;
            }
        }
        break;

    case QEvent::TouchCancel:
        d->handleUngrab();
        break;

    default:
        QQuickItem::touchEvent(event);
        break;
    }
}

void QQuickControl::touchUngrabEvent()
{
    Q_D(QQuickControl);
    d->handleUngrab();
}
#endif

#if QT_CONFIG(wheelevent)
void QQuickControl::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickControl);
    event->setAccepted(d->wheelEnabled);
}
#endif

void QQuickControl::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickControl);
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    d->resizeBackground();
    d->resizeContent();
    if (!qFuzzyCompare(newGeometry.width(), oldGeometry.width()))
        emit availableWidthChanged();
    if (!qFuzzyCompare(newGeometry.height(), oldGeometry.height()))
        emit availableHeightChanged();
}

void QQuickControl::enabledChange()
{
}

void QQuickControl::fontChange(const QFont &newFont, const QFont &oldFont)
{
    Q_UNUSED(newFont);
    Q_UNUSED(oldFont);
}

#if QT_CONFIG(quicktemplates2_hover)
void QQuickControl::hoverChange()
{
}
#endif

void QQuickControl::mirrorChange()
{
    emit mirroredChanged();
}

void QQuickControl::spacingChange(qreal newSpacing, qreal oldSpacing)
{
    Q_UNUSED(newSpacing);
    Q_UNUSED(oldSpacing);
}

void QQuickControl::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickControl);
    Q_UNUSED(newPadding);
    Q_UNUSED(oldPadding);
    d->resizeContent();
    d->updateBaselineOffset();
}

void QQuickControl::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_UNUSED(newItem);
    Q_UNUSED(oldItem);
}

void QQuickControl::localeChange(const QLocale &newLocale, const QLocale &oldLocale)
{
    Q_UNUSED(newLocale);
    Q_UNUSED(oldLocale);
}

void QQuickControl::insetChange(const QMarginsF &newInset, const QMarginsF &oldInset)
{
    Q_D(QQuickControl);
    Q_UNUSED(newInset);
    Q_UNUSED(oldInset);
    d->resizeBackground();
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickControl::accessibleRole() const
{
    return QAccessible::NoRole;
}

void QQuickControl::accessibilityActiveChanged(bool active)
{
    Q_D(QQuickControl);
    if (!active)
        return;

    QQuickAccessibleAttached *accessibleAttached = qobject_cast<QQuickAccessibleAttached *>(qmlAttachedPropertiesObject<QQuickAccessibleAttached>(this, true));
    Q_ASSERT(accessibleAttached);
    accessibleAttached->setRole(d->effectiveAccessibleRole());
}
#endif

QString QQuickControl::accessibleName() const
{
#if QT_CONFIG(accessibility)
    if (QQuickAccessibleAttached *accessibleAttached = QQuickControlPrivate::accessibleAttached(this))
        return accessibleAttached->name();
#endif
    return QString();
}

void QQuickControl::maybeSetAccessibleName(const QString &name)
{
#if QT_CONFIG(accessibility)
    if (QQuickAccessibleAttached *accessibleAttached = QQuickControlPrivate::accessibleAttached(this)) {
        if (!accessibleAttached->wasNameExplicitlySet())
            accessibleAttached->setNameImplicitly(name);
    }
#else
    Q_UNUSED(name);
#endif
}

QVariant QQuickControl::accessibleProperty(const char *propertyName)
{
#if QT_CONFIG(accessibility)
    if (QAccessible::isActive())
        return QQuickAccessibleAttached::property(this, propertyName);
#endif
    Q_UNUSED(propertyName);
    return QVariant();
}

bool QQuickControl::setAccessibleProperty(const char *propertyName, const QVariant &value)
{
#if QT_CONFIG(accessibility)
    if (QAccessible::isActive())
        return QQuickAccessibleAttached::setProperty(this, propertyName, value);
#endif
    Q_UNUSED(propertyName);
    Q_UNUSED(value);
    return false;
}

QT_END_NAMESPACE

#include "moc_qquickcontrol_p.cpp"
