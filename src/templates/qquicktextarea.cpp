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

    TextArea is a multi-line text editor. TextArea extends TextEdit
    with a \l placeholder text functionality, and adds decoration.

    \code
    TextArea {
        placeholder.text: qsTr("Enter description")
    }
    \endcode

    \sa TextField, {Customizing TextArea}, {Input Controls}
*/

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
    QFont naturalFont = QQuickControlPrivate::naturalControlFont(q);
    QFont resolvedFont = sourceFont.resolve(naturalFont);
    if (sourceFont.resolve() == resolvedFont.resolve() && sourceFont == resolvedFont)
        return;

    q->QQuickTextEdit::setFont(resolvedFont);

    emit q->fontChanged();
}

void QQuickTextAreaPrivate::_q_readOnlyChanged(bool isReadOnly)
{
#ifndef QT_NO_ACCESSIBILITY
    Q_Q(QQuickTextArea);
    if (accessibleAttached)
        QQuickAccessibleAttached::setProperty(q, "readOnly", isReadOnly);
#else
    Q_UNUSED(isReadOnly)
#endif
}

void QQuickTextAreaPrivate::_q_placeholderTextChanged(const QString &text)
{
#ifndef QT_NO_ACCESSIBILITY
    if (accessibleAttached)
        accessibleAttached->setDescription(text);
#else
    Q_UNUSED(text)
#endif
}

QFont QQuickTextArea::font() const
{
    return QQuickTextEdit::font();
}

void QQuickTextArea::setFont(const QFont &font)
{
    Q_D(QQuickTextArea);
    if (d->sourceFont == font)
        return;

    // Determine which font is inherited from this control's ancestors and
    // QGuiApplication::font, resolve this against \a font (attributes from the
    // inherited font are copied over). Then propagate this font to this
    // control's children.
    QFont naturalFont = QQuickControlPrivate::naturalControlFont(this);
    QFont resolvedFont = font.resolve(naturalFont);
    if (d->sourceFont.resolve() == resolvedFont.resolve() && d->sourceFont == resolvedFont)
        return;

    QQuickTextEdit::setFont(font);

    emit fontChanged();
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
    \qmlproperty Text Qt.labs.controls::TextArea::placeholder

    This property holds the placeholder text item.

    \sa {Customizing TextArea}
*/
QQuickText *QQuickTextArea::placeholder() const
{
    Q_D(const QQuickTextArea);
    return d->placeholder;
}

void QQuickTextArea::setPlaceholder(QQuickText *placeholder)
{
    Q_D(QQuickTextArea);
    if (d->placeholder != placeholder) {
        if (d->placeholder) {
            QObjectPrivate::disconnect(d->placeholder, &QQuickText::textChanged,
                                       d, &QQuickTextAreaPrivate::_q_placeholderTextChanged);
            delete d->placeholder;
        }
        d->placeholder = placeholder;
        if (placeholder && !placeholder->parentItem()) {
            placeholder->setParentItem(this);
            QObjectPrivate::connect(d->placeholder, &QQuickText::textChanged,
                                    d, &QQuickTextAreaPrivate::_q_placeholderTextChanged);
        } else {
#ifndef QT_NO_ACCESSIBILITY
            if (d->accessibleAttached)
                d->accessibleAttached->setDescription(QLatin1Literal(""));
#endif
        }
        emit placeholderChanged();
    }
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

void QQuickTextArea::classBegin()
{
    QQuickTextEdit::classBegin();
#ifndef QT_NO_ACCESSIBILITY
    Q_D(QQuickTextArea);
    d->accessibleAttached = qobject_cast<QQuickAccessibleAttached *>(qmlAttachedPropertiesObject<QQuickAccessibleAttached>(this, true));
    if (d->accessibleAttached) {
        d->accessibleAttached->setRole((QAccessible::Role)(0x0000002A)); // Accessible.EditableText
        QQuickAccessibleAttached::setProperty(this, "multiLine", true);
    } else {
        qWarning() << "QQuickTextArea: QQuickAccessibleAttached object creation failed!";
    }
#endif
}

QT_END_NAMESPACE
