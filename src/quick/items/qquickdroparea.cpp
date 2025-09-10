// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:interprocess-communication

#include "qquickdroparea_p.h"
#include "qquickdrag_p.h"
#include "qquickitem_p.h"

#include <private/qv4arraybuffer_p.h>

#include <QtCore/qpointer.h>
#include <QtCore/qregularexpression.h>

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
    QRegularExpression keyRegExp;
    QPointF dragPosition;
    QQuickDropAreaDrag *drag;
    QPointer<QObject> source;
    bool containsDrag;
};

QQuickDropAreaPrivate::QQuickDropAreaPrivate()
    : drag(nullptr)
    , containsDrag(false)
{
}

QQuickDropAreaPrivate::~QQuickDropAreaPrivate()
{
    delete drag;
}

/*!
    \qmltype DropArea
    \nativetype QQuickDropArea
    \inherits Item
    \inqmlmodule QtQuick
    \ingroup qtquick-input
    \brief For specifying drag and drop handling in an area.

    A DropArea is an invisible item which receives events when other items are
    dragged over it.

    The \l Drag attached property can be used to notify the DropArea when an Item is
    dragged over it.

    The \l keys property can be used to filter drag events which don't include
    a matching key.

    The \l drag.source property is communicated to the source of a drag event as
    the recipient of a drop on the drag target.

    \sa {Qt Quick Examples - Drag and Drop}
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
    \qmlproperty bool QtQuick::DropArea::containsDrag

    This property identifies whether the DropArea currently contains any
    dragged items.
*/

bool QQuickDropArea::containsDrag() const
{
    Q_D(const QQuickDropArea);
    return d->containsDrag;
}

/*!
    \qmlproperty stringlist QtQuick::DropArea::keys

    This property holds a list of drag keys a DropArea will accept.

    If no keys are listed the DropArea will accept events from any drag source,
    otherwise the drag source must have at least one compatible key.

    \sa QtQuick::Drag::keys
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
            d->keyRegExp = QRegularExpression();
        } else {
            QString pattern = QLatin1Char('(') + QRegularExpression::escape(keys.first());
            for (int i = 1; i < keys.size(); ++i)
                pattern += QLatin1Char('|') + QRegularExpression::escape(keys.at(i));
            pattern += QLatin1Char(')');
            d->keyRegExp = QRegularExpression(
                    QRegularExpression::anchoredPattern(pattern.replace(QLatin1String("\\*"),
                                                                        QLatin1String(".+"))));
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
    \qmlproperty QtObject QtQuick::DropArea::drag.source

    This property holds the source of a drag.
*/

QObject *QQuickDropAreaDrag::source() const
{
    return d->source;
}

/*!
    \qmlpropertygroup QtQuick::DropArea::drag
    \qmlproperty real QtQuick::DropArea::drag.x
    \qmlproperty real QtQuick::DropArea::drag.y

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
    \qmlsignal QtQuick::DropArea::positionChanged(DragEvent drag)

    This signal is emitted when the position of a \a drag has changed.
*/

void QQuickDropArea::dragMoveEvent(QDragMoveEvent *event)
{
    Q_D(QQuickDropArea);
    if (!d->containsDrag)
        return;

    d->dragPosition = event->position().toPoint();
    if (d->drag)
        emit d->drag->positionChanged();

    event->accept();
    QQuickDragEvent dragTargetEvent(d, event);
    emit positionChanged(&dragTargetEvent);
}

