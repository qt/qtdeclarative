// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktextarea_p.h"
#include "qquicktextarea_p_p.h"
#include "qquickcontrol_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickscrollview_p.h"
#include "qquickdeferredexecute_p_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickclipnode_p.h>
#include <QtQuick/private/qquickflickable_p.h>

#if QT_CONFIG(accessibility)
#include <QtQuick/private/qquickaccessibleattached_p.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \qmltype TextArea
    \inherits TextEdit
//!     \nativetype QQuickTextArea
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-input
    \brief Multi-line text input area.

    TextArea is a multi-line text editor. TextArea extends TextEdit with
    a \l {placeholderText}{placeholder text} functionality, and adds decoration.

    \image qtquickcontrols-textarea.png

    \code
    TextArea {
        placeholderText: qsTr("Enter description")
    }
    \endcode

    TextArea is not scrollable by itself. Especially on screen-size constrained
    platforms, it is often preferable to make entire application pages scrollable.
    On such a scrollable page, a non-scrollable TextArea might behave better than
    nested scrollable controls. Notice, however, that in such a scenario, the background
    decoration of the TextArea scrolls together with the rest of the scrollable
    content.

    \section2 Scrollable TextArea

    If you want to make a TextArea scrollable, for example, when it covers
    an entire application page, it can be placed inside a \l ScrollView.

    \image qtquickcontrols-textarea-scrollable.png

    \snippet qtquickcontrols-textarea-scrollable.qml 1

    A TextArea that is placed inside a \l ScrollView does the following:

    \list
    \li Sets the content size automatically
    \li Ensures that the background decoration stays in place
    \li Clips the content
    \endlist

    \section2 Tab Focus

    By default, pressing the tab key while TextArea has
    \l {Item::activeFocus}{active focus} results in a tab character being input
    into the control itself. To make tab pass active focus onto another item,
    use the attached \l KeyNavigation properties:

    \code
    TextField {
        id: textField
    }

    TextArea {
        KeyNavigation.priority: KeyNavigation.BeforeItem
        KeyNavigation.tab: textField
    }
    \endcode

    \sa TextField, {Customizing TextArea}, {Input Controls}
*/

/*!
    \qmlsignal QtQuick.Controls::TextArea::pressAndHold(MouseEvent event)

    This signal is emitted when there is a long press (the delay depends on the platform plugin).
    The \a event parameter provides information about the press, including the x and y
    coordinates of the press, and which button is pressed.

    \sa pressed, released
*/

/*!
    \qmlsignal QtQuick.Controls::TextArea::pressed(MouseEvent event)
    \since QtQuick.Controls 2.1 (Qt 5.8)

    This signal is emitted when the text area is pressed by the user.
    The \a event parameter provides information about the press,
    including the x and y coordinates of the press, and which button is pressed.

    \sa released, pressAndHold
*/

/*!
    \qmlsignal QtQuick.Controls::TextArea::released(MouseEvent event)
    \since QtQuick.Controls 2.1 (Qt 5.8)

    This signal is emitted when the text area is released by the user.
    The \a event parameter provides information about the release,
    including the x and y coordinates of the press, and which button
    is pressed.

    \sa pressed, pressAndHold
*/

QQuickTextAreaPrivate::QQuickTextAreaPrivate()
{
}

QQuickTextAreaPrivate::~QQuickTextAreaPrivate()
{
}

void QQuickTextAreaPrivate::setTopInset(qreal value, bool reset)
{
    Q_Q(QQuickTextArea);
    const QMarginsF oldInset = getInset();
    extra.value().topInset = value;
    extra.value().hasTopInset = !reset;
    if (!qFuzzyCompare(oldInset.top(), value)) {
        emit q->topInsetChanged();
        q->insetChange(getInset(), oldInset);
    }
}

void QQuickTextAreaPrivate::setLeftInset(qreal value, bool reset)
{
    Q_Q(QQuickTextArea);
    const QMarginsF oldInset = getInset();
    extra.value().leftInset = value;
    extra.value().hasLeftInset = !reset;
    if (!qFuzzyCompare(oldInset.left(), value)) {
        emit q->leftInsetChanged();
        q->insetChange(getInset(), oldInset);
    }
}

