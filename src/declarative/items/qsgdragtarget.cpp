/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgdragtarget_p.h"
#include "qsgitem_p.h"
#include "qsgcanvas.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass DragEvent QSGDragEvent
    \brief The DragEvent object provides information about a drag event.

    The position of the drag event can be obtained from the \l x and \l
    properties, the \l keys property identifies the drag keys of the event
    source and the \l data property contains the payload of the drag event.
*/

/*!
    \qmlproperty real DragEvent::x

    This property holds the x coordinate of a drag event.
*/

/*!
    \qmlproperty real DragEvent::y

    This property holds the y coordinate of a drag event.
*/

/*!
    \qmlproperty stringlist DragEvent::keys

    This property holds a list of keys identifying the data type or source of a
    drag event.
*/

/*!
    \qmlproperty variant DragEvent::data

    This property holds data payload of a drag event.
*/

/*!
    \qmlproperty real DragEvent::accepted

    This property holds whether the drag event was accepted by a handler.

    The default value is true.
*/

class QSGDragTargetPrivate : public QSGItemPrivate
{
    Q_DECLARE_PUBLIC(QSGDragTarget)

public:
    QSGDragTargetPrivate();
    ~QSGDragTargetPrivate();

    bool hasMatchingKey(const QStringList &keys) const;

    QStringList keys;
    QRegExp keyRegExp;
    QVariant dragData;
    QPointF dragPosition;
    QSGItem *dropItem;
    bool containsDrag : 1;
};

QSGDragTargetPrivate::QSGDragTargetPrivate()
    : dropItem(0)
    , containsDrag(false)
{
}

QSGDragTargetPrivate::~QSGDragTargetPrivate()
{
}

/*!
    \qmlclass DragTarget QSGDragTarget
    \brief The DragTarget item provides drag and drop handling.

    A DragTarget is an invisible item which receives events when another item
    is dragged over it.

    A MouseArea item can be used to drag items.

    The \l keys property can be used to filter drag events which don't include
    a matching key.

    The \l dropItem property is communicated to the source of a drag event as
    the recipient of a drop on the drag target.

    The \l delegate property provides a means to specify a component to be
    instantiated for each active drag over a drag target.
*/

QSGDragTarget::QSGDragTarget(QSGItem *parent)
    : QSGItem(*new QSGDragTargetPrivate, parent)
{
}

QSGDragTarget::~QSGDragTarget()
{
}

/*!
    \qmlproperty bool DragTarget::containsDrag

    This property identifies whether the DragTarget currently contains any
    dragged items.
*/

bool QSGDragTarget::containsDrag() const
{
    Q_D(const QSGDragTarget);
    return d->containsDrag;
}

/*!
    \qmlproperty stringlist DragTarget::keys

    This property holds a list of drag keys a DragTarget will accept.
*/

QStringList QSGDragTarget::keys() const
{
    Q_D(const QSGDragTarget);
    return d->keys;
}

void QSGDragTarget::setKeys(const QStringList &keys)
{
    Q_D(QSGDragTarget);
    if (d->keys != keys) {
        d->keys = keys;

        if (keys.isEmpty()) {
            d->keyRegExp = QRegExp();
        } else {
            QString pattern = QLatin1Char('(') + QRegExp::escape(keys.first());
            for (int i = 1; i < keys.count(); ++i)
                pattern += QLatin1Char('|') + QRegExp::escape(keys.at(i));
            pattern += QLatin1Char(')');
            d->keyRegExp = QRegExp(pattern.replace(QLatin1String("\\*"), QLatin1String(".+")));
        }
        emit keysChanged();
    }
}

/*!
    \qmlproperty Item DragTarget::dropItem

    This property identifies an item as the recipient of a drop event within
    a DragTarget.

    \sa MouseArea::drag.dropItem
*/

QSGItem *QSGDragTarget::dropItem() const
{
    Q_D(const QSGDragTarget);
    return d->dropItem;
}

