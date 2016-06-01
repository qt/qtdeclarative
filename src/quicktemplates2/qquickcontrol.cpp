/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickcontrol_p.h"
#include "qquickcontrol_p_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include "qquicklabel_p.h"
#include "qquicklabel_p_p.h"
#include "qquicktextarea_p.h"
#include "qquicktextarea_p_p.h"
#include "qquicktextfield_p.h"
#include "qquicktextfield_p_p.h"
#include "qquickpopup_p.h"
#include "qquickpopup_p_p.h"
#include "qquickapplicationwindow_p.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#ifndef QT_NO_ACCESSIBILITY
#include <QtQuick/private/qquickaccessibleattached_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype Control
    \inherits Item
    \instantiates QQuickControl
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \brief The base type of user interface controls.

    Control is the base type of user interface controls.  It receives input
    events from the window system, and paints a representation of itself on
    the screen.

    \image qtquickcontrols2-control.png
*/

static bool isKeyFocusReason(Qt::FocusReason reason)
{
    return reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason || reason == Qt::ShortcutFocusReason;
}

QQuickControlPrivate::ExtraData::ExtraData()
{
}

QQuickControlPrivate::QQuickControlPrivate() :
    hasTopPadding(false), hasLeftPadding(false), hasRightPadding(false), hasBottomPadding(false), hasLocale(false), hovered(false), wheelEnabled(false),
    padding(0), topPadding(0), leftPadding(0), rightPadding(0), bottomPadding(0), spacing(0),
    focusPolicy(Qt::NoFocus), focusReason(Qt::OtherFocusReason),
    background(nullptr), contentItem(nullptr), accessibleAttached(nullptr)
{
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installActivationObserver(this);
#endif
}

QQuickControlPrivate::~QQuickControlPrivate()
{
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::removeActivationObserver(this);
#endif
}

void QQuickControlPrivate::mirrorChange()
{
    Q_Q(QQuickControl);
    if (locale.textDirection() == Qt::LeftToRight)
        q->mirrorChange();
}

void QQuickControlPrivate::setTopPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    qreal oldPadding = q->topPadding();
    topPadding = value;
    hasTopPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->topPaddingChanged();
        emit q->availableHeightChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(leftPadding, oldPadding, rightPadding, bottomPadding));
    }
}

void QQuickControlPrivate::setLeftPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    qreal oldPadding = q->leftPadding();
    leftPadding = value;
    hasLeftPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->leftPaddingChanged();
        emit q->availableWidthChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(oldPadding, topPadding, rightPadding, bottomPadding));
    }
}

void QQuickControlPrivate::setRightPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    qreal oldPadding = q->rightPadding();
    rightPadding = value;
    hasRightPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->rightPaddingChanged();
        emit q->availableWidthChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(leftPadding, topPadding, oldPadding, bottomPadding));
    }
}

void QQuickControlPrivate::setBottomPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    qreal oldPadding = q->bottomPadding();
    bottomPadding = value;
    hasBottomPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->bottomPaddingChanged();
        emit q->availableHeightChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(leftPadding, topPadding, rightPadding, oldPadding));
    }
}

void QQuickControlPrivate::resizeBackground()
{
    Q_Q(QQuickControl);
    if (background) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(background);
        if (!p->widthValid && qFuzzyIsNull(background->x())) {
            background->setWidth(q->width());
            p->widthValid = false;
        }
        if (!p->heightValid && qFuzzyIsNull(background->y())) {
            background->setHeight(q->height());
            p->heightValid = false;
        }
    }
}

void QQuickControlPrivate::resizeContent()
{
    Q_Q(QQuickControl);
    if (contentItem) {
        contentItem->setPosition(QPointF(q->leftPadding(), q->topPadding()));
        contentItem->setSize(QSizeF(q->availableWidth(), q->availableHeight()));
    }
}

#ifndef QT_NO_ACCESSIBILITY
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

QAccessible::Role QQuickControl::accessibleRole() const
{
    return QAccessible::NoRole;
}

void QQuickControl::accessibilityActiveChanged(bool active)
{
    Q_D(QQuickControl);
    if (d->accessibleAttached || !active)
        return;

    d->accessibleAttached = qobject_cast<QQuickAccessibleAttached *>(qmlAttachedPropertiesObject<QQuickAccessibleAttached>(this, true));

    // QQuickControl relies on the existence of a QQuickAccessibleAttached object.
    // However, qmlAttachedPropertiesObject(create=true) creates an instance only
    // for items that have been created by a QML engine. Therefore we create the
    // object by hand for items created in C++ (QQuickPopupItem, for instance).
    if (!d->accessibleAttached)
        d->accessibleAttached = new QQuickAccessibleAttached(this);

    d->accessibleAttached->setRole(accessibleRole());
}
#endif