void QQuickTextAreaPrivate::setRightInset(qreal value, bool reset)
{
    Q_Q(QQuickTextArea);
    const QMarginsF oldInset = getInset();
    extra.value().rightInset = value;
    extra.value().hasRightInset = !reset;
    if (!qFuzzyCompare(oldInset.right(), value)) {
        emit q->rightInsetChanged();
        q->insetChange(getInset(), oldInset);
    }
}

void QQuickTextAreaPrivate::setBottomInset(qreal value, bool reset)
{
    Q_Q(QQuickTextArea);
    const QMarginsF oldInset = getInset();
    extra.value().bottomInset = value;
    extra.value().hasBottomInset = !reset;
    if (!qFuzzyCompare(oldInset.bottom(), value)) {
        emit q->bottomInsetChanged();
        q->insetChange(getInset(), oldInset);
    }
}

void QQuickTextAreaPrivate::resizeBackground()
{
    if (!background)
        return;

    resizingBackground = true;

    // When using the attached property TextArea.flickable, we reparent the background out
    // of TextArea and into the Flickable since we don't want the background to move while
    // flicking. This means that the size of the background should also follow the size of
    // the Flickable rather than the size of the TextArea.
    const auto flickable = qobject_cast<QQuickFlickable *>(background->parentItem());

    QQuickItemPrivate *p = QQuickItemPrivate::get(background);
    if (((!p->widthValid() || !extra.isAllocated() || !extra->hasBackgroundWidth) && qFuzzyIsNull(background->x()))
            || (extra.isAllocated() && (extra->hasLeftInset || extra->hasRightInset))) {
        const qreal bgWidth = flickable ? flickable->width() : width;
        background->setX(getLeftInset());
        background->setWidth(bgWidth - getLeftInset() - getRightInset());
    }

    if (((!p->heightValid() || !extra.isAllocated() || !extra->hasBackgroundHeight) && qFuzzyIsNull(background->y()))
            || (extra.isAllocated() && (extra->hasTopInset || extra->hasBottomInset))) {
        const qreal bgHeight = flickable ? flickable->height() : height;
        background->setY(getTopInset());
        background->setHeight(bgHeight - getTopInset() - getBottomInset());
    }

    resizingBackground = false;
}

/*!
    \internal

    Determine which font is implicitly imposed on this control by its ancestors
    and QGuiApplication::font, resolve this against its own font (attributes from
    the implicit font are copied over). Then propagate this font to this
    control's children.
*/
void QQuickTextAreaPrivate::resolveFont()
{
    Q_Q(QQuickTextArea);
    inheritFont(QQuickControlPrivate::parentFont(q));
}

void QQuickTextAreaPrivate::inheritFont(const QFont &font)
{
    QFont parentFont = extra.isAllocated() ? extra->requestedFont.resolve(font) : font;
    parentFont.setResolveMask(extra.isAllocated() ? extra->requestedFont.resolveMask() | font.resolveMask() : font.resolveMask());

    const QFont defaultFont = QQuickTheme::font(QQuickTheme::TextArea);
    QFont resolvedFont = parentFont.resolve(defaultFont);

    setFont_helper(resolvedFont);
}

/*!
    \internal

    Assign \a font to this control, and propagate it to all children.
*/
void QQuickTextAreaPrivate::updateFont(const QFont &font)
{
    Q_Q(QQuickTextArea);
    QFont oldFont = sourceFont;
    q->QQuickTextEdit::setFont(font);

    QQuickControlPrivate::updateFontRecur(q, font);

    if (oldFont != font)
        emit q->fontChanged();
}

