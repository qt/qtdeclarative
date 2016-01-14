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

#include "qquickpageindicator_p.h"
#include "qquickcontrol_p_p.h"

#include <QtCore/qmath.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PageIndicator
    \inherits Control
    \instantiates QQuickPageIndicator
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-indicators
    \brief A page indicator control.

    PageIndicator is used to indicate the currently active page
    in a container of multiple pages. PageIndicator consists of
    delegate items that present pages.

    \image qtlabscontrols-pageindicator.png

    \snippet qtlabscontrols-pageindicator.qml 1

    \labs

    \sa SwipeView, {Customizing PageIndicator}, {Indicator Controls}
*/

class QQuickPageIndicatorPrivate : public QQuickControlPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickPageIndicator)

public:
    QQuickPageIndicatorPrivate() : count(0), currentIndex(0),
        interactive(false), delegate(Q_NULLPTR), pressedItem(Q_NULLPTR)
    {
    }

    QQuickItem *itemAt(const QPoint &pos) const;
    void updatePressed(bool pressed, const QPoint &pos = QPoint());
    void setContextProperty(QQuickItem *item, const QString &name, const QVariant &value);

    void itemChildAdded(QQuickItem *, QQuickItem *child);

    int count;
    int currentIndex;
    bool interactive;
    QQmlComponent *delegate;
    QQuickItem *pressedItem;
};

QQuickItem *QQuickPageIndicatorPrivate::itemAt(const QPoint &pos) const
{
    Q_Q(const QQuickPageIndicator);
    if (!contentItem || !q->contains(pos))
        return Q_NULLPTR;

    QPointF contentPos = q->mapToItem(contentItem, pos);
    QQuickItem *item = contentItem->childAt(contentPos.x(), contentPos.y());
    while (item && item->parentItem() != contentItem)
        item = item->parentItem();
    if (item && !QQuickItemPrivate::get(item)->isTransparentForPositioner())
        return item;

    // find the nearest
    qreal distance = qInf();
    QQuickItem *nearest = Q_NULLPTR;
    foreach (QQuickItem *child, contentItem->childItems()) {
        if (QQuickItemPrivate::get(child)->isTransparentForPositioner())
            continue;

        QPointF center = child->boundingRect().center();
        QPointF pt = contentItem->mapToItem(child, contentPos);

        qreal len = QLineF(center, pt).length();
        if (len < distance) {
            distance = len;
            nearest = child;
        }
    }
    return nearest;
}

void QQuickPageIndicatorPrivate::updatePressed(bool pressed, const QPoint &pos)
{
    QQuickItem *prevItem = pressedItem;
    pressedItem = pressed ? itemAt(pos) : Q_NULLPTR;
    if (prevItem != pressedItem) {
        setContextProperty(prevItem, QStringLiteral("pressed"), false);
        setContextProperty(pressedItem, QStringLiteral("pressed"), pressed);
    }
}

void QQuickPageIndicatorPrivate::setContextProperty(QQuickItem *item, const QString &name, const QVariant &value)
{
    QQmlContext *context = qmlContext(item);
    if (context && context->isValid()) {
        context = context->parentContext();
        if (context && context->isValid())
            context->setContextProperty(name, value);
    }
}

void QQuickPageIndicatorPrivate::itemChildAdded(QQuickItem *, QQuickItem *child)
{
    if (!QQuickItemPrivate::get(child)->isTransparentForPositioner())
        setContextProperty(child, QStringLiteral("pressed"), false);
}

QQuickPageIndicator::QQuickPageIndicator(QQuickItem *parent) :
    QQuickControl(*(new QQuickPageIndicatorPrivate), parent)
{
}

/*!
    \qmlproperty int Qt.labs.controls::PageIndicator::count

    This property holds the number of pages.
*/
int QQuickPageIndicator::count() const
{
    Q_D(const QQuickPageIndicator);
    return d->count;
}

void QQuickPageIndicator::setCount(int count)
{
    Q_D(QQuickPageIndicator);
    if (d->count != count) {
        d->count = count;
        emit countChanged();
    }
}

/*!
    \qmlproperty int Qt.labs.controls::PageIndicator::currentIndex

    This property holds the index of the current page.
*/
int QQuickPageIndicator::currentIndex() const
{
    Q_D(const QQuickPageIndicator);
    return d->currentIndex;
}

void QQuickPageIndicator::setCurrentIndex(int index)
{
    Q_D(QQuickPageIndicator);
    if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}

/*!
    \qmlproperty bool Qt.labs.controls::PageIndicator::interactive

    This property holds whether the control is interactive. An interactive page indicator
    reacts to presses and automatically changes the \l {currentIndex}{current index}
    appropriately.

    The default value is \c false.
*/
bool QQuickPageIndicator::isInteractive() const
{
    Q_D(const QQuickPageIndicator);
    return d->interactive;
}

void QQuickPageIndicator::setInteractive(bool interactive)
{
    Q_D(QQuickPageIndicator);
    if (d->interactive != interactive) {
        d->interactive = interactive;
        setAcceptedMouseButtons(interactive ? Qt::LeftButton : Qt::NoButton);
        emit interactiveChanged();
    }
}

/*!
    \qmlproperty Component Qt.labs.controls::PageIndicator::delegate

    This property holds a delegate that presents a page.

    The following properties are available in the context of each delegate:
    \table
        \row \li \b index : int \li The index of the item
        \row \li \b pressed : bool \li Whether the item is pressed
    \endtable
*/
QQmlComponent *QQuickPageIndicator::delegate() const
{
    Q_D(const QQuickPageIndicator);
    return d->delegate;
}

void QQuickPageIndicator::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickPageIndicator);
    if (d->delegate != delegate) {
        d->delegate = delegate;
        emit delegateChanged();
    }
}

void QQuickPageIndicator::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickPageIndicator);
    QQuickControl::contentItemChange(newItem, oldItem);
    if (oldItem)
        QQuickItemPrivate::get(oldItem)->removeItemChangeListener(d, QQuickItemPrivate::Children);
    if (newItem)
        QQuickItemPrivate::get(newItem)->addItemChangeListener(d, QQuickItemPrivate::Children);
}

void QQuickPageIndicator::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickPageIndicator);
    if (d->interactive) {
        d->updatePressed(true, event->pos());
        event->accept();
    }
}

void QQuickPageIndicator::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickPageIndicator);
    if (d->interactive) {
        d->updatePressed(true, event->pos());
        event->accept();
    }
}

void QQuickPageIndicator::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickPageIndicator);
    if (d->interactive) {
        if (d->pressedItem)
            setCurrentIndex(d->contentItem->childItems().indexOf(d->pressedItem));
        d->updatePressed(false);
        event->accept();
    }
}

void QQuickPageIndicator::mouseUngrabEvent()
{
    Q_D(QQuickPageIndicator);
    if (d->interactive)
        d->updatePressed(false);
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickPageIndicator::accessibleRole() const
{
    return QAccessible::Indicator;
}
#endif

QT_END_NAMESPACE
