/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include "qquicktextarea_p.h"
#include "qquicktextarea_p_p.h"
#include "qquickcontrol_p.h"
#include "qquickcontrol_p_p.h"

#include <QtGui/qguiapplication.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickclipnode_p.h>

#ifndef QT_NO_ACCESSIBILITY
#include <QtQuick/private/qquickaccessibleattached_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype TextArea
    \inherits TextEdit
    \instantiates QQuickTextArea
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-input
    \brief A multi line text input control.

    TextArea is a multi-line text editor. TextArea extends TextEdit with
    a \l {placeholderText}{placeholder text} functionality, and adds decoration.

    \code
    TextArea {
        placeholderText: qsTr("Enter description")
    }
    \endcode

    \labs

    \sa TextField, {Customizing TextArea}, {Input Controls}
*/

/*!
    \qmlsignal Qt.labs.controls::TextArea::pressAndHold(MouseEvent mouse)

    This signal is emitted when there is a long press (the delay depends on the platform plugin).
    The \l {MouseEvent}{mouse} parameter provides information about the press, including the x and y
    position of the press, and which button is pressed.
*/

QQuickTextAreaPrivate::QQuickTextAreaPrivate()
    : background(Q_NULLPTR), focusReason(Qt::OtherFocusReason), accessibleAttached(Q_NULLPTR)
{
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installActivationObserver(this);
#endif
}

QQuickTextAreaPrivate::~QQuickTextAreaPrivate()
{
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::removeActivationObserver(this);
#endif
}

void QQuickTextAreaPrivate::resizeBackground()
{
    Q_Q(QQuickTextArea);
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
    emit q->implicitWidthChanged();
}

void QQuickTextAreaPrivate::implicitHeightChanged()
{
    Q_Q(QQuickTextArea);
    QQuickItemPrivate::implicitHeightChanged();
    emit q->implicitHeightChanged();
}

QQuickTextArea::QQuickTextArea(QQuickItem *parent) :
    QQuickTextEdit(*(new QQuickTextAreaPrivate), parent)
{
    Q_D(QQuickTextArea);
    setActiveFocusOnTab(true);
    d->setImplicitResizeEnabled(false);
    d->pressAndHoldHelper.control = this;
    QObjectPrivate::connect(this, &QQuickTextEdit::readOnlyChanged,
                            d, &QQuickTextAreaPrivate::_q_readOnlyChanged);
}

QQuickTextArea::~QQuickTextArea()
{
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
    inheritFont(QQuickControlPrivate::naturalControlFont(q));
}

void QQuickTextAreaPrivate::inheritFont(const QFont &f)
{
    Q_Q(QQuickTextArea);
    QFont parentFont = font.resolve(f);
    parentFont.resolve(font.resolve() | f.resolve());

    const QFont defaultFont = QQuickControlPrivate::themeFont(QPlatformTheme::SystemFont);
    const QFont resolvedFont = parentFont.resolve(defaultFont);

    const bool changed = resolvedFont != sourceFont;
    q->QQuickTextEdit::setFont(resolvedFont);
    if (changed)
        emit q->fontChanged();
}

void QQuickTextAreaPrivate::_q_readOnlyChanged(bool isReadOnly)
{
#ifndef QT_NO_ACCESSIBILITY
    if (accessibleAttached)
        accessibleAttached->set_readOnly(isReadOnly);
#else
    Q_UNUSED(isReadOnly)
#endif
}

#ifndef QT_NO_ACCESSIBILITY
void QQuickTextAreaPrivate::accessibilityActiveChanged(bool active)
{
    if (accessibleAttached || !active)
        return;

    Q_Q(QQuickTextArea);
    accessibleAttached = qobject_cast<QQuickAccessibleAttached *>(qmlAttachedPropertiesObject<QQuickAccessibleAttached>(q, true));
    if (accessibleAttached) {
        accessibleAttached->setRole(accessibleRole());
        accessibleAttached->set_readOnly(q->isReadOnly());
        accessibleAttached->setDescription(placeholder);
    } else {
        qWarning() << "QQuickTextArea: " << q << " QQuickAccessibleAttached object creation failed!";
    }
}

QAccessible::Role QQuickTextAreaPrivate::accessibleRole() const
{
    return QAccessible::EditableText;
}
#endif

QFont QQuickTextArea::font() const
{
    return QQuickTextEdit::font();
}

void QQuickTextArea::setFont(const QFont &font)
{
    Q_D(QQuickTextArea);
    if (d->font.resolve() == font.resolve() && d->font == font)
        return;

    d->font = font;
    d->resolveFont();
}