#if QT_CONFIG(quicktemplates2_hover)
void QQuickTextAreaPrivate::updateHoverEnabled(bool enabled, bool xplicit)
{
    Q_Q(QQuickTextArea);
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
#endif

void QQuickTextAreaPrivate::attachFlickable(QQuickFlickable *item)
{
    Q_Q(QQuickTextArea);
    flickable = item;
    q->setParentItem(flickable->contentItem());

    if (background)
        background->setParentItem(flickable);

    QObjectPrivate::connect(q, &QQuickTextArea::contentSizeChanged, this, &QQuickTextAreaPrivate::resizeFlickableContent);
    QObjectPrivate::connect(q, &QQuickTextEdit::cursorRectangleChanged, this, &QQuickTextAreaPrivate::ensureCursorVisible);

    QObject::connect(flickable, &QQuickFlickable::contentXChanged, q, &QQuickItem::update);
    QObject::connect(flickable, &QQuickFlickable::contentYChanged, q, &QQuickItem::update);

    QQuickItemPrivate::get(flickable)->updateOrAddGeometryChangeListener(this, QQuickGeometryChange::Size);
    QQuickItemPrivate::get(flickable)->addItemChangeListener(this, QQuickItemPrivate::Destroyed);
    QObjectPrivate::connect(flickable, &QQuickFlickable::contentWidthChanged, this, &QQuickTextAreaPrivate::resizeFlickableControl);
    QObjectPrivate::connect(flickable, &QQuickFlickable::contentHeightChanged, this, &QQuickTextAreaPrivate::resizeFlickableControl);

    resizeFlickableControl();
}

void QQuickTextAreaPrivate::detachFlickable()
{
    Q_Q(QQuickTextArea);
    q->setParentItem(nullptr);
    if (background && background->parentItem() == flickable)
        background->setParentItem(q);

    QObjectPrivate::disconnect(q, &QQuickTextArea::contentSizeChanged, this, &QQuickTextAreaPrivate::resizeFlickableContent);
    QObjectPrivate::disconnect(q, &QQuickTextEdit::cursorRectangleChanged, this, &QQuickTextAreaPrivate::ensureCursorVisible);

    QObject::disconnect(flickable, &QQuickFlickable::contentXChanged, q, &QQuickItem::update);
    QObject::disconnect(flickable, &QQuickFlickable::contentYChanged, q, &QQuickItem::update);

    QQuickItemPrivate::get(flickable)->updateOrRemoveGeometryChangeListener(this, QQuickGeometryChange::Nothing);
    QQuickItemPrivate::get(flickable)->removeItemChangeListener(this, QQuickItemPrivate::Destroyed);
    QObjectPrivate::disconnect(flickable, &QQuickFlickable::contentWidthChanged, this, &QQuickTextAreaPrivate::resizeFlickableControl);
    QObjectPrivate::disconnect(flickable, &QQuickFlickable::contentHeightChanged, this, &QQuickTextAreaPrivate::resizeFlickableControl);

    flickable = nullptr;

    resizeBackground();
}

void QQuickTextAreaPrivate::ensureCursorVisible()
{
    Q_Q(QQuickTextArea);
    if (!flickable)
        return;

    const qreal cx = flickable->contentX();
    const qreal cy = flickable->contentY();
    const qreal w = flickable->width();
    const qreal h = flickable->height();

    const qreal tp = q->topPadding();
    const qreal lp = q->leftPadding();
    const QRectF cr = q->cursorRectangle();

    if (cr.left() <= cx + lp) {
        flickable->setContentX(cr.left() - lp);
    } else {
        // calculate the rectangle of the next character and ensure that
        // it's visible if it's on the same line with the cursor
        const qreal rp = q->rightPadding();
        const QRectF nr = q->cursorPosition() < q->length() ? q->positionToRectangle(q->cursorPosition() + 1) : QRectF();
        if (qFuzzyCompare(nr.y(), cr.y()) && nr.right() >= cx + lp + w - rp)
            flickable->setContentX(nr.right() - w + rp);
        else if (cr.right() >= cx + lp + w - rp)
            flickable->setContentX(cr.right() - w + rp);
    }

    if (cr.top() <= cy + tp) {
        flickable->setContentY(cr.top() - tp);
    } else {
        const qreal bp = q->bottomPadding();
        if (cr.bottom() >= cy + tp + h - bp && cr.bottom() <= flickable->contentHeight())
            flickable->setContentY(cr.bottom() - h + bp);
    }
}

void QQuickTextAreaPrivate::resizeFlickableControl()
{
    Q_Q(QQuickTextArea);
    if (!flickable)
        return;

    const qreal w = wrapMode == QQuickTextArea::NoWrap ? qMax(flickable->width(), flickable->contentWidth()) : flickable->width();
    const qreal h = qMax(flickable->height(), flickable->contentHeight());
    q->setSize(QSizeF(w, h));

    resizeBackground();
}

void QQuickTextAreaPrivate::resizeFlickableContent()
{
    Q_Q(QQuickTextArea);
    if (!flickable)
        return;

    flickable->setContentWidth(q->implicitWidth());
    flickable->setContentHeight(q->implicitHeight());
}

void QQuickTextAreaPrivate::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff)
{
    Q_UNUSED(diff);
    if (!resizingBackground && item == background) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(item);
        // Only set hasBackgroundWidth/Height if it was a width/height change,
        // otherwise we're prevented from setting a width/height in the future.
        if (change.widthChange())
            extra.value().hasBackgroundWidth = p->widthValid();
        if (change.heightChange())
            extra.value().hasBackgroundHeight = p->heightValid();
    }

    if (flickable)
        resizeFlickableControl();
    else
        resizeBackground();
}