/*!
    \internal

    Returns the font that the control w inherits from its ancestors and
    QGuiApplication::font.
*/
QFont QQuickControlPrivate::parentFont(const QQuickItem *item)
{
    QQuickItem *p = item->parentItem();
    while (p) {
        if (QQuickControl *control = qobject_cast<QQuickControl *>(p))
            return control->font();
        else if (QQuickLabel *label = qobject_cast<QQuickLabel *>(p))
            return label->font();
        else if (QQuickTextField *textField = qobject_cast<QQuickTextField *>(p))
            return textField->font();
        else if (QQuickTextArea *textArea = qobject_cast<QQuickTextArea *>(p))
            return textArea->font();

        p = p->parentItem();
    }

    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(item->window()))
        return window->font();

    return themeFont(QPlatformTheme::SystemFont);
}

QFont QQuickControlPrivate::themeFont(QPlatformTheme::Font type)
{
    if (QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        if (const QFont *font = theme->font(type)) {
            QFont f = *font;
            if (type == QPlatformTheme::SystemFont)
                f.resolve(0);
            return f;
        }
    }

    return QFont();
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

void QQuickControlPrivate::inheritFont(const QFont &f)
{
    Q_Q(QQuickControl);
    QFont parentFont = extra.isAllocated() ? extra->font.resolve(f) : f;
    parentFont.resolve(extra.isAllocated() ? extra->font.resolve() | f.resolve() : f.resolve());

    const QFont defaultFont = q->defaultFont();
    const QFont resolvedFont = parentFont.resolve(defaultFont);

    setFont_helper(resolvedFont);
}

/*!
    \internal

    Assign \a font to this control, and propagate it to all children.
*/
void QQuickControlPrivate::updateFont(const QFont &f)
{
    Q_Q(QQuickControl);
    QFont old = resolvedFont;
    resolvedFont = f;

    if (old != f)
        q->fontChange(f, old);

    QQuickControlPrivate::updateFontRecur(q, f);

    if (old != f)
        emit q->fontChanged();
}

void QQuickControlPrivate::updateFontRecur(QQuickItem *item, const QFont &f)
{
    const auto childItems = item->childItems();
    for (QQuickItem *child : childItems) {
        if (QQuickControl *control = qobject_cast<QQuickControl *>(child))
            QQuickControlPrivate::get(control)->inheritFont(f);
        else if (QQuickLabel *label = qobject_cast<QQuickLabel *>(child))
            QQuickLabelPrivate::get(label)->inheritFont(f);
        else if (QQuickTextArea *textArea = qobject_cast<QQuickTextArea *>(child))
            QQuickTextAreaPrivate::get(textArea)->inheritFont(f);
        else if (QQuickTextField *textField = qobject_cast<QQuickTextField *>(child))
            QQuickTextFieldPrivate::get(textField)->inheritFont(f);
        else
            QQuickControlPrivate::updateFontRecur(child, f);
    }
}

QString QQuickControl::accessibleName() const
{
#ifndef QT_NO_ACCESSIBILITY
    Q_D(const QQuickControl);
    if (d->accessibleAttached)
        return d->accessibleAttached->name();
#endif
    return QString();
}

void QQuickControl::setAccessibleName(const QString &name)
{
#ifndef QT_NO_ACCESSIBILITY
    Q_D(QQuickControl);
    if (d->accessibleAttached)
        d->accessibleAttached->setName(name);
#else
    Q_UNUSED(name)
#endif
}

QVariant QQuickControl::accessibleProperty(const char *propertyName)
{
#ifndef QT_NO_ACCESSIBILITY
    Q_D(QQuickControl);
    if (d->accessibleAttached)
        return QQuickAccessibleAttached::property(this, propertyName);
#endif
    Q_UNUSED(propertyName)
    return QVariant();
}

bool QQuickControl::setAccessibleProperty(const char *propertyName, const QVariant &value)
{
#ifndef QT_NO_ACCESSIBILITY
    Q_D(QQuickControl);
    if (d->accessibleAttached)
        return QQuickAccessibleAttached::setProperty(this, propertyName, value);
#endif
    Q_UNUSED(propertyName)
    Q_UNUSED(value)
    return false;
}

QQuickControl::QQuickControl(QQuickItem *parent) :
    QQuickItem(*(new QQuickControlPrivate), parent)
{
}

QQuickControl::QQuickControl(QQuickControlPrivate &dd, QQuickItem *parent) :
    QQuickItem(dd, parent)
{
}

void QQuickControl::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_D(QQuickControl);
    QQuickItem::itemChange(change, value);
    switch (change) {
    case ItemParentHasChanged:
        if (value.item) {
            d->resolveFont();
            if (!d->hasLocale)
                d->updateLocale(QQuickControlPrivate::calcLocale(d->parentItem), false); // explicit=false
        }
        break;
    case ItemActiveFocusHasChanged:
        if (isKeyFocusReason(d->focusReason))
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
    certain types of controls. You can also set the default font for controls by passing a custom
    font to QGuiApplication::setFont(), before loading the QML. Finally, the font is matched
    against Qt's font database to find the best match.

    Control propagates explicit font properties from parent to children. If you change a specific
    property on a control's font, that property propagates to all of the control's children,
    overriding any system defaults for that property.
*/
QFont QQuickControl::font() const
{
    Q_D(const QQuickControl);
    return d->resolvedFont;
}

void QQuickControl::setFont(const QFont &font)
{
    Q_D(QQuickControl);
    if (d->extra.value().font.resolve() == font.resolve() && d->extra.value().font == font)
        return;

    d->extra.value().font = font;
    d->resolveFont();
}

void QQuickControl::resetFont()
{
    setFont(QFont());
}

/*!
    \qmlproperty real QtQuick.Controls::Control::availableWidth
    \readonly

    This property holds the width available after deducting horizontal padding.

    \sa padding, leftPadding, rightPadding
*/
qreal QQuickControl::availableWidth() const
{
    return qMax<qreal>(0.0, width() - leftPadding() - rightPadding());
}

/*!
    \qmlproperty real QtQuick.Controls::Control::availableHeight
    \readonly

    This property holds the height available after deducting vertical padding.

    \sa padding, topPadding, bottomPadding
*/
qreal QQuickControl::availableHeight() const
{
    return qMax<qreal>(0.0, height() - topPadding() - bottomPadding());
}

/*!
    \qmlproperty real QtQuick.Controls::Control::padding

    This property holds the default padding.

    \sa availableWidth, availableHeight, topPadding, leftPadding, rightPadding, bottomPadding
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
    QMarginsF oldPadding(leftPadding(), topPadding(), rightPadding(), bottomPadding());
    d->padding = padding;
    emit paddingChanged();
    QMarginsF newPadding(leftPadding(), topPadding(), rightPadding(), bottomPadding());
    if (!qFuzzyCompare(newPadding.top(), oldPadding.top()))
        emit topPaddingChanged();
    if (!qFuzzyCompare(newPadding.left(), oldPadding.left()))
        emit leftPaddingChanged();
    if (!qFuzzyCompare(newPadding.right(), oldPadding.right()))
        emit rightPaddingChanged();
    if (!qFuzzyCompare(newPadding.bottom(), oldPadding.bottom()))
        emit bottomPaddingChanged();
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

    This property holds the top padding.

    \sa padding, bottomPadding, availableHeight
*/
qreal QQuickControl::topPadding() const
{
    Q_D(const QQuickControl);
    if (d->hasTopPadding)
        return d->topPadding;
    return d->padding;
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

    This property holds the left padding.

    \sa padding, rightPadding, availableWidth
*/
qreal QQuickControl::leftPadding() const
{
    Q_D(const QQuickControl);
    if (d->hasLeftPadding)
        return d->leftPadding;
    return d->padding;
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

    This property holds the right padding.

    \sa padding, leftPadding, availableWidth
*/
qreal QQuickControl::rightPadding() const
{
    Q_D(const QQuickControl);
    if (d->hasRightPadding)
        return d->rightPadding;
    return d->padding;
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

    This property holds the bottom padding.

    \sa padding, topPadding, availableHeight
*/
qreal QQuickControl::bottomPadding() const
{
    Q_D(const QQuickControl);
    if (d->hasBottomPadding)
        return d->bottomPadding;
    return d->padding;
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
*/
qreal QQuickControl::spacing() const
{
    Q_D(const QQuickControl);
    return d->spacing;
}

void QQuickControl::setSpacing(qreal spacing)
{
    Q_D(QQuickControl);
    if (!qFuzzyCompare(d->spacing, spacing)) {
        d->spacing = spacing;
        emit spacingChanged();
    }
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

    Control propagates explicit locale properties from parent to children.
    If you change a specific property on a control's locale, that property
    propagates to all of the control's children, overriding any system defaults
    for that property.

    \sa mirrored, {LayoutMirroring}{LayoutMirroring}
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

QLocale QQuickControlPrivate::calcLocale(const QQuickItem *item)
{
    const QQuickItem *p = item;
    while (p) {
        if (const QQuickControl *control = qobject_cast<const QQuickControl *>(p))
            return control->locale();

        QVariant v = p->property("locale");
        if (v.isValid() && v.userType() == QMetaType::QLocale)
            return v.toLocale();

        p = p->parentItem();
    }

    if (item) {
        if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(item->window()))
            return window->locale();
    }

    return QLocale();
}

void QQuickControlPrivate::updateLocale(const QLocale &l, bool e)
{
    Q_Q(QQuickControl);
    if (!e && hasLocale)
        return;

    QLocale old = q->locale();
    hasLocale = e;
    if (old != l) {
        bool wasMirrored = q->isMirrored();
        q->localeChange(l, old);
        locale = l;
        QQuickControlPrivate::updateLocaleRecur(q, l);
        emit q->localeChanged();
        if (wasMirrored != q->isMirrored())
            q->mirrorChange();
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

/*!
    \qmlproperty bool QtQuick.Controls::Control::mirrored
    \readonly

    This property holds whether the control is mirrored.

    This property is provided for convenience. A control is considered mirrored
    when its visual layout direction is right-to-left.

    \sa locale, {LayoutMirroring}{LayoutMirroring}
*/
bool QQuickControl::isMirrored() const
{
    Q_D(const QQuickControl);
    return d->isMirrored() || d->locale.textDirection() == Qt::RightToLeft;
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Control::focusPolicy

    This property determines the way the control accepts focus.

    \value Qt.TabFocus    The control accepts focus by tabbing.
    \value Qt.ClickFocus  The control accepts focus by clicking.
    \value Qt.StrongFocus The control accepts focus by both tabbing and clicking.
    \value Qt.WheelFocus  The control accepts focus by tabbing, clicking, and using the mouse wheel.
    \value Qt.NoFocus     The control does not accept focus.
*/
Qt::FocusPolicy QQuickControl::focusPolicy() const
{
    Q_D(const QQuickControl);
    uint policy = d->focusPolicy;
    if (activeFocusOnTab())
        policy |= Qt::TabFocus;
    return static_cast<Qt::FocusPolicy>(policy);
}

void QQuickControl::setFocusPolicy(Qt::FocusPolicy policy)
{
    Q_D(QQuickControl);
    if (d->focusPolicy == policy)
        return;

    d->focusPolicy = policy;
    setActiveFocusOnTab(policy & Qt::TabFocus);
    emit focusPolicyChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Control::focusReason
    \readonly

    This property holds the reason of the last focus change.

    \note This property does not indicate whether the control has \l {Item::activeFocus}
          {active focus}, but the reason why the control either gained or lost focus.

    \value Qt.MouseFocusReason         A mouse action occurred.
    \value Qt.TabFocusReason           The Tab key was pressed.
    \value Qt.BacktabFocusReason       A Backtab occurred. The input for this may include the Shift or Control keys; e.g. Shift+Tab.
    \value Qt.ActiveWindowFocusReason  The window system made this window either active or inactive.
    \value Qt.PopupFocusReason         The application opened/closed a pop-up that grabbed/released the keyboard focus.
    \value Qt.ShortcutFocusReason      The user typed a label's buddy shortcut
    \value Qt.MenuBarFocusReason       The menu bar took focus.
    \value Qt.OtherFocusReason         Another reason, usually application-specific.

    \sa visualFocus, Item::activeFocus
*/
Qt::FocusReason QQuickControl::focusReason() const
{
    Q_D(const QQuickControl);
    return d->focusReason;
}

void QQuickControl::setFocusReason(Qt::FocusReason reason)
{
    Q_D(QQuickControl);
    if (d->focusReason == reason)
        return;

    Qt::FocusReason oldReason = d->focusReason;
    d->focusReason = reason;
    emit focusReasonChanged();
    if (d->activeFocus && isKeyFocusReason(oldReason) != isKeyFocusReason(reason))
        emit visualFocusChanged();
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
    return d->activeFocus && isKeyFocusReason(d->focusReason);
}

/*!
    \qmlproperty bool QtQuick.Controls::Control::hovered
    \readonly

    This property holds whether the control is hovered.

    \sa hoverEnabled
*/
bool QQuickControl::isHovered() const
{
    Q_D(const QQuickControl);
    return d->hovered;
}

void QQuickControl::setHovered(bool hovered)
{
    Q_D(QQuickControl);
    if (hovered == d->hovered)
        return;

    d->hovered = hovered;
    emit hoveredChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Control::hoverEnabled

    This property determines whether the control accepts hover events. The default value is \c false.

    \sa hovered
*/
bool QQuickControl::isHoverEnabled() const
{
    Q_D(const QQuickControl);
    return d->hoverEnabled;
}

void QQuickControl::setHoverEnabled(bool enabled)
{
    Q_D(QQuickControl);
    if (enabled == d->hoverEnabled)
        return;

    setAcceptHoverEvents(enabled);
    emit hoverEnabledChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Control::wheelEnabled

    This property determines whether the control handles wheel events. The default value is \c false.
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

    \note If the background item has no explicit size specified, it automatically
          follows the control's size. In most cases, there is no need to specify
          width or height for a background item.
*/
QQuickItem *QQuickControl::background() const
{
    Q_D(const QQuickControl);
    return d->background;
}

void QQuickControl::setBackground(QQuickItem *background)
{
    Q_D(QQuickControl);
    if (d->background == background)
        return;

    delete d->background;
    d->background = background;
    if (background) {
        background->setParentItem(this);
        if (qFuzzyIsNull(background->z()))
            background->setZ(-1);
        if (isComponentComplete())
            d->resizeBackground();
    }
    emit backgroundChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::Control::contentItem

    This property holds the visual content item.

    \note The content item is automatically resized inside the \l padding of the control.
*/
QQuickItem *QQuickControl::contentItem() const
{
    Q_D(const QQuickControl);
    return d->contentItem;
}

void QQuickControl::setContentItem(QQuickItem *item)
{
    Q_D(QQuickControl);
    if (d->contentItem == item)
        return;

    contentItemChange(item, d->contentItem);
    delete d->contentItem;
    d->contentItem = item;
    if (item) {
        if (!item->parentItem())
            item->setParentItem(this);
        if (isComponentComplete())
            d->resizeContent();
    }
    emit contentItemChanged();
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
    QQuickItem::componentComplete();
    if (!d->hasLocale)
        d->locale = QQuickControlPrivate::calcLocale(d->parentItem);
#ifndef QT_NO_ACCESSIBILITY
    if (!d->accessibleAttached && QAccessible::isActive())
        accessibilityActiveChanged(true);
#endif
}

QFont QQuickControl::defaultFont() const
{
    return QQuickControlPrivate::themeFont(QPlatformTheme::SystemFont);
}

void QQuickControl::focusInEvent(QFocusEvent *event)
{
    QQuickItem::focusInEvent(event);
    setFocusReason(event->reason());
}

void QQuickControl::focusOutEvent(QFocusEvent *event)
{
    QQuickItem::focusOutEvent(event);
    setFocusReason(event->reason());
}

void QQuickControl::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(QQuickControl);
    setHovered(d->hoverEnabled);
    event->setAccepted(d->hoverEnabled);
}

void QQuickControl::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(QQuickControl);
    setHovered(false);
    event->setAccepted(d->hoverEnabled);
}

void QQuickControl::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickControl);
    if ((d->focusPolicy & Qt::ClickFocus) == Qt::ClickFocus && !QGuiApplication::styleHints()->setFocusOnTouchRelease())
        forceActiveFocus(Qt::MouseFocusReason);

    event->accept();
}

void QQuickControl::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
}

void QQuickControl::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickControl);
    if ((d->focusPolicy & Qt::ClickFocus) == Qt::ClickFocus && QGuiApplication::styleHints()->setFocusOnTouchRelease())
        forceActiveFocus(Qt::MouseFocusReason);

    event->accept();
}

void QQuickControl::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickControl);
    if ((d->focusPolicy & Qt::WheelFocus) == Qt::WheelFocus)
        forceActiveFocus(Qt::MouseFocusReason);

    event->setAccepted(d->wheelEnabled);
}

void QQuickControl::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickControl);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    d->resizeBackground();
    d->resizeContent();
    if (!qFuzzyCompare(newGeometry.width(), oldGeometry.width()))
        emit availableWidthChanged();
    if (!qFuzzyCompare(newGeometry.height(), oldGeometry.height()))
        emit availableHeightChanged();
}

void QQuickControl::fontChange(const QFont &newFont, const QFont &oldFont)
{
    Q_UNUSED(newFont);
    Q_UNUSED(oldFont);
}

void QQuickControl::mirrorChange()
{
    emit mirroredChanged();
}

void QQuickControl::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickControl);
    Q_UNUSED(newPadding);
    Q_UNUSED(oldPadding);
    d->resizeContent();
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

QT_END_NAMESPACE
