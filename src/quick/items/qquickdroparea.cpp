/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickdroparea_p.h"
#include "qquickdrag_p.h"
#include "qquickitem_p.h"
#include "qquickwindow.h"

#include <private/qqmlengine_p.h>

#ifndef QT_NO_DRAGANDDROP

QT_BEGIN_NAMESPACE

QQuickDropAreaDrag::QQuickDropAreaDrag(QQuickDropAreaPrivate *d, QObject *parent)
    : QObject(parent)
    , d(d)
{
}

QQuickDropAreaDrag::~QQuickDropAreaDrag()
{
}

class QQuickDropAreaPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickDropArea)

public:
    QQuickDropAreaPrivate();
    ~QQuickDropAreaPrivate();

    bool hasMatchingKey(const QStringList &keys) const;

    QStringList getKeys(const QMimeData *mimeData) const;

    QStringList keys;
    QRegExp keyRegExp;
    QPointF dragPosition;
    QQuickDropAreaDrag *drag;
    QQmlGuard<QObject> source;
    QQmlGuard<QMimeData> mimeData;
};

QQuickDropAreaPrivate::QQuickDropAreaPrivate()
    : drag(0)
{
}

QQuickDropAreaPrivate::~QQuickDropAreaPrivate()
{
    delete drag;
}

/*!
    \qmltype DropArea
    \instantiates QQuickDropArea
    \inqmlmodule QtQuick 2
    \ingroup qtquick-input
    \brief For specifying drag and drop handling in an area

    A DropArea is an invisible item which receives events when other items are
    dragged over it.

    The \l Drag attached property can be used to notify the DropArea when an Item is
    dragged over it.

    The \l keys property can be used to filter drag events which don't include
    a matching key.

    The \l source property is communicated to the source of a drag event as
    the recipient of a drop on the drag target.

    The \l delegate property provides a means to specify a component to be
    instantiated for each active drag over a drag target.
*/

QQuickDropArea::QQuickDropArea(QQuickItem *parent)
    : QQuickItem(*new QQuickDropAreaPrivate, parent)
{
    setFlags(ItemAcceptsDrops);
}

QQuickDropArea::~QQuickDropArea()
{
}

/*!
    \qmlproperty bool QtQuick2::DropArea::containsDrag

    This property identifies whether the DropArea currently contains any
    dragged items.
*/

bool QQuickDropArea::containsDrag() const
{
    Q_D(const QQuickDropArea);
    return d->mimeData;
}

/*!
    \qmlproperty stringlist QtQuick2::DropArea::keys

    This property holds a list of drag keys a DropArea will accept.

    If no keys are listed the DropArea will accept events from any drag source,
    otherwise the drag source must have at least one compatible key.

    \sa QtQuick2::Drag::keys
*/

QStringList QQuickDropArea::keys() const
{
    Q_D(const QQuickDropArea);
    return d->keys;
}