qreal QQuickTextAreaPrivate::getImplicitWidth() const
{
    return QQuickItemPrivate::getImplicitWidth();
}

qreal QQuickTextAreaPrivate::getImplicitHeight() const
{
    return QQuickItemPrivate::getImplicitHeight();
}

void QQuickTextAreaPrivate::implicitWidthChanged()
{
    Q_Q(QQuickTextArea);
    QQuickItemPrivate::implicitWidthChanged();
    emit q->implicitWidthChanged3();
}

void QQuickTextAreaPrivate::implicitHeightChanged()
{
    Q_Q(QQuickTextArea);
    QQuickItemPrivate::implicitHeightChanged();
    emit q->implicitHeightChanged3();
}

#if QT_CONFIG(accessibility)
void QQuickTextAreaPrivate::accessibilityActiveChanged(bool active)
{
    QQuickTextEditPrivate::accessibilityActiveChanged(active);
    if (QQuickAccessibleAttached *accessibleAttached = QQuickControlPrivate::accessibleAttached(q_func()))
        accessibleAttached->setDescriptionImplicitly(placeholder);
}
#endif

void QQuickTextAreaPrivate::cancelBackground()
{
    Q_Q(QQuickTextArea);
    quickCancelDeferred(q, backgroundName());
}

void QQuickTextAreaPrivate::executeBackground(bool complete)
{
    Q_Q(QQuickTextArea);
    if (background.wasExecuted())
        return;

    if (!background || complete)
        quickBeginDeferred(q, backgroundName(), background);
    if (complete)
        quickCompleteDeferred(q, backgroundName(), background);
}

void QQuickTextAreaPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_Q(QQuickTextArea);
    if (item == background)
        emit q->implicitBackgroundWidthChanged();
}

void QQuickTextAreaPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_Q(QQuickTextArea);
    if (item == background)
        emit q->implicitBackgroundHeightChanged();
}

void QQuickTextAreaPrivate::itemDestroyed(QQuickItem *item)
{
    Q_Q(QQuickTextArea);
    if (item == background) {
        background = nullptr;
        emit q->implicitBackgroundWidthChanged();
        emit q->implicitBackgroundHeightChanged();
    } else if (item == flickable) {
        detachFlickable();
    }
}

QPalette QQuickTextAreaPrivate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::TextArea);
}

bool QQuickTextAreaPrivate::setLastFocusChangeReason(Qt::FocusReason reason)
{
    Q_Q(QQuickTextArea);
    const auto focusReasonChanged = QQuickItemPrivate::setLastFocusChangeReason(reason);
    if (focusReasonChanged)
        emit q->focusReasonChanged();

    return focusReasonChanged;
}

QQuickTextArea::QQuickTextArea(QQuickItem *parent)
    : QQuickTextEdit(*(new QQuickTextAreaPrivate), parent)
{
    Q_D(QQuickTextArea);
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    d->setImplicitResizeEnabled(false);
    d->pressHandler.control = this;

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    if (qEnvironmentVariable("QT_QUICK_CONTROLS_TEXT_SELECTION_BEHAVIOR") == u"old"_s)
        QQuickTextEdit::setOldSelectionDefault();
#endif
}

