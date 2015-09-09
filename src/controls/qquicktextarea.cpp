/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

QT_BEGIN_NAMESPACE

/*!
    \qmltype TextArea
    \inherits TextEdit
    \instantiates QQuickTextArea
    \inqmlmodule QtQuick.Controls
    \ingroup editors
    \brief A multi line text input control.

    TextArea is a multi-line text editor. TextArea extends TextEdit
    with a \l placeholder text functionality, and adds decoration.

    \code
    TextArea {
        placeholder.text: qsTr("Enter description")
    }
    \endcode

    \sa TextField, {Customizing TextArea}
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

QQuickTextArea::QQuickTextArea(QQuickItem *parent) :
    QQuickTextEdit(*(new QQuickTextAreaPrivate), parent)
{
    setActiveFocusOnTab(true);
    d_func()->pressAndHoldHelper.control = this;
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
    Q_Q(const QQuickTextArea);
    QFont naturalFont = QQuickControlPrivate::naturalControlFont(q);
    QFont resolvedFont = sourceFont.resolve(naturalFont);
    setFont_helper(resolvedFont);
}

QFont QQuickTextArea::font() const
{
    Q_D(const QQuickTextArea);
    return d->sourceFont;
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
    d->setFont_helper(resolvedFont);

    QQuickTextEdit::setFont(font);

    emit fontChanged();
}

/*!
    \qmlproperty Item QtQuickControls2::TextArea::background

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
    \qmlproperty Text QtQuickControls2::TextArea::placeholder

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
        delete d->placeholder;
        d->placeholder = placeholder;
        if (placeholder && !placeholder->parentItem())
            placeholder->setParentItem(this);
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
    QQuickTextEdit::mousePressEvent(event);
}

void QQuickTextArea::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickTextArea);
    d->pressAndHoldHelper.mouseMoveEvent(event);
    if (!d->pressAndHoldHelper.timer.isActive())
        QQuickTextEdit::mouseMoveEvent(event);
}

void QQuickTextArea::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickTextArea);
    d->pressAndHoldHelper.mouseReleaseEvent(event);
    if (!d->pressAndHoldHelper.longPress)
        QQuickTextEdit::mouseReleaseEvent(event);
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