bool QQuickDropAreaPrivate::hasMatchingKey(const QStringList &keys) const
{
    if (keyRegExp.pattern().isEmpty())
        return true;

    for (const QString &key : keys) {
        if (key.contains(keyRegExp))
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
    \qmlsignal QtQuick::DropArea::entered(DragEvent drag)

    This signal is emitted when a \a drag enters the bounds of a DropArea.
*/

void QQuickDropArea::dragEnterEvent(QDragEnterEvent *event)
{
    Q_D(QQuickDropArea);
    const QMimeData *mimeData = event->mimeData();
    if (!d->effectiveEnable || d->containsDrag || !mimeData || !d->hasMatchingKey(d->getKeys(mimeData)))
        return;

    const QQuickDragMimeData *dragMime = qobject_cast<const QQuickDragMimeData *>(mimeData);
    auto dragSource = dragMime ? dragMime->source() : event->source();

    // if the source of the drag is an ancestor of the drop area, then dragging
    // also drags the drop area; see QTBUG-64128
    if (QQuickItem *dragSourceItem = qobject_cast<QQuickItem *>(dragSource)) {
        if (dragSourceItem->isAncestorOf(this))
            return;
    }

    d->dragPosition = event->position().toPoint();

    event->accept();

    QQuickDragEvent dragTargetEvent(d, event);
    emit entered(&dragTargetEvent);
    if (!event->isAccepted())
        return;

    d->containsDrag = true;
    d->source = dragSource;
    d->dragPosition = event->position().toPoint();
    if (d->drag) {
        emit d->drag->positionChanged();
        emit d->drag->sourceChanged();
    }
    emit containsDragChanged();
}

/*!
    \qmlsignal QtQuick::DropArea::exited()

    This signal is emitted when a drag exits the bounds of a DropArea.
*/

void QQuickDropArea::dragLeaveEvent(QDragLeaveEvent *)
{
    Q_D(QQuickDropArea);
    if (!d->containsDrag)
        return;

    emit exited();

    d->containsDrag = false;
    d->source = nullptr;
    emit containsDragChanged();
    if (d->drag)
        emit d->drag->sourceChanged();
}

/*!
    \qmlsignal QtQuick::DropArea::dropped(DragEvent drop)

    This signal is emitted when a \a drop event occurs within the bounds of
    a DropArea.
*/

void QQuickDropArea::dropEvent(QDropEvent *event)
{
    Q_D(QQuickDropArea);
    if (!d->containsDrag)
        return;

    QQuickDragEvent dragTargetEvent(d, event);
    emit dropped(&dragTargetEvent);

    d->containsDrag = false;
    d->source = nullptr;
    emit containsDragChanged();
    if (d->drag)
        emit d->drag->sourceChanged();
}

/*!
    \qmltype DragEvent
    \nativetype QQuickDragEvent
    \inqmlmodule QtQuick
    \ingroup qtquick-input-events
    \brief Provides information about a drag event.

    The position of the drag event can be obtained from the \l x and \l y
    properties, and the \l keys property identifies the drag keys of the event
    \l {drag.source}{source}.

    The existence of specific drag types can be determined using the \l hasColor,
    \l hasHtml, \l hasText, and \l hasUrls properties.

    The list of all supplied formats can be determined using the \l formats property.

    Specific drag types can be obtained using the \l colorData, \l html, \l text,
    and \l urls properties.

    A string version of any available mimeType can be obtained using \l getDataAsString.
*/

/*!
    \qmlproperty real QtQuick::DragEvent::x

    This property holds the x coordinate of a drag event.
*/

/*!
    \qmlproperty real QtQuick::DragEvent::y

    This property holds the y coordinate of a drag event.
*/

/*!
    \qmlproperty QtObject QtQuick::DragEvent::drag.source

    This property holds the source of a drag event.
*/

/*!
    \qmlproperty stringlist QtQuick::DragEvent::keys

    This property holds a list of keys identifying the data type or source of a
    drag event.
*/

/*!
    \qmlproperty enumeration QtQuick::DragEvent::action

    This property holds the action that the \l {drag.source}{source} is to perform on an accepted drop.

    The drop action may be one of:

    \value Qt.CopyAction    Copy the data to the target.
    \value Qt.MoveAction    Move the data from the source to the target.
    \value Qt.LinkAction    Create a link from the source to the target.
    \value Qt.IgnoreAction  Ignore the action (do nothing with the data).
*/

/*!
    \qmlproperty flags QtQuick::DragEvent::supportedActions

    This property holds the set of \l {action}{actions} supported by the
    drag source.
*/

/*!
    \qmlproperty flags QtQuick::DragEvent::proposedAction
    \since 5.2

    This property holds the set of \l {action}{actions} proposed by the
    drag source.
*/

/*!
    \qmlproperty bool QtQuick::DragEvent::accepted

    This property holds whether the drag event was accepted by a handler.

    The default value is true.
*/

/*!
    \qmlmethod QtQuick::DragEvent::accept()
    \qmlmethod QtQuick::DragEvent::accept(enumeration action)

    Accepts the drag event.

    If an \a action is specified it will overwrite the value of the \l action property.
*/

/*!
    \qmlmethod QtQuick::DragEvent::acceptProposedAction()
    \since 5.2

    Accepts the drag event with the \l proposedAction.
*/

/*!
    \qmlproperty bool QtQuick::DragEvent::hasColor
    \since 5.2

    This property holds whether the drag event contains a color item.
*/

/*!
    \qmlproperty bool QtQuick::DragEvent::hasHtml
    \since 5.2

    This property holds whether the drag event contains a html item.
*/

/*!
    \qmlproperty bool QtQuick::DragEvent::hasText
    \since 5.2

    This property holds whether the drag event contains a text item.
*/

/*!
    \qmlproperty bool QtQuick::DragEvent::hasUrls
    \since 5.2

    This property holds whether the drag event contains one or more url items.
*/

/*!
    \qmlproperty color QtQuick::DragEvent::colorData
    \since 5.2

    This property holds color data, if any.
*/

/*!
    \qmlproperty string QtQuick::DragEvent::html
    \since 5.2

    This property holds html data, if any.
*/

/*!
    \qmlproperty string QtQuick::DragEvent::text
    \since 5.2

    This property holds text data, if any.
*/

/*!
    \qmlproperty urllist QtQuick::DragEvent::urls
    \since 5.2

    This property holds a list of urls, if any.
*/

/*!
    \qmlproperty stringlist QtQuick::DragEvent::formats
    \since 5.2

    This property holds a list of mime type formats contained in the drag data.
*/

/*!
    \qmlmethod string QtQuick::DragEvent::getDataAsString(string format)
    \since 5.2

    Returns the data for the given \a format converted to a string. \a format should be one contained in the \l formats property.
*/

/*!
    \qmlmethod string QtQuick::DragEvent::getDataAsArrayBuffer(string format)
    \since 5.5

    Returns the data for the given \a format into an ArrayBuffer, which can
    easily be translated into a QByteArray. \a format should be one contained in the \l formats property.
*/

QObject *QQuickDragEvent::source() const
{
    if (const QQuickDragMimeData *dragMime = qobject_cast<const QQuickDragMimeData *>(event->mimeData()))
        return dragMime->source();
    else
        return event->source();
}

QStringList QQuickDragEvent::keys() const
{
    return d->getKeys(event->mimeData());
}

bool QQuickDragEvent::hasColor() const
{
    return event->mimeData()->hasColor();
}

bool QQuickDragEvent::hasHtml() const
{
    return event->mimeData()->hasHtml();
}

bool QQuickDragEvent::hasText() const
{
    return event->mimeData()->hasText();
}

bool QQuickDragEvent::hasUrls() const
{
    return event->mimeData()->hasUrls();
}

QVariant QQuickDragEvent::colorData() const
{
    return event->mimeData()->colorData();
}

QString QQuickDragEvent::html() const
{
    return event->mimeData()->html();
}

QString QQuickDragEvent::text() const
{
    return event->mimeData()->text();
}

QList<QUrl> QQuickDragEvent::urls() const
{
    return event->mimeData()->urls();
}

QStringList QQuickDragEvent::formats() const
{
    return event->mimeData()->formats();
}

QString QQuickDragEvent::getDataAsString(const QString &format) const
{
    return QString::fromUtf8(event->mimeData()->data(format));
}

QByteArray QQuickDragEvent::getDataAsArrayBuffer(const QString &format) const
{
    return event->mimeData()->data(format);
}

void QQuickDragEvent::acceptProposedAction()
{
    event->acceptProposedAction();
}

void QQuickDragEvent::accept()
{
    Qt::DropAction action = event->dropAction();
    event->setDropAction(action);
    event->accept();
}

void QQuickDragEvent::accept(Qt::DropAction action)
{
    // get action from arguments.
    event->setDropAction(action);
    event->accept();
}


QT_END_NAMESPACE

#include "moc_qquickdroparea_p.cpp"