QQuickTextArea::~QQuickTextArea()
{
    Q_D(QQuickTextArea);
    if (d->flickable)
        d->detachFlickable();
    QQuickControlPrivate::removeImplicitSizeListener(d->background, d, QQuickControlPrivate::ImplicitSizeChanges | QQuickItemPrivate::Geometry);
}

QQuickTextAreaAttached *QQuickTextArea::qmlAttachedProperties(QObject *object)
{
    return new QQuickTextAreaAttached(object);
}

QFont QQuickTextArea::font() const
{
    Q_D(const QQuickTextArea);
    QFont font = QQuickTextEdit::font();
    // The resolve mask should inherit from the requestedFont
    font.setResolveMask(d->extra.value().requestedFont.resolveMask());
    return font;
}

void QQuickTextArea::setFont(const QFont &font)
{
    Q_D(QQuickTextArea);
    if (d->extra.value().requestedFont.resolveMask() == font.resolveMask() && d->extra.value().requestedFont == font)
        return;

    d->extra.value().requestedFont = font;
    d->resolveFont();
}

/*!
    \qmlproperty Item QtQuick.Controls::TextArea::background

    This property holds the background item.

    \input qquickcontrol-background.qdocinc notes

    \sa {Customizing TextArea}
*/
QQuickItem *QQuickTextArea::background() const
{
    QQuickTextAreaPrivate *d = const_cast<QQuickTextAreaPrivate *>(d_func());
    if (!d->background)
        d->executeBackground();
    return d->background;
}

void QQuickTextArea::setBackground(QQuickItem *background)
{
    Q_D(QQuickTextArea);
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

    QQuickControlPrivate::removeImplicitSizeListener(d->background, d, QQuickControlPrivate::ImplicitSizeChanges | QQuickItemPrivate::Geometry);
    QQuickControlPrivate::hideOldItem(d->background);
    d->background = background;

    if (background) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(background);
        if (p->widthValid() || p->heightValid()) {
            d->extra.value().hasBackgroundWidth = p->widthValid();
            d->extra.value().hasBackgroundHeight = p->heightValid();
        }
        if (d->flickable)
            background->setParentItem(d->flickable);
        else
            background->setParentItem(this);
        if (qFuzzyIsNull(background->z()))
            background->setZ(-1);
        if (isComponentComplete())
            d->resizeBackground();
        QQuickControlPrivate::addImplicitSizeListener(background, d, QQuickControlPrivate::ImplicitSizeChanges | QQuickItemPrivate::Geometry);
    }

    if (!qFuzzyCompare(oldImplicitBackgroundWidth, implicitBackgroundWidth()))
        emit implicitBackgroundWidthChanged();
    if (!qFuzzyCompare(oldImplicitBackgroundHeight, implicitBackgroundHeight()))
        emit implicitBackgroundHeightChanged();
    if (!d->background.isExecuting())
        emit backgroundChanged();
}

/*!
    \qmlproperty string QtQuick.Controls::TextArea::placeholderText

    This property holds the short hint that is displayed in the text area before
    the user enters a value.
*/
QString QQuickTextArea::placeholderText() const
{
    Q_D(const QQuickTextArea);
    return d->placeholder;
}

void QQuickTextArea::setPlaceholderText(const QString &text)
{
    Q_D(QQuickTextArea);
    if (d->placeholder == text)
        return;

    d->placeholder = text;
#if QT_CONFIG(accessibility)
    if (QQuickAccessibleAttached *accessibleAttached = QQuickControlPrivate::accessibleAttached(this))
        accessibleAttached->setDescriptionImplicitly(text);
#endif
    emit placeholderTextChanged();
}

/*!
    \qmlproperty color QtQuick.Controls::TextArea::placeholderTextColor
    \since QtQuick.Controls 2.5 (Qt 5.12)

    This property holds the color of placeholderText.

    \sa placeholderText
*/
QColor QQuickTextArea::placeholderTextColor() const
{
    Q_D(const QQuickTextArea);
    return d->placeholderColor;
}

