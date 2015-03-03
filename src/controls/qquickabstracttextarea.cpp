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

#include "qquickabstracttextarea_p.h"
#include "qquickabstractapplicationwindow_p.h"
#include "qquickstyle_p_p.h"
#include "qquickstyle_p.h"

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickclipnode_p.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractTextAreaPrivate
{
    Q_DECLARE_PUBLIC(QQuickAbstractTextArea)

public:
    QQuickAbstractTextAreaPrivate() : hasStyle(false), style(Q_NULLPTR),
        background(Q_NULLPTR), placeholder(Q_NULLPTR) { }

    void resolveStyle(QQuickStyle *other = 0); // TODO

    bool hasStyle;
    QQuickStyle *style;
    QQuickItem *background;
    QQuickText *placeholder;
    QQuickAbstractTextArea *q_ptr;
};

void QQuickAbstractTextAreaPrivate::resolveStyle(QQuickStyle *res)
{
    Q_Q(QQuickAbstractTextArea);
    res = QQuickStylePrivate::resolve(q, res);
    if (style != res) {
        style = res;
        emit q->styleChanged();
    }
}

QQuickAbstractTextArea::QQuickAbstractTextArea(QQuickItem *parent) :
    QQuickTextEdit(parent), d_ptr(new QQuickAbstractTextAreaPrivate)
{
    Q_D(QQuickAbstractTextArea);
    d->q_ptr = this;
}

QQuickAbstractTextArea::~QQuickAbstractTextArea()
{
}

QQuickStyle *QQuickAbstractTextArea::style() const
{
    Q_D(const QQuickAbstractTextArea);
    if (!d->style)
        const_cast<QQuickAbstractTextArea *>(this)->d_func()->resolveStyle();
    return d->style;
}

void QQuickAbstractTextArea::setStyle(QQuickStyle *style)
{
    Q_D(QQuickAbstractTextArea);
    if (d->style != style) {
        d->hasStyle = true;
        d->resolveStyle(style);

        QEvent change(QEvent::StyleChange);
        foreach (QObject *object, findChildren<QObject *>()) {
            if (qobject_cast<QQuickStylable *>(object))
                QCoreApplication::sendEvent(object, &change);
        }
    }
}

bool QQuickAbstractTextArea::hasStyle() const
{
    Q_D(const QQuickAbstractTextArea);
    return d->hasStyle;
}

void QQuickAbstractTextArea::resetStyle()
{
    setStyle(Q_NULLPTR);
}

QQuickItem *QQuickAbstractTextArea::background() const
{
    Q_D(const QQuickAbstractTextArea);
    return d->background;
}

void QQuickAbstractTextArea::setBackground(QQuickItem *background)
{
    Q_D(QQuickAbstractTextArea);
    if (d->background != background) {
        delete d->background;
        d->background = background;
        if (background) {
            background->setParentItem(this);
            if (qFuzzyIsNull(background->z()))
                background->setZ(-1);
        }
        emit backgroundChanged();
    }
}

QQuickText *QQuickAbstractTextArea::placeholder() const
{
    Q_D(const QQuickAbstractTextArea);
    return d->placeholder;
}

void QQuickAbstractTextArea::setPlaceholder(QQuickText *placeholder)
{
    Q_D(QQuickAbstractTextArea);
    if (d->placeholder != placeholder) {
        delete d->placeholder;
        d->placeholder = placeholder;
        if (placeholder && !placeholder->parentItem())
            placeholder->setParentItem(this);
        emit placeholderChanged();
    }
}

bool QQuickAbstractTextArea::event(QEvent *event)
{
    Q_D(QQuickAbstractTextArea);
    if (event->type() == QEvent::StyleChange)
        d->resolveStyle();
    return QQuickTextEdit::event(event);
}

void QQuickAbstractTextArea::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickAbstractTextArea);
    QQuickTextEdit::itemChange(change, data);
    if (change == ItemSceneChange || change == ItemParentHasChanged)
        d->resolveStyle();
}

void QQuickAbstractTextArea::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickAbstractTextArea);
    QQuickTextEdit::geometryChanged(newGeometry, oldGeometry);
    if (d->background) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->background);
        if (!p->widthValid || qFuzzyCompare(d->background->width(), oldGeometry.width()))
            d->background->setWidth(newGeometry.width());
        if (!p->heightValid || qFuzzyCompare(d->background->height(), oldGeometry.height()))
            d->background->setHeight(newGeometry.height());
    }
}

QSGNode *QQuickAbstractTextArea::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
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

QT_END_NAMESPACE