void QSGDragTarget::setDropItem(QSGItem *item)
{
    Q_D(QSGDragTarget);
    if (d->dropItem != item) {
        d->dropItem = item;
        emit dropItemChanged();
    }
}

void QSGDragTarget::resetDropItem()
{
    Q_D(QSGDragTarget);
    if (d->dropItem) {
        d->dropItem = 0;
        emit dropItemChanged();
    }
}

qreal QSGDragTarget::dragX() const
{
    Q_D(const QSGDragTarget);
    return d->dragPosition.x();
}

qreal QSGDragTarget::dragY() const
{
    Q_D(const QSGDragTarget);
    return d->dragPosition.y();
}

QVariant QSGDragTarget::dragData() const
{
    Q_D(const QSGDragTarget);
    return d->dragData;
}

/*!
    \qmlsignal DragTarget::onPositionChanged(DragEvent drag)
    \qmlattachedsignal DragTarget::onPositionChanged(DragEvent drag)

    This handler is called when the position of a drag has changed.
*/

void QSGDragTarget::dragMoveEvent(QSGDragEvent *event)
{
    Q_D(QSGDragTarget);
    if (!d->containsDrag) {
        event->setAccepted(false);
        return;
    }

    event->setDropItem(d->dropItem);

    d->dragPosition = event->position();
    emit dragPositionChanged();

    QSGDragTargetEvent dragTargetEvent(event);
    emit positionChanged(&dragTargetEvent);
}

bool QSGDragTargetPrivate::hasMatchingKey(const QStringList &keys) const
{
    if (keyRegExp.isEmpty())
        return true;

    foreach (const QString &key, keys) {
        if (keyRegExp.exactMatch(key))
            return true;
    }
    return false;
}

/*!
    \qmlsignal DragTarget::onEntered(DragEvent drag)
    \qmlattachedsignal DragTarget::onEntered(DragEvent drag)

    This handler is called when a drag enters the bounds of a DragTarget.
*/

void QSGDragTarget::dragEnterEvent(QSGDragEvent *event)
{
    Q_D(QSGDragTarget);
    if (!d->effectiveEnable || !d->hasMatchingKey(event->keys()) || d->containsDrag) {
        event->setAccepted(false);
        return;
    }

    event->setDropItem(d->dropItem);

    QSGDragTargetEvent dragTargetEvent(event);
    emit entered(&dragTargetEvent);

    if (event->isAccepted()) {

        d->dragData = event->data();
        d->containsDrag = true;
        if (!d->dragData.isNull())
            emit dragDataChanged();
        emit containsDragChanged();
    }
}

/*!
    \qmlsignal DragTarget::onExited(DragEvent drag)
    \qmlattachedsignal DragTarget::onExited(DragEvent drag)

    This handler is called when a drag exits the bounds of a DragTarget.
*/

void QSGDragTarget::dragExitEvent(QSGDragEvent *event)
{
    Q_D(QSGDragTarget);
    if (!d->containsDrag) {
        event->setAccepted(false);
        return;
    }

    QSGDragTargetEvent dragTargetEvent(event);
    emit exited(&dragTargetEvent);

    d->containsDrag = false;
    emit containsDragChanged();
    if (!d->dragData.isNull()) {
        d->dragData = QVariant();
        emit dragDataChanged();
    }
}

/*!
    \qmlsignal DragTarget::onDropped(DragEvent drag)
    \qmlattachedsignal DragTarget::onDropped(DragEvent drag)

    This handler is called when a drop event occurs within the bounds of a
    a DragTarget.
*/

void QSGDragTarget::dragDropEvent(QSGDragEvent *event)
{
    Q_D(QSGDragTarget);
    if (!d->containsDrag) {
        event->setAccepted(false);
        return;
    }

    event->setDropItem(d->dropItem);

    QSGDragTargetEvent dragTargetEvent(event);
    emit dropped(&dragTargetEvent);

    d->containsDrag = false;
    emit containsDragChanged();
    if (!d->dragData.isNull()) {
        d->dragData = QVariant();
        emit dragDataChanged();
    }
}

QT_END_NAMESPACE