void QQuickTextArea::setPlaceholderTextColor(const QColor &color)
{
    Q_D(QQuickTextArea);
    if (d->placeholderColor == color)
        return;

    d->placeholderColor = color;
    emit placeholderTextColorChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::TextArea::focusReason

    This property holds the reason of the last focus change.

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

    \note Prefer \l {QtQuick.Controls::Control::focusReason} to this property.
*/
Qt::FocusReason QQuickTextArea::focusReason() const
{
    Q_D(const QQuickTextArea);
    return d->lastFocusChangeReason();
}

void QQuickTextArea::setFocusReason(Qt::FocusReason reason)
{
    Q_D(QQuickTextArea);
    d->setLastFocusChangeReason(reason);
}


bool QQuickTextArea::contains(const QPointF &point) const
{
    Q_D(const QQuickTextArea);
    if (d->flickable && !d->flickable->contains(d->flickable->mapFromItem(this, point)))
        return false;
    return QQuickTextEdit::contains(point);
}

/*!
    \since QtQuick.Controls 2.1 (Qt 5.8)
    \qmlproperty bool QtQuick.Controls::TextArea::hovered
    \readonly

    This property holds whether the text area is hovered.

    \sa hoverEnabled
*/
bool QQuickTextArea::isHovered() const
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(const QQuickTextArea);
    return d->hovered;
#else
    return false;
#endif
}

void QQuickTextArea::setHovered(bool hovered)
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(QQuickTextArea);
    if (hovered == d->hovered)
        return;

    d->hovered = hovered;
    emit hoveredChanged();
#else
    Q_UNUSED(hovered);
#endif
}

/*!
    \since QtQuick.Controls 2.1 (Qt 5.8)
    \qmlproperty bool QtQuick.Controls::TextArea::hoverEnabled

    This property determines whether the text area accepts hover events. The default value is \c true.

    \sa hovered
*/
bool QQuickTextArea::isHoverEnabled() const
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(const QQuickTextArea);
    return d->hoverEnabled;
#else
    return false;
#endif
}

void QQuickTextArea::setHoverEnabled(bool enabled)
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(QQuickTextArea);
    if (d->explicitHoverEnabled && enabled == d->hoverEnabled)
        return;

    d->updateHoverEnabled(enabled, true); // explicit=true
#else
    Q_UNUSED(enabled);
#endif
}