void QQuickDropArea::setKeys(const QStringList &keys)
{
    Q_D(QQuickDropArea);
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

QQuickDropAreaDrag *QQuickDropArea::drag()
{
    Q_D(QQuickDropArea);
    if (!d->drag)
        d->drag = new QQuickDropAreaDrag(d);
    return d->drag;
}

/*!
    \qmlproperty Object QtQuick2::DropArea::drag.source

    This property holds the source of a drag.
*/

QObject *QQuickDropAreaDrag::source() const
{
    return d->source;
}

/*!
    \qmlproperty qreal QtQuick2::DropArea::drag.x
    \qmlproperty qreal QtQuick2::DropArea::drag.y

    These properties hold the coordinates of the last drag event.
*/

qreal QQuickDropAreaDrag::x() const
{
    return d->dragPosition.x();
}

qreal QQuickDropAreaDrag::y() const
{
    return d->dragPosition.y();
}

/*!
    \qmlsignal QtQuick2::DropArea::onPositionChanged(DragEvent drag)

    This handler is called when the position of a drag has changed.
*/

void QQuickDropArea::dragMoveEvent(QDragMoveEvent *event)
{
    Q_D(QQuickDropArea);
    if (!d->mimeData)
        return;

    d->dragPosition = event->pos();
    if (d->drag)
        emit d->drag->positionChanged();

    event->accept();
    QQuickDropEvent dragTargetEvent(d, event);
    emit positionChanged(&dragTargetEvent);
}

bool QQuickDropAreaPrivate::hasMatchingKey(const QStringList &keys) const
{
    if (keyRegExp.isEmpty())
        return true;

    QRegExp copy = keyRegExp;
    foreach (const QString &key, keys) {
        if (copy.exactMatch(key))
            return true;
    }
    return false;
}

QStringList QQuickDropAreaPrivate::getKeys(const QMimeData *mimeData) const
{
    if (const QQuickDragMimeData *dragMime = qobject_cast<const QQuickDragMimeData *>(mimeData))
        return dragMime->keys();
    return mimeData->formats();
}

/*!
    \qmlsignal QtQuick2::DropArea::onEntered(DragEvent drag)

    This handler is called when a \a drag enters the bounds of a DropArea.
*/

void QQuickDropArea::dragEnterEvent(QDragEnterEvent *event)
{
    Q_D(QQuickDropArea);
    const QMimeData *mimeData = event->mimeData();
    if (!d->effectiveEnable || d->mimeData || !mimeData || !d->hasMatchingKey(d->getKeys(mimeData)))
        return;

    d->dragPosition = event->pos();

    event->accept();
    QQuickDropEvent dragTargetEvent(d, event);
    emit entered(&dragTargetEvent);

    if (event->isAccepted()) {
        d->mimeData = const_cast<QMimeData *>(mimeData);
        if (QQuickDragMimeData *dragMime = qobject_cast<QQuickDragMimeData *>(d->mimeData))
            d->source = dragMime->source();
        else
            d->source = event->source();
        d->dragPosition = event->pos();
        if (d->drag) {
            emit d->drag->positionChanged();
            emit d->drag->sourceChanged();
        }
        emit containsDragChanged();
    }
}

/*!
    \qmlsignal QtQuick2::DropArea::onExited()

    This handler is called when a drag exits the bounds of a DropArea.
*/

void QQuickDropArea::dragLeaveEvent(QDragLeaveEvent *)
{
    Q_D(QQuickDropArea);
    if (!d->mimeData)
        return;

    emit exited();

    d->mimeData = 0;
    d->source = 0;
    emit containsDragChanged();
    if (d->drag)
        emit d->drag->sourceChanged();
}

/*!
    \qmlsignal QtQuick2::DropArea::onDropped(DragEvent drop)

    This handler is called when a drop event occurs within the bounds of a
    a DropArea.
*/

void QQuickDropArea::dropEvent(QDropEvent *event)
{
    Q_D(QQuickDropArea);
    if (!d->mimeData)
        return;

    QQuickDropEvent dragTargetEvent(d, event);
    emit dropped(&dragTargetEvent);

    d->mimeData = 0;
    d->source = 0;
    emit containsDragChanged();
    if (d->drag)
        emit d->drag->sourceChanged();
}

/*!
    \qmltype DragEvent
    \instantiates QQuickDropEvent
    \inqmlmodule QtQuick 2
    \ingroup qtquick-input-events
    \brief Provides information about a drag event

    The position of the drag event can be obtained from the \l x and \l y
    properties, and the \l keys property identifies the drag keys of the event
    \l source.
*/

/*!
    \qmlproperty real QtQuick2::DragEvent::x

    This property holds the x coordinate of a drag event.
*/

/*!
    \qmlproperty real QtQuick2::DragEvent::y

    This property holds the y coordinate of a drag event.
*/

/*!
    \qmlproperty Object QtQuick2::DragEvent::drag.source

    This property holds the source of a drag event.
*/

QObject *QQuickDropEvent::source()
{
    if (const QQuickDragMimeData *dragMime = qobject_cast<const QQuickDragMimeData *>(event->mimeData()))
        return dragMime->source();
    else
        return event->source();
}

/*!
    \qmlproperty stringlist QtQuick2::DragEvent::keys

    This property holds a list of keys identifying the data type or source of a
    drag event.
*/

QStringList QQuickDropEvent::keys() const
{
    return d->getKeys(event->mimeData());
}

/*!
    \qmlproperty enumeration QtQuick2::DragEvent::action

    This property holds the action that the \l source is to perform on an accepted drop.

    The drop action may be one of:

    \list
    \li Qt.CopyAction Copy the data to the target
    \li Qt.MoveAction Move the data from the source to the target
    \li Qt.LinkAction Create a link from the source to the target.
    \li Qt.IgnoreAction Ignore the action (do nothing with the data).
    \endlist
*/

/*!
    \qmlproperty flags QtQuick2::DragEvent::supportedActions

    This property holds the set of \l {action}{actions} supported by the
    drag source.
*/

/*!
    \qmlproperty bool QtQuick2::DragEvent::accepted

    This property holds whether the drag event was accepted by a handler.

    The default value is true.
*/

/*!
    \qmlmethod QtQuick2::DragEvent::accept()
    \qmlmethod QtQuick2::DragEvent::accept(enumeration action)

    Accepts the drag event.

    If an \a action is specified it will overwrite the value of the \l action property.
*/

void QQuickDropEvent::accept(QQmlV8Function *args)
{
    Qt::DropAction action = event->dropAction();

    if (args->Length() >= 1) {
        v8::Local<v8::Value> v = (*args)[0];
        if (v->IsInt32())
            action = Qt::DropAction(v->Int32Value());
    }
    // get action from arguments.
    event->setDropAction(action);
    event->accept();
}


QT_END_NAMESPACE

#endif // QT_NO_DRAGANDDROP