/*!
    \qmlproperty Item Qt.labs.controls::TextArea::background

    This property holds the background item.

    \note If the background item has no explicit size specified, it automatically
          follows the control's size. In most cases, there is no need to specify
          width or height for a background item.

    \sa {Customizing TextArea}
*/
QQuickItem *QQuickTextArea::background() const
{
    Q_D(const QQuickTextArea);
    return d->background;
}

void QQuickTextArea::setBackground(QQuickItem *background)
{
    Q_D(QQuickTextArea);
    if (d->background != background) {
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
}

/*!
    \qmlproperty string Qt.labs.controls::TextArea::placeholderText

    This property holds the placeholder text.
*/
QString QQuickTextArea::placeholderText() const
{
    Q_D(const QQuickTextArea);
    return d->placeholder;
}

void QQuickTextArea::setPlaceholderText(const QString &text)
{
    Q_D(QQuickTextArea);
    if (d->placeholder != text) {
        d->placeholder = text;
#ifndef QT_NO_ACCESSIBILITY
        if (d->accessibleAttached)
            d->accessibleAttached->setDescription(text);
#endif
        emit placeholderTextChanged();
    }
}

/*!
    \qmlproperty enumeration Qt.labs.controls::TextArea::focusReason

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

    \sa Item::activeFocus
*/
Qt::FocusReason QQuickTextArea::focusReason() const
{
    Q_D(const QQuickTextArea);
    return d->focusReason;
}

void QQuickTextArea::setFocusReason(Qt::FocusReason reason)
{
    Q_D(QQuickTextArea);
    if (d->focusReason != reason) {
        d->focusReason = reason;
        emit focusReasonChanged();
    }
}

void QQuickTextArea::classBegin()
{
    Q_D(QQuickTextArea);
    QQuickTextEdit::classBegin();
    d->resolveFont();
}

void QQuickTextArea::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_D(QQuickTextArea);
    QQuickTextEdit::itemChange(change, value);
    if (change == ItemParentHasChanged && value.item)
        d->resolveFont();
}

void QQuickTextArea::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTextArea);
    QQuickTextEdit::geometryChanged(newGeometry, oldGeometry);
    d->resizeBackground();
}

QSGNode *QQuickTextArea::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    QQuickDefaultClipNode *clipNode = static_cast<QQuickDefaultClipNode *>(oldNode);
    if (!clipNode)
        clipNode = new QQuickDefaultClipNode(QRectF());

    clipNode->setRect(clipRect().adjusted(leftPadding(), topPadding(), -rightPadding(), -bottomPadding()));
    clipNode->update();

    QSGNode *textNode = QQuickTextEdit::updatePaintNode(clipNode->firstChild(), data);
    if (!textNode->parent())
        clipNode->appendChildNode(textNode);

    return clipNode;
}

void QQuickTextArea::focusInEvent(QFocusEvent *event)
{
    QQuickTextEdit::focusInEvent(event);
    setFocusReason(event->reason());
}

void QQuickTextArea::focusOutEvent(QFocusEvent *event)
{
    QQuickTextEdit::focusOutEvent(event);
    setFocusReason(event->reason());
}

void QQuickTextArea::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickTextArea);
    d->pressAndHoldHelper.mousePressEvent(event);
    if (d->pressAndHoldHelper.isActive()) {
        if (d->pressAndHoldHelper.delayedMousePressEvent) {
            QQuickTextEdit::mousePressEvent(d->pressAndHoldHelper.delayedMousePressEvent);
            d->pressAndHoldHelper.clearDelayedMouseEvent();
        }
        QQuickTextEdit::mousePressEvent(event);
    }
}

void QQuickTextArea::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickTextArea);
    d->pressAndHoldHelper.mouseMoveEvent(event);
    if (d->pressAndHoldHelper.isActive()) {
        if (d->pressAndHoldHelper.delayedMousePressEvent) {
            QQuickTextEdit::mousePressEvent(d->pressAndHoldHelper.delayedMousePressEvent);
            d->pressAndHoldHelper.clearDelayedMouseEvent();
        }
        QQuickTextEdit::mouseMoveEvent(event);
    }
}

void QQuickTextArea::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickTextArea);
    d->pressAndHoldHelper.mouseReleaseEvent(event);
    if (d->pressAndHoldHelper.isActive()) {
        if (d->pressAndHoldHelper.delayedMousePressEvent) {
            QQuickTextEdit::mousePressEvent(d->pressAndHoldHelper.delayedMousePressEvent);
            d->pressAndHoldHelper.clearDelayedMouseEvent();
        }
        QQuickTextEdit::mouseReleaseEvent(event);
    }
}

void QQuickTextArea::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickTextArea);
    if (event->timerId() == d->pressAndHoldHelper.timer.timerId()) {
        d->pressAndHoldHelper.timerEvent(event);
    } else {
        QQuickTextEdit::timerEvent(event);
    }
}

QT_END_NAMESPACE