void QQuickTextArea::resetHoverEnabled()
{
#if QT_CONFIG(quicktemplates2_hover)
    Q_D(QQuickTextArea);
    if (!d->explicitHoverEnabled)
        return;

    d->explicitHoverEnabled = false;
    d->updateHoverEnabled(QQuickControlPrivate::calcHoverEnabled(d->parentItem), false); // explicit=false
#endif
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::TextArea::implicitBackgroundWidth
    \readonly

    This property holds the implicit background width.

    The value is equal to \c {background ? background.implicitWidth : 0}.

    \sa implicitBackgroundHeight
*/
qreal QQuickTextArea::implicitBackgroundWidth() const
{
    Q_D(const QQuickTextArea);
    if (!d->background)
        return 0;
    return d->background->implicitWidth();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::TextArea::implicitBackgroundHeight
    \readonly

    This property holds the implicit background height.

    The value is equal to \c {background ? background.implicitHeight : 0}.

    \sa implicitBackgroundWidth
*/
qreal QQuickTextArea::implicitBackgroundHeight() const
{
    Q_D(const QQuickTextArea);
    if (!d->background)
        return 0;
    return d->background->implicitHeight();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::TextArea::topInset

    This property holds the top inset for the background.

    \sa {Control Layout}, bottomInset
*/
qreal QQuickTextArea::topInset() const
{
    Q_D(const QQuickTextArea);
    return d->getTopInset();
}

void QQuickTextArea::setTopInset(qreal inset)
{
    Q_D(QQuickTextArea);
    d->setTopInset(inset);
}

void QQuickTextArea::resetTopInset()
{
    Q_D(QQuickTextArea);
    d->setTopInset(0, true);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::TextArea::leftInset

    This property holds the left inset for the background.

    \sa {Control Layout}, rightInset
*/
qreal QQuickTextArea::leftInset() const
{
    Q_D(const QQuickTextArea);
    return d->getLeftInset();
}

void QQuickTextArea::setLeftInset(qreal inset)
{
    Q_D(QQuickTextArea);
    d->setLeftInset(inset);
}

void QQuickTextArea::resetLeftInset()
{
    Q_D(QQuickTextArea);
    d->setLeftInset(0, true);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::TextArea::rightInset

    This property holds the right inset for the background.

    \sa {Control Layout}, leftInset
*/
qreal QQuickTextArea::rightInset() const
{
    Q_D(const QQuickTextArea);
    return d->getRightInset();
}

void QQuickTextArea::setRightInset(qreal inset)
{
    Q_D(QQuickTextArea);
    d->setRightInset(inset);
}

void QQuickTextArea::resetRightInset()
{
    Q_D(QQuickTextArea);
    d->setRightInset(0, true);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::TextArea::bottomInset

    This property holds the bottom inset for the background.

    \sa {Control Layout}, topInset
*/
qreal QQuickTextArea::bottomInset() const
{
    Q_D(const QQuickTextArea);
    return d->getBottomInset();
}

void QQuickTextArea::setBottomInset(qreal inset)
{
    Q_D(QQuickTextArea);
    d->setBottomInset(inset);
}

void QQuickTextArea::resetBottomInset()
{
    Q_D(QQuickTextArea);
    d->setBottomInset(0, true);
}

void QQuickTextArea::classBegin()
{
    Q_D(QQuickTextArea);
    QQuickTextEdit::classBegin();
    d->resolveFont();
}

void QQuickTextArea::componentComplete()
{
    Q_D(QQuickTextArea);
    d->executeBackground(true);
    QQuickTextEdit::componentComplete();
    d->resizeBackground();
#if QT_CONFIG(quicktemplates2_hover)
    if (!d->explicitHoverEnabled)
        setAcceptHoverEvents(QQuickControlPrivate::calcHoverEnabled(d->parentItem));
#endif
}

void QQuickTextArea::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_D(QQuickTextArea);
    QQuickTextEdit::itemChange(change, value);
    switch (change) {
    case ItemEnabledHasChanged:
        break;
    case ItemSceneChange:
    case ItemParentHasChanged:
        if ((change == ItemParentHasChanged && value.item) || (change == ItemSceneChange && value.window)) {
            d->resolveFont();
#if QT_CONFIG(quicktemplates2_hover)
            if (!d->explicitHoverEnabled)
                d->updateHoverEnabled(QQuickControlPrivate::calcHoverEnabled(d->parentItem), false); // explicit=false
#endif
            if (change == ItemParentHasChanged) {
                QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(value.item->parentItem());
                if (flickable) {
                    QQuickScrollView *scrollView = qobject_cast<QQuickScrollView *>(flickable->parentItem());
                    if (scrollView)
                        d->attachFlickable(flickable);
                }
            }
        }
        break;
    default:
        break;
    }
}

void QQuickTextArea::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTextArea);
    QQuickTextEdit::geometryChange(newGeometry, oldGeometry);
    d->resizeBackground();
}

void QQuickTextArea::insetChange(const QMarginsF &newInset, const QMarginsF &oldInset)
{
    Q_D(QQuickTextArea);
    Q_UNUSED(newInset);
    Q_UNUSED(oldInset);
    d->resizeBackground();
}

QSGNode *QQuickTextArea::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_D(QQuickTextArea);
    QQuickDefaultClipNode *clipNode = static_cast<QQuickDefaultClipNode *>(oldNode);
    if (!clipNode)
        clipNode = new QQuickDefaultClipNode(QRectF());

    QQuickItem *clipper = this;
    if (d->flickable)
        clipper = d->flickable;

    const QRectF cr = clipper->clipRect().adjusted(
            leftPadding(), topPadding(),
            (!d->cursorItem && effectiveHAlign() == HAlignment::AlignRight ? 1 : 0) - rightPadding(),
            -bottomPadding());

    clipNode->setRect(!d->flickable ? cr : cr.translated(d->flickable->contentX(), d->flickable->contentY()));
    clipNode->update();

    QSGNode *textNode = QQuickTextEdit::updatePaintNode(clipNode->firstChild(), data);
    if (!textNode->parent())
        clipNode->appendChildNode(textNode);

    if (d->cursorItem) {
        QQuickDefaultClipNode *cursorNode = QQuickItemPrivate::get(d->cursorItem)->clipNode();
        if (cursorNode)
            cursorNode->setClipRect(d->cursorItem->mapRectFromItem(clipper, cr));
    }

    return clipNode;
}

void QQuickTextArea::focusInEvent(QFocusEvent *event)
{
    QQuickTextEdit::focusInEvent(event);
}

void QQuickTextArea::focusOutEvent(QFocusEvent *event)
{
    QQuickTextEdit::focusOutEvent(event);
}

#if QT_CONFIG(quicktemplates2_hover)
void QQuickTextArea::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(QQuickTextArea);
    QQuickTextEdit::hoverEnterEvent(event);
    setHovered(d->hoverEnabled);
    event->ignore();
}

void QQuickTextArea::hoverLeaveEvent(QHoverEvent *event)
{
    QQuickTextEdit::hoverLeaveEvent(event);
    setHovered(false);
    event->ignore();
}
#endif

void QQuickTextArea::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickTextArea);
    d->pressHandler.mousePressEvent(event);
    if (d->pressHandler.isActive()) {
        if (d->pressHandler.delayedMousePressEvent) {
            QQuickTextEdit::mousePressEvent(d->pressHandler.delayedMousePressEvent.get());
            d->pressHandler.clearDelayedMouseEvent();
        }
        // Calling the base class implementation will result in QQuickTextControl's
        // press handler being called, which ignores events that aren't Qt::LeftButton.
        const bool wasAccepted = event->isAccepted();
        QQuickTextEdit::mousePressEvent(event);
        if (wasAccepted)
            event->accept();
    }
}

void QQuickTextArea::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickTextArea);
    d->pressHandler.mouseMoveEvent(event);
    if (d->pressHandler.isActive()) {
        if (d->pressHandler.delayedMousePressEvent) {
            QQuickTextEdit::mousePressEvent(d->pressHandler.delayedMousePressEvent.get());
            d->pressHandler.clearDelayedMouseEvent();
        }
        QQuickTextEdit::mouseMoveEvent(event);
    }
}

void QQuickTextArea::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickTextArea);
    d->pressHandler.mouseReleaseEvent(event);
    if (d->pressHandler.isActive()) {
        if (d->pressHandler.delayedMousePressEvent) {
            QQuickTextEdit::mousePressEvent(d->pressHandler.delayedMousePressEvent.get());
            d->pressHandler.clearDelayedMouseEvent();
        }
        QQuickTextEdit::mouseReleaseEvent(event);
    }
}

void QQuickTextArea::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickTextArea);
    if (d->pressHandler.delayedMousePressEvent) {
        QQuickTextEdit::mousePressEvent(d->pressHandler.delayedMousePressEvent.get());
        d->pressHandler.clearDelayedMouseEvent();
    }
    QQuickTextEdit::mouseDoubleClickEvent(event);
}

void QQuickTextArea::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickTextArea);
    if (event->timerId() == d->pressHandler.timer.timerId())
        d->pressHandler.timerEvent(event);
    else
        QQuickTextEdit::timerEvent(event);
}

class QQuickTextAreaAttachedPrivate : public QObjectPrivate
{
public:
    QQuickTextArea *control = nullptr;
};

QQuickTextAreaAttached::QQuickTextAreaAttached(QObject *parent)
    : QObject(*(new QQuickTextAreaAttachedPrivate), parent)
{
}

/*!
    \qmlattachedproperty TextArea QtQuick.Controls::TextArea::flickable

    This property attaches a text area to a \l Flickable.

    \sa ScrollBar, ScrollIndicator, {Scrollable TextArea}
*/
QQuickTextArea *QQuickTextAreaAttached::flickable() const
{
    Q_D(const QQuickTextAreaAttached);
    return d->control;
}

void QQuickTextAreaAttached::setFlickable(QQuickTextArea *control)
{
    Q_D(QQuickTextAreaAttached);
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(parent());
    if (!flickable) {
        qmlWarning(parent()) << "TextArea must be attached to a Flickable";
        return;
    }

    if (d->control == control)
        return;

    if (d->control)
        QQuickTextAreaPrivate::get(d->control)->detachFlickable();

    d->control = control;

    if (control)
        QQuickTextAreaPrivate::get(control)->attachFlickable(flickable);

    emit flickableChanged();
}

QT_END_NAMESPACE

#include "moc_qquicktextarea_p.cpp"
