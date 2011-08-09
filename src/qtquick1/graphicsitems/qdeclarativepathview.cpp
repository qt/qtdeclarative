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

#include "QtQuick1/private/qdeclarativepathview_p.h"
#include "QtQuick1/private/qdeclarativepathview_p_p.h"

#include <QtQuick1/private/qdeclarativestate_p.h>
#include <QtQuick1/private/qdeclarativeopenmetaobject_p.h>
#include <QDebug>
#include <QEvent>
#include <QGraphicsSceneEvent>

#include <qmath.h>
#include <math.h>

QT_BEGIN_NAMESPACE



inline qreal qmlMod(qreal x, qreal y)
{
#ifdef QT_USE_MATH_H_FLOATS
    if(sizeof(qreal) == sizeof(float))
        return fmodf(float(x), float(y));
    else
#endif
        return fmod(x, y);
}

static QDeclarative1OpenMetaObjectType *qPathViewAttachedType = 0;

QDeclarative1PathViewAttached::QDeclarative1PathViewAttached(QObject *parent)
: QObject(parent), m_percent(-1), m_view(0), m_onPath(false), m_isCurrent(false)
{
    if (qPathViewAttachedType) {
        m_metaobject = new QDeclarative1OpenMetaObject(this, qPathViewAttachedType);
        m_metaobject->setCached(true);
    } else {
        m_metaobject = new QDeclarative1OpenMetaObject(this);
    }
}

QDeclarative1PathViewAttached::~QDeclarative1PathViewAttached()
{
}

QVariant QDeclarative1PathViewAttached::value(const QByteArray &name) const
{
    return m_metaobject->value(name);
}
void QDeclarative1PathViewAttached::setValue(const QByteArray &name, const QVariant &val)
{
    m_metaobject->setValue(name, val);
}


void QDeclarative1PathViewPrivate::init()
{
    Q_Q(QDeclarative1PathView);
    offset = 0;
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QGraphicsItem::ItemIsFocusScope);
    q->setFiltersChildEvents(true);
    q->connect(&tl, SIGNAL(updated()), q, SLOT(ticked()));
    lastPosTime.invalidate();
    static int timelineCompletedIdx = -1;
    static int movementEndingIdx = -1;
    if (timelineCompletedIdx == -1) {
        timelineCompletedIdx = QDeclarative1TimeLine::staticMetaObject.indexOfSignal("completed()");
        movementEndingIdx = QDeclarative1PathView::staticMetaObject.indexOfSlot("movementEnding()");
    }
    QMetaObject::connect(&tl, timelineCompletedIdx,
                         q, movementEndingIdx, Qt::DirectConnection);
}

QDeclarativeItem *QDeclarative1PathViewPrivate::getItem(int modelIndex)
{
    Q_Q(QDeclarative1PathView);
    requestedIndex = modelIndex;
    QDeclarativeItem *item = model->item(modelIndex, false);
    if (item) {
        if (!attType) {
            // pre-create one metatype to share with all attached objects
            attType = new QDeclarative1OpenMetaObjectType(&QDeclarative1PathViewAttached::staticMetaObject, qmlEngine(q));
            foreach(const QString &attr, path->attributes())
                attType->createProperty(attr.toUtf8());
        }
        qPathViewAttachedType = attType;
        QDeclarative1PathViewAttached *att = static_cast<QDeclarative1PathViewAttached *>(qmlAttachedPropertiesObject<QDeclarative1PathView>(item));
        qPathViewAttachedType = 0;
        if (att) {
            att->m_view = q;
            att->setOnPath(true);
        }
        item->setParentItem(q);
        QDeclarativeItemPrivate *itemPrivate = static_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(item));
        itemPrivate->addItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
    }
    requestedIndex = -1;
    return item;
}

void QDeclarative1PathViewPrivate::releaseItem(QDeclarativeItem *item)
{
    if (!item || !model)
        return;
    QDeclarativeItemPrivate *itemPrivate = static_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(item));
    itemPrivate->removeItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
    if (model->release(item) == 0) {
        // item was not destroyed, and we no longer reference it.
        if (QDeclarative1PathViewAttached *att = attached(item))
            att->setOnPath(false);
    }
}

QDeclarative1PathViewAttached *QDeclarative1PathViewPrivate::attached(QDeclarativeItem *item)
{
    return static_cast<QDeclarative1PathViewAttached *>(qmlAttachedPropertiesObject<QDeclarative1PathView>(item, false));
}

void QDeclarative1PathViewPrivate::clear()
{
    for (int i=0; i<items.count(); i++){
        QDeclarativeItem *p = items[i];
        releaseItem(p);
    }
    items.clear();
}

void QDeclarative1PathViewPrivate::updateMappedRange()
{
    if (model && pathItems != -1 && pathItems < modelCount)
        mappedRange = qreal(pathItems)/modelCount;
    else
        mappedRange = 1.0;
}

qreal QDeclarative1PathViewPrivate::positionOfIndex(qreal index) const
{
    qreal pos = -1.0;

    if (model && index >= 0 && index < modelCount) {
        qreal start = 0.0;
        if (haveHighlightRange && highlightRangeMode != QDeclarative1PathView::NoHighlightRange)
            start = highlightRangeStart;
        qreal globalPos = index + offset;
        globalPos = qmlMod(globalPos, qreal(modelCount)) / modelCount;
        if (pathItems != -1 && pathItems < modelCount) {
            globalPos += start * mappedRange;
            globalPos = qmlMod(globalPos, 1.0);
            if (globalPos < mappedRange)
                pos = globalPos / mappedRange;
        } else {
            pos = qmlMod(globalPos + start, 1.0);
        }
    }

    return pos;
}

void QDeclarative1PathViewPrivate::createHighlight()
{
    Q_Q(QDeclarative1PathView);
    if (!q->isComponentComplete())
        return;

    bool changed = false;
    if (highlightItem) {
        if (highlightItem->scene())
            highlightItem->scene()->removeItem(highlightItem);
        highlightItem->deleteLater();
        highlightItem = 0;
        changed = true;
    }

    QDeclarativeItem *item = 0;
    if (highlightComponent) {
        QDeclarativeContext *highlightContext = new QDeclarativeContext(qmlContext(q));
        QObject *nobj = highlightComponent->create(highlightContext);
        if (nobj) {
            QDeclarative_setParent_noEvent(highlightContext, nobj);
            item = qobject_cast<QDeclarativeItem *>(nobj);
            if (!item)
                delete nobj;
        } else {
            delete highlightContext;
        }
    } else {
        item = new QDeclarativeItem;
    }
    if (item) {
        QDeclarative_setParent_noEvent(item, q);
        item->setParentItem(q);
        highlightItem = item;
        changed = true;
    }
    if (changed)
        emit q->highlightItemChanged();
}

void QDeclarative1PathViewPrivate::updateHighlight()
{
    Q_Q(QDeclarative1PathView);
    if (!q->isComponentComplete() || !isValid())
        return;
    if (highlightItem) {
        if (haveHighlightRange && highlightRangeMode == QDeclarative1PathView::StrictlyEnforceRange) {
            updateItem(highlightItem, highlightRangeStart);
        } else {
            qreal target = currentIndex;

            offsetAdj = 0.0;
            tl.reset(moveHighlight);
            moveHighlight.setValue(highlightPosition);

            const int duration = highlightMoveDuration;

            if (target - highlightPosition > modelCount/2) {
                highlightUp = false;
                qreal distance = modelCount - target + highlightPosition;
                tl.move(moveHighlight, 0.0, QEasingCurve(QEasingCurve::InQuad), int(duration * highlightPosition / distance));
                tl.set(moveHighlight, modelCount-0.01);
                tl.move(moveHighlight, target, QEasingCurve(QEasingCurve::OutQuad), int(duration * (modelCount-target) / distance));
            } else if (target - highlightPosition <= -modelCount/2) {
                highlightUp = true;
                qreal distance = modelCount - highlightPosition + target;
                tl.move(moveHighlight, modelCount-0.01, QEasingCurve(QEasingCurve::InQuad), int(duration * (modelCount-highlightPosition) / distance));
                tl.set(moveHighlight, 0.0);
                tl.move(moveHighlight, target, QEasingCurve(QEasingCurve::OutQuad), int(duration * target / distance));
            } else {
                highlightUp = highlightPosition - target < 0;
                tl.move(moveHighlight, target, QEasingCurve(QEasingCurve::InOutQuad), duration);
            }
        }
    }
}

void QDeclarative1PathViewPrivate::setHighlightPosition(qreal pos)
{
    if (pos != highlightPosition) {
        qreal start = 0.0;
        qreal end = 1.0;
        if (haveHighlightRange && highlightRangeMode != QDeclarative1PathView::NoHighlightRange) {
            start = highlightRangeStart;
            end = highlightRangeEnd;
        }

        qreal range = qreal(modelCount);
        // calc normalized position of highlight relative to offset
        qreal relativeHighlight = qmlMod(pos + offset, range) / range;

        if (!highlightUp && relativeHighlight > end * mappedRange) {
            qreal diff = 1.0 - relativeHighlight;
            setOffset(offset + diff * range);
        } else if (highlightUp && relativeHighlight >= (end - start) * mappedRange) {
            qreal diff = relativeHighlight - (end - start) * mappedRange;
            setOffset(offset - diff * range - 0.00001);
        }

        highlightPosition = pos;
        qreal pathPos = positionOfIndex(pos);
        updateItem(highlightItem, pathPos);
        if (QDeclarative1PathViewAttached *att = attached(highlightItem))
            att->setOnPath(pathPos != -1.0);
    }
}

void QDeclarative1PathView::pathUpdated()
{
    Q_D(QDeclarative1PathView);
    QList<QDeclarativeItem*>::iterator it = d->items.begin();
    while (it != d->items.end()) {
        QDeclarativeItem *item = *it;
        if (QDeclarative1PathViewAttached *att = d->attached(item))
            att->m_percent = -1;
        ++it;
    }
    refill();
}

void QDeclarative1PathViewPrivate::updateItem(QDeclarativeItem *item, qreal percent)
{
    if (QDeclarative1PathViewAttached *att = attached(item)) {
        if (qFuzzyCompare(att->m_percent, percent))
            return;
        att->m_percent = percent;
        foreach(const QString &attr, path->attributes())
            att->setValue(attr.toUtf8(), path->attributeAt(attr, percent));
    }
    QPointF pf = path->pointAt(percent);
    item->setX(qRound(pf.x() - item->width()/2));
    item->setY(qRound(pf.y() - item->height()/2));
}

void QDeclarative1PathViewPrivate::regenerate()
{
    Q_Q(QDeclarative1PathView);
    if (!q->isComponentComplete())
        return;

    clear();

    if (!isValid())
        return;

    firstIndex = -1;
    updateMappedRange();
    q->refill();
}

/*!
    \qmlclass PathView QDeclarative1PathView
    \inqmlmodule QtQuick 1
    \ingroup qml-view-elements
    \since QtQuick 1.0
    \brief The PathView element lays out model-provided items on a path.
    \inherits Item

    A PathView displays data from models created from built-in QML elements like ListModel
    and XmlListModel, or custom model classes defined in C++ that inherit from
    QAbstractListModel.

    The view has a \l model, which defines the data to be displayed, and
    a \l delegate, which defines how the data should be displayed.  
    The \l delegate is instantiated for each item on the \l path.
    The items may be flicked to move them along the path.

    For example, if there is a simple list model defined in a file \c ContactModel.qml like this:

    \snippet doc/src/snippets/qtquick1/pathview/ContactModel.qml 0

    This data can be represented as a PathView, like this:

    \snippet doc/src/snippets/qtquick1/pathview/pathview.qml 0

    \image pathview.gif

    (Note the above example uses PathAttribute to scale and modify the
    opacity of the items as they rotate. This additional code can be seen in the
    PathAttribute documentation.)

    PathView does not automatically handle keyboard navigation.  This is because
    the keys to use for navigation will depend upon the shape of the path.  Navigation
    can be added quite simply by setting \c focus to \c true and calling
    \l decrementCurrentIndex() or \l incrementCurrentIndex(), for example to navigate
    using the left and right arrow keys:

    \qml
    PathView {
        // ...
        focus: true
        Keys.onLeftPressed: decrementCurrentIndex()
        Keys.onRightPressed: incrementCurrentIndex()
    }
    \endqml

    The path view itself is a focus scope (see \l{qmlfocus#Acquiring Focus and Focus Scopes}{the focus documentation page} for more details).

    Delegates are instantiated as needed and may be destroyed at any time.
    State should \e never be stored in a delegate.

    PathView attaches a number of properties to the root item of the delegate, for example
    \c {PathView.isCurrentItem}.  In the following example, the root delegate item can access
    this attached property directly as \c PathView.isCurrentItem, while the child
    \c nameText object must refer to this property as \c wrapper.PathView.isCurrentItem.

    \snippet doc/src/snippets/qtquick1/pathview/pathview.qml 1

    \bold Note that views do not enable \e clip automatically.  If the view
    is not clipped by another item or the screen, it will be necessary
    to set \e {clip: true} in order to have the out of view items clipped
    nicely.

    \sa Path, {declarative/modelviews/pathview}{PathView example}
*/

QDeclarative1PathView::QDeclarative1PathView(QDeclarativeItem *parent)
  : QDeclarativeItem(*(new QDeclarative1PathViewPrivate), parent)
{
    Q_D(QDeclarative1PathView);
    d->init();
}

QDeclarative1PathView::~QDeclarative1PathView()
{
    Q_D(QDeclarative1PathView);
    d->clear();
    if (d->attType)
        d->attType->release();
    if (d->ownModel)
        delete d->model;
}

/*!
    \qmlattachedproperty PathView PathView::view
    This attached property holds the view that manages this delegate instance.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty bool PathView::onPath
    This attached property holds whether the item is currently on the path.

    If a pathItemCount has been set, it is possible that some items may
    be instantiated, but not considered to be currently on the path.
    Usually, these items would be set invisible, for example:

    \qml
    Component {
        Rectangle {
            visible: PathView.onPath
            // ...
        }
    }
    \endqml

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty bool PathView::isCurrentItem
    This attached property is true if this delegate is the current item; otherwise false.

    It is attached to each instance of the delegate.

    This property may be used to adjust the appearance of the current item.

    \snippet doc/src/snippets/qtquick1/pathview/pathview.qml 1
*/

/*!
    \qmlproperty model QtQuick1::PathView::model
    This property holds the model providing data for the view.

    The model provides a set of data that is used to create the items for the view.
    For large or dynamic datasets the model is usually provided by a C++ model object.
    Models can also be created directly in QML, using the ListModel element.

    \sa {qmlmodels}{Data Models}
*/
QVariant QDeclarative1PathView::model() const
{
    Q_D(const QDeclarative1PathView);
    return d->modelVariant;
}

void QDeclarative1PathView::setModel(const QVariant &model)
{
    Q_D(QDeclarative1PathView);
    if (d->modelVariant == model)
        return;

    if (d->model) {
        disconnect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        disconnect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        disconnect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        disconnect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        disconnect(d->model, SIGNAL(createdItem(int,QDeclarativeItem*)), this, SLOT(createdItem(int,QDeclarativeItem*)));
        for (int i=0; i<d->items.count(); i++){
            QDeclarativeItem *p = d->items[i];
            d->model->release(p);
        }
        d->items.clear();
    }

    d->modelVariant = model;
    QObject *object = qvariant_cast<QObject*>(model);
    QDeclarative1VisualModel *vim = 0;
    if (object && (vim = qobject_cast<QDeclarative1VisualModel *>(object))) {
        if (d->ownModel) {
            delete d->model;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QDeclarative1VisualDataModel(qmlContext(this), this);
            d->ownModel = true;
        }
        if (QDeclarative1VisualDataModel *dataModel = qobject_cast<QDeclarative1VisualDataModel*>(d->model))
            dataModel->setModel(model);
    }
    d->modelCount = 0;
    if (d->model) {
        connect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        connect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        connect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        connect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        connect(d->model, SIGNAL(createdItem(int,QDeclarativeItem*)), this, SLOT(createdItem(int,QDeclarativeItem*)));
        d->modelCount = d->model->count();
        if (d->model->count())
            d->offset = qmlMod(d->offset, qreal(d->model->count()));
        if (d->offset < 0)
            d->offset = d->model->count() + d->offset;
}
    d->regenerate();
    d->fixOffset();
    emit countChanged();
    emit modelChanged();
}

/*!
    \qmlproperty int QtQuick1::PathView::count
    This property holds the number of items in the model.
*/
int QDeclarative1PathView::count() const
{
    Q_D(const QDeclarative1PathView);
    return d->model ? d->modelCount : 0;
}

/*!
    \qmlproperty Path QtQuick1::PathView::path
    This property holds the path used to lay out the items.
    For more information see the \l Path documentation.
*/
QDeclarative1Path *QDeclarative1PathView::path() const
{
    Q_D(const QDeclarative1PathView);
    return d->path;
}

void QDeclarative1PathView::setPath(QDeclarative1Path *path)
{
    Q_D(QDeclarative1PathView);
    if (d->path == path)
        return;
    if (d->path)
        disconnect(d->path, SIGNAL(changed()), this, SLOT(pathUpdated()));
    d->path = path;
    connect(d->path, SIGNAL(changed()), this, SLOT(pathUpdated()));
    if (d->isValid() && isComponentComplete()) {
        d->clear();
        if (d->attType) {
            d->attType->release();
            d->attType = 0;
        }
        d->regenerate();
    }
    emit pathChanged();
}

/*!
    \qmlproperty int QtQuick1::PathView::currentIndex
    This property holds the index of the current item.
*/
int QDeclarative1PathView::currentIndex() const
{
    Q_D(const QDeclarative1PathView);
    return d->currentIndex;
}

void QDeclarative1PathView::setCurrentIndex(int idx)
{
    Q_D(QDeclarative1PathView);
    if (d->model && d->modelCount)
        idx = qAbs(idx % d->modelCount);
    if (d->model && idx != d->currentIndex) {
        if (d->modelCount) {
            int itemIndex = (d->currentIndex - d->firstIndex + d->modelCount) % d->modelCount;
            if (itemIndex < d->items.count()) {
                if (QDeclarativeItem *item = d->items.at(itemIndex)) {
                    if (QDeclarative1PathViewAttached *att = d->attached(item))
                        att->setIsCurrentItem(false);
                }
            }
        }
        d->currentItem = 0;
        d->moveReason = QDeclarative1PathViewPrivate::SetIndex;
        d->currentIndex = idx;
        if (d->modelCount) {
            if (d->haveHighlightRange && d->highlightRangeMode == QDeclarative1PathView::StrictlyEnforceRange)
                d->snapToCurrent();
            int itemIndex = (idx - d->firstIndex + d->modelCount) % d->modelCount;
            if (itemIndex < d->items.count()) {
                d->currentItem = d->items.at(itemIndex);
                d->currentItem->setFocus(true);
                if (QDeclarative1PathViewAttached *att = d->attached(d->currentItem))
                    att->setIsCurrentItem(true);
            }
            d->currentItemOffset = d->positionOfIndex(d->currentIndex);
            d->updateHighlight();
        }
        emit currentIndexChanged();
    }
}

/*!
    \qmlmethod QtQuick1::PathView::incrementCurrentIndex()

    Increments the current index.

    \bold Note: methods should only be called after the Component has completed.
*/
void QDeclarative1PathView::incrementCurrentIndex()
{
    Q_D(QDeclarative1PathView);
    d->moveDirection = QDeclarative1PathViewPrivate::Positive;
    setCurrentIndex(currentIndex()+1);
}


/*!
    \qmlmethod QtQuick1::PathView::decrementCurrentIndex()

    Decrements the current index.

    \bold Note: methods should only be called after the Component has completed.
*/
void QDeclarative1PathView::decrementCurrentIndex()
{
    Q_D(QDeclarative1PathView);
    if (d->model && d->modelCount) {
        int idx = currentIndex()-1;
        if (idx < 0)
            idx = d->modelCount - 1;
        d->moveDirection = QDeclarative1PathViewPrivate::Negative;
        setCurrentIndex(idx);
    }
}

/*!
    \qmlproperty real QtQuick1::PathView::offset

    The offset specifies how far along the path the items are from their initial positions.
    This is a real number that ranges from 0.0 to the count of items in the model.
*/
qreal QDeclarative1PathView::offset() const
{
    Q_D(const QDeclarative1PathView);
    return d->offset;
}

void QDeclarative1PathView::setOffset(qreal offset)
{
    Q_D(QDeclarative1PathView);
    d->setOffset(offset);
    d->updateCurrent();
}

void QDeclarative1PathViewPrivate::setOffset(qreal o)
{
    Q_Q(QDeclarative1PathView);
    if (offset != o) {
        if (isValid() && q->isComponentComplete()) {
            offset = qmlMod(o, qreal(modelCount));
            if (offset < 0)
                offset += qreal(modelCount);
            q->refill();
        } else {
            offset = o;
        }
        emit q->offsetChanged();
    }
}

void QDeclarative1PathViewPrivate::setAdjustedOffset(qreal o)
{
    setOffset(o+offsetAdj);
}

/*!
    \qmlproperty Component QtQuick1::PathView::highlight
    This property holds the component to use as the highlight.

    An instance of the highlight component will be created for each view.
    The geometry of the resultant component instance will be managed by the view
    so as to stay with the current item.

    The below example demonstrates how to make a simple highlight.  Note the use
    of the \l{PathView::onPath}{PathView.onPath} attached property to ensure that
    the highlight is hidden when flicked away from the path.

    \qml
    Component {
        Rectangle {
            visible: PathView.onPath
            // ...
        }
    }
    \endqml

    \sa highlightItem, highlightRangeMode
*/

QDeclarativeComponent *QDeclarative1PathView::highlight() const
{
    Q_D(const QDeclarative1PathView);
    return d->highlightComponent;
}

void QDeclarative1PathView::setHighlight(QDeclarativeComponent *highlight)
{
    Q_D(QDeclarative1PathView);
    if (highlight != d->highlightComponent) {
        d->highlightComponent = highlight;
        d->createHighlight();
        d->updateHighlight();
        emit highlightChanged();
    }
}

/*!
  \qmlproperty Item QtQuick1::PathView::highlightItem

  \c highlightItem holds the highlight item, which was created
  from the \l highlight component.

  \sa highlight
*/
QDeclarativeItem *QDeclarative1PathView::highlightItem()
{
    Q_D(const QDeclarative1PathView);
    return d->highlightItem;
}
/*!
    \qmlproperty real QtQuick1::PathView::preferredHighlightBegin
    \qmlproperty real QtQuick1::PathView::preferredHighlightEnd
    \qmlproperty enumeration QtQuick1::PathView::highlightRangeMode

    These properties set the preferred range of the highlight (current item)
    within the view.  The preferred values must be in the range 0.0-1.0.

    If highlightRangeMode is set to \e PathView.NoHighlightRange

    If highlightRangeMode is set to \e PathView.ApplyRange the view will
    attempt to maintain the highlight within the range, however
    the highlight can move outside of the range at the ends of the path
    or due to a mouse interaction.

    If highlightRangeMode is set to \e PathView.StrictlyEnforceRange the highlight will never
    move outside of the range.  This means that the current item will change
    if a keyboard or mouse action would cause the highlight to move
    outside of the range.

    Note that this is the correct way to influence where the
    current item ends up when the view moves. For example, if you want the
    currently selected item to be in the middle of the path, then set the
    highlight range to be 0.5,0.5 and highlightRangeMode to PathView.StrictlyEnforceRange.
    Then, when the path scrolls,
    the currently selected item will be the item at that position. This also applies to
    when the currently selected item changes - it will scroll to within the preferred
    highlight range. Furthermore, the behaviour of the current item index will occur
    whether or not a highlight exists.

    The default value is \e PathView.StrictlyEnforceRange.

    Note that a valid range requires preferredHighlightEnd to be greater
    than or equal to preferredHighlightBegin.
*/
qreal QDeclarative1PathView::preferredHighlightBegin() const
{
    Q_D(const QDeclarative1PathView);
    return d->highlightRangeStart;
}

void QDeclarative1PathView::setPreferredHighlightBegin(qreal start)
{
    Q_D(QDeclarative1PathView);
    if (d->highlightRangeStart == start || start < 0 || start > 1.0)
        return;
    d->highlightRangeStart = start;
    d->haveHighlightRange = d->highlightRangeMode != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    refill();
    emit preferredHighlightBeginChanged();
}

qreal QDeclarative1PathView::preferredHighlightEnd() const
{
    Q_D(const QDeclarative1PathView);
    return d->highlightRangeEnd;
}

void QDeclarative1PathView::setPreferredHighlightEnd(qreal end)
{
    Q_D(QDeclarative1PathView);
    if (d->highlightRangeEnd == end || end < 0 || end > 1.0)
        return;
    d->highlightRangeEnd = end;
    d->haveHighlightRange = d->highlightRangeMode != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    refill();
    emit preferredHighlightEndChanged();
}

QDeclarative1PathView::HighlightRangeMode QDeclarative1PathView::highlightRangeMode() const
{
    Q_D(const QDeclarative1PathView);
    return d->highlightRangeMode;
}

void QDeclarative1PathView::setHighlightRangeMode(HighlightRangeMode mode)
{
    Q_D(QDeclarative1PathView);
    if (d->highlightRangeMode == mode)
        return;
    d->highlightRangeMode = mode;
    d->haveHighlightRange = d->highlightRangeMode != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit highlightRangeModeChanged();
}


/*!
    \qmlproperty int QtQuick1::PathView::highlightMoveDuration
    This property holds the move animation duration of the highlight delegate.

    If the highlightRangeMode is StrictlyEnforceRange then this property
    determines the speed that the items move along the path.

    The default value for the duration is 300ms.
*/
int QDeclarative1PathView::highlightMoveDuration() const
{
    Q_D(const QDeclarative1PathView);
    return d->highlightMoveDuration;
}

void QDeclarative1PathView::setHighlightMoveDuration(int duration)
{
    Q_D(QDeclarative1PathView);
    if (d->highlightMoveDuration == duration)
        return;
    d->highlightMoveDuration = duration;
    emit highlightMoveDurationChanged();
}

/*!
    \qmlproperty real QtQuick1::PathView::dragMargin
    This property holds the maximum distance from the path that initiate mouse dragging.

    By default the path can only be dragged by clicking on an item.  If
    dragMargin is greater than zero, a drag can be initiated by clicking
    within dragMargin pixels of the path.
*/
qreal QDeclarative1PathView::dragMargin() const
{
    Q_D(const QDeclarative1PathView);
    return d->dragMargin;
}

void QDeclarative1PathView::setDragMargin(qreal dragMargin)
{
    Q_D(QDeclarative1PathView);
    if (d->dragMargin == dragMargin)
        return;
    d->dragMargin = dragMargin;
    emit dragMarginChanged();
}

/*!
    \qmlproperty real QtQuick1::PathView::flickDeceleration
    This property holds the rate at which a flick will decelerate.

    The default is 100.
*/
qreal QDeclarative1PathView::flickDeceleration() const
{
    Q_D(const QDeclarative1PathView);
    return d->deceleration;
}

void QDeclarative1PathView::setFlickDeceleration(qreal dec)
{
    Q_D(QDeclarative1PathView);
    if (d->deceleration == dec)
        return;
    d->deceleration = dec;
    emit flickDecelerationChanged();
}

/*!
    \qmlproperty bool QtQuick1::PathView::interactive

    A user cannot drag or flick a PathView that is not interactive.

    This property is useful for temporarily disabling flicking. This allows
    special interaction with PathView's children.
*/
bool QDeclarative1PathView::isInteractive() const
{
    Q_D(const QDeclarative1PathView);
    return d->interactive;
}

void QDeclarative1PathView::setInteractive(bool interactive)
{
    Q_D(QDeclarative1PathView);
    if (interactive != d->interactive) {
        d->interactive = interactive;
        if (!interactive)
            d->tl.clear();
        emit interactiveChanged();
    }
}

/*!
    \qmlproperty bool QtQuick1::PathView::moving

    This property holds whether the view is currently moving
    due to the user either dragging or flicking the view.
*/
bool QDeclarative1PathView::isMoving() const
{
    Q_D(const QDeclarative1PathView);
    return d->moving;
}

/*!
    \qmlproperty bool QtQuick1::PathView::flicking

    This property holds whether the view is currently moving
    due to the user flicking the view.
*/
bool QDeclarative1PathView::isFlicking() const
{
    Q_D(const QDeclarative1PathView);
    return d->flicking;
}

/*!
    \qmlsignal QtQuick1::PathView::onMovementStarted()

    This handler is called when the view begins moving due to user
    interaction.
*/

/*!
    \qmlsignal QtQuick1::PathView::onMovementEnded()

    This handler is called when the view stops moving due to user
    interaction.  If a flick was generated, this handler will
    be triggered once the flick stops.  If a flick was not
    generated, the handler will be triggered when the
    user stops dragging - i.e. a mouse or touch release.
*/

/*!
    \qmlsignal QtQuick1::PathView::onFlickStarted()

    This handler is called when the view is flicked.  A flick
    starts from the point that the mouse or touch is released,
    while still in motion.
*/

/*!
    \qmlsignal QtQuick1::PathView::onFlickEnded()

    This handler is called when the view stops moving due to a flick.
*/

/*!
    \qmlproperty Component QtQuick1::PathView::delegate

    The delegate provides a template defining each item instantiated by the view.
    The index is exposed as an accessible \c index property.  Properties of the
    model are also available depending upon the type of \l {qmlmodels}{Data Model}.

    The number of elements in the delegate has a direct effect on the
    flicking performance of the view when pathItemCount is specified.  If at all possible, place functionality
    that is not needed for the normal display of the delegate in a \l Loader which
    can load additional elements when needed.

    Note that the PathView will layout the items based on the size of the root
    item in the delegate.

    Here is an example delegate:
    \snippet doc/src/snippets/qtquick1/pathview/pathview.qml 1
*/
QDeclarativeComponent *QDeclarative1PathView::delegate() const
{
    Q_D(const QDeclarative1PathView);
     if (d->model) {
        if (QDeclarative1VisualDataModel *dataModel = qobject_cast<QDeclarative1VisualDataModel*>(d->model))
            return dataModel->delegate();
    }

    return 0;
}

void QDeclarative1PathView::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QDeclarative1PathView);
    if (delegate == this->delegate())
        return;
    if (!d->ownModel) {
        d->model = new QDeclarative1VisualDataModel(qmlContext(this));
        d->ownModel = true;
    }
    if (QDeclarative1VisualDataModel *dataModel = qobject_cast<QDeclarative1VisualDataModel*>(d->model)) {
        int oldCount = dataModel->count();
        dataModel->setDelegate(delegate);
        d->modelCount = dataModel->count();
        d->regenerate();
        if (oldCount != dataModel->count())
            emit countChanged();
        emit delegateChanged();
    }
}

/*!
  \qmlproperty int QtQuick1::PathView::pathItemCount
  This property holds the number of items visible on the path at any one time.
*/
int QDeclarative1PathView::pathItemCount() const
{
    Q_D(const QDeclarative1PathView);
    return d->pathItems;
}

void QDeclarative1PathView::setPathItemCount(int i)
{
    Q_D(QDeclarative1PathView);
    if (i == d->pathItems)
        return;
    if (i < 1)
        i = 1;
    d->pathItems = i;
    d->updateMappedRange();
    if (d->isValid() && isComponentComplete()) {
        d->regenerate();
    }
    emit pathItemCountChanged();
}

QPointF QDeclarative1PathViewPrivate::pointNear(const QPointF &point, qreal *nearPercent) const
{
    //XXX maybe do recursively at increasing resolution.
    qreal mindist = 1e10; // big number
    QPointF nearPoint = path->pointAt(0);
    qreal nearPc = 0;
    for (qreal i=1; i < 1000; i++) {
        QPointF pt = path->pointAt(i/1000.0);
        QPointF diff = pt - point;
        qreal dist = diff.x()*diff.x() + diff.y()*diff.y();
        if (dist < mindist) {
            nearPoint = pt;
            nearPc = i;
            mindist = dist;
        }
    }

    if (nearPercent)
        *nearPercent = nearPc / 1000.0;

    return nearPoint;
}

void QDeclarative1PathView::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1PathView);
    if (d->interactive) {
        d->handleMousePressEvent(event);
        event->accept();
    } else {
        QDeclarativeItem::mousePressEvent(event);
    }
}

void QDeclarative1PathViewPrivate::handleMousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_Q(QDeclarative1PathView);
    if (!interactive || !items.count())
        return;
    QPointF scenePoint = q->mapToScene(event->pos());
    int idx = 0;
    for (; idx < items.count(); ++idx) {
        QRectF rect = items.at(idx)->boundingRect();
        rect = items.at(idx)->mapToScene(rect).boundingRect();
        if (rect.contains(scenePoint))
            break;
    }
    if (idx == items.count() && dragMargin == 0.)  // didn't click on an item
        return;

    startPoint = pointNear(event->pos(), &startPc);
    if (idx == items.count()) {
        qreal distance = qAbs(event->pos().x() - startPoint.x()) + qAbs(event->pos().y() - startPoint.y());
        if (distance > dragMargin)
            return;
    }

    if (tl.isActive() && flicking)
        stealMouse = true; // If we've been flicked then steal the click.
    else
        stealMouse = false;

    lastElapsed = 0;
    lastDist = 0;
    QDeclarativeItemPrivate::start(lastPosTime);
    tl.clear();
}

void QDeclarative1PathView::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1PathView);
    if (d->interactive) {
        d->handleMouseMoveEvent(event);
        if (d->stealMouse)
            setKeepMouseGrab(true);
        event->accept();
    } else {
        QDeclarativeItem::mouseMoveEvent(event);
    }
}

void QDeclarative1PathViewPrivate::handleMouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_Q(QDeclarative1PathView);
    if (!interactive || !lastPosTime.isValid())
        return;

    qreal newPc;
    QPointF pathPoint = pointNear(event->pos(), &newPc);
    if (!stealMouse) {
        QPointF delta = pathPoint - startPoint;
        if (qAbs(delta.x()) > QApplication::startDragDistance() || qAbs(delta.y()) > QApplication::startDragDistance()) {
            stealMouse = true;
            startPc = newPc;
        }
    }

    if (stealMouse) {
        moveReason = QDeclarative1PathViewPrivate::Mouse;
        qreal diff = (newPc - startPc)*modelCount*mappedRange;
        if (diff) {
            q->setOffset(offset + diff);

            if (diff > modelCount/2)
                diff -= modelCount;
            else if (diff < -modelCount/2)
                diff += modelCount;

            lastElapsed = QDeclarativeItemPrivate::restart(lastPosTime);
            lastDist = diff;
            startPc = newPc;
        }
        if (!moving) {
            moving = true;
            emit q->movingChanged();
            emit q->movementStarted();
        }
    }
}

void QDeclarative1PathView::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1PathView);
    if (d->interactive) {
        d->handleMouseReleaseEvent(event);
        event->accept();
        ungrabMouse();
    } else {
        QDeclarativeItem::mouseReleaseEvent(event);
    }
}

void QDeclarative1PathViewPrivate::handleMouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    Q_Q(QDeclarative1PathView);
    stealMouse = false;
    q->setKeepMouseGrab(false);
    if (!interactive || !lastPosTime.isValid())
        return;

    qreal elapsed = qreal(lastElapsed + QDeclarativeItemPrivate::elapsed(lastPosTime)) / 1000.;
    qreal velocity = elapsed > 0. ? lastDist / elapsed : 0;
    if (model && modelCount && qAbs(velocity) > 1.) {
        qreal count = pathItems == -1 ? modelCount : pathItems;
        if (qAbs(velocity) > count * 2) // limit velocity
            velocity = (velocity > 0 ? count : -count) * 2;
        // Calculate the distance to be travelled
        qreal v2 = velocity*velocity;
        qreal accel = deceleration/10;
        // + 0.25 to encourage moving at least one item in the flick direction
        qreal dist = qMin(qreal(modelCount-1), qreal(v2 / (accel * 2.0) + 0.25));
        if (haveHighlightRange && highlightRangeMode == QDeclarative1PathView::StrictlyEnforceRange) {
            // round to nearest item.
            if (velocity > 0.)
                dist = qRound(dist + offset) - offset;
            else
                dist = qRound(dist - offset) + offset;
            // Calculate accel required to stop on item boundary
            if (dist <= 0.) {
                dist = 0.;
                accel = 0.;
            } else {
                accel = v2 / (2.0f * qAbs(dist));
            }
        }
        offsetAdj = 0.0;
        moveOffset.setValue(offset);
        tl.accel(moveOffset, velocity, accel, dist);
        tl.callback(QDeclarative1TimeLineCallback(&moveOffset, fixOffsetCallback, this));
        if (!flicking) {
            flicking = true;
            emit q->flickingChanged();
            emit q->flickStarted();
        }
    } else {
        fixOffset();
    }

    lastPosTime.invalidate();
    if (!tl.isActive())
        q->movementEnding();
}

bool QDeclarative1PathView::sendMouseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1PathView);
    QGraphicsSceneMouseEvent mouseEvent(event->type());
    QRectF myRect = mapToScene(QRectF(0, 0, width(), height())).boundingRect();
    QGraphicsScene *s = scene();
    QDeclarativeItem *grabber = s ? qobject_cast<QDeclarativeItem*>(s->mouseGrabberItem()) : 0;
    bool stealThisEvent = d->stealMouse;
    if ((stealThisEvent || myRect.contains(event->scenePos().toPoint())) && (!grabber || !grabber->keepMouseGrab())) {
        mouseEvent.setAccepted(false);
        for (int i = 0x1; i <= 0x10; i <<= 1) {
            if (event->buttons() & i) {
                Qt::MouseButton button = Qt::MouseButton(i);
                mouseEvent.setButtonDownPos(button, mapFromScene(event->buttonDownPos(button)));
            }
        }
        mouseEvent.setScenePos(event->scenePos());
        mouseEvent.setLastScenePos(event->lastScenePos());
        mouseEvent.setPos(mapFromScene(event->scenePos()));
        mouseEvent.setLastPos(mapFromScene(event->lastScenePos()));

        switch(mouseEvent.type()) {
        case QEvent::GraphicsSceneMouseMove:
            d->handleMouseMoveEvent(&mouseEvent);
            break;
        case QEvent::GraphicsSceneMousePress:
            d->handleMousePressEvent(&mouseEvent);
            stealThisEvent = d->stealMouse;   // Update stealThisEvent in case changed by function call above
            break;
        case QEvent::GraphicsSceneMouseRelease:
            d->handleMouseReleaseEvent(&mouseEvent);
            break;
        default:
            break;
        }
        grabber = qobject_cast<QDeclarativeItem*>(s->mouseGrabberItem());
        if (grabber && stealThisEvent && !grabber->keepMouseGrab() && grabber != this)
            grabMouse();

        return d->stealMouse;
    } else if (d->lastPosTime.isValid()) {
        d->lastPosTime.invalidate();
        d->fixOffset();
    }
    if (mouseEvent.type() == QEvent::GraphicsSceneMouseRelease)
        d->stealMouse = false;
    return false;
}

bool QDeclarative1PathView::sceneEventFilter(QGraphicsItem *i, QEvent *e)
{
    Q_D(QDeclarative1PathView);
    if (!isVisible() || !d->interactive)
        return QDeclarativeItem::sceneEventFilter(i, e);

    switch (e->type()) {
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMouseRelease:
        return sendMouseEvent(static_cast<QGraphicsSceneMouseEvent *>(e));
    default:
        break;
    }

    return QDeclarativeItem::sceneEventFilter(i, e);
}

bool QDeclarative1PathView::sceneEvent(QEvent *event)
{
    bool rv = QDeclarativeItem::sceneEvent(event);
    if (event->type() == QEvent::UngrabMouse) {
        Q_D(QDeclarative1PathView);
        if (d->stealMouse) {
            // if our mouse grab has been removed (probably by another Flickable),
            // fix our state
            d->stealMouse = false;
            setKeepMouseGrab(false);
            d->lastPosTime.invalidate();
        }
    }
    return rv;
}

bool QDeclarative1PathView::event(QEvent *event)
{
    if (event->type() == QEvent::User) {
        refill();
        return true;
    }

    return QDeclarativeItem::event(event);
}

void QDeclarative1PathView::componentComplete()
{
    Q_D(QDeclarative1PathView);
    QDeclarativeItem::componentComplete();
    d->createHighlight();
    // It is possible that a refill has already happended to to Path
    // bindings being handled in the componentComplete().  If so
    // don't do it again.
    if (d->items.count() == 0 && d->model) {
        d->modelCount = d->model->count();
        d->regenerate();
    }
    d->updateHighlight();
}

void QDeclarative1PathView::refill()
{
    Q_D(QDeclarative1PathView);
    if (!d->isValid() || !isComponentComplete())
        return;

    d->layoutScheduled = false;
    bool currentVisible = false;

    // first move existing items and remove items off path
    int idx = d->firstIndex;
    QList<QDeclarativeItem*>::iterator it = d->items.begin();
    while (it != d->items.end()) {
        qreal pos = d->positionOfIndex(idx);
        QDeclarativeItem *item = *it;
        if (pos >= 0.0) {
            d->updateItem(item, pos);
            if (idx == d->currentIndex) {
                currentVisible = true;
                d->currentItemOffset = pos;
            }
            ++it;
        } else {
//            qDebug() << "release";
            d->updateItem(item, 1.0);
            d->releaseItem(item);
            if (it == d->items.begin()) {
                if (++d->firstIndex >= d->modelCount)
                    d->firstIndex = 0;
            }
            it = d->items.erase(it);
        }
        ++idx;
        if (idx >= d->modelCount)
            idx = 0;
    }
    if (!d->items.count())
        d->firstIndex = -1;

    if (d->modelCount) {
        // add items to beginning and end
        int count = d->pathItems == -1 ? d->modelCount : qMin(d->pathItems, d->modelCount);
        if (d->items.count() < count) {
            int idx = qRound(d->modelCount - d->offset) % d->modelCount;
            qreal startPos = 0.0;
            if (d->haveHighlightRange && d->highlightRangeMode != QDeclarative1PathView::NoHighlightRange)
                startPos = d->highlightRangeStart;
            if (d->firstIndex >= 0) {
                startPos = d->positionOfIndex(d->firstIndex);
                idx = (d->firstIndex + d->items.count()) % d->modelCount;
            }
            qreal pos = d->positionOfIndex(idx);
            while ((pos > startPos || !d->items.count()) && d->items.count() < count) {
    //            qDebug() << "append" << idx;
                QDeclarativeItem *item = d->getItem(idx);
                if (d->model->completePending())
                    item->setZValue(idx+1);
                if (d->currentIndex == idx) {
                    item->setFocus(true);
                    if (QDeclarative1PathViewAttached *att = d->attached(item))
                        att->setIsCurrentItem(true);
                    currentVisible = true;
                    d->currentItemOffset = pos;
                    d->currentItem = item;
                }
                if (d->items.count() == 0)
                    d->firstIndex = idx;
                d->items.append(item);
                d->updateItem(item, pos);
                if (d->model->completePending())
                    d->model->completeItem();
                ++idx;
                if (idx >= d->modelCount)
                    idx = 0;
                pos = d->positionOfIndex(idx);
            }

            idx = d->firstIndex - 1;
            if (idx < 0)
                idx = d->modelCount - 1;
            pos = d->positionOfIndex(idx);
            while (pos >= 0.0 && pos < startPos) {
    //            qDebug() << "prepend" << idx;
                QDeclarativeItem *item = d->getItem(idx);
                if (d->model->completePending())
                    item->setZValue(idx+1);
                if (d->currentIndex == idx) {
                    item->setFocus(true);
                    if (QDeclarative1PathViewAttached *att = d->attached(item))
                        att->setIsCurrentItem(true);
                    currentVisible = true;
                    d->currentItemOffset = pos;
                    d->currentItem = item;
                }
                d->items.prepend(item);
                d->updateItem(item, pos);
                if (d->model->completePending())
                    d->model->completeItem();
                d->firstIndex = idx;
                idx = d->firstIndex - 1;
                if (idx < 0)
                    idx = d->modelCount - 1;
                pos = d->positionOfIndex(idx);
            }
        }
    }

    if (!currentVisible)
        d->currentItemOffset = 1.0;

    if (d->highlightItem && d->haveHighlightRange && d->highlightRangeMode == QDeclarative1PathView::StrictlyEnforceRange) {
        d->updateItem(d->highlightItem, d->highlightRangeStart);
        if (QDeclarative1PathViewAttached *att = d->attached(d->highlightItem))
            att->setOnPath(true);
    } else if (d->highlightItem && d->moveReason != QDeclarative1PathViewPrivate::SetIndex) {
        d->updateItem(d->highlightItem, d->currentItemOffset);
        if (QDeclarative1PathViewAttached *att = d->attached(d->highlightItem))
            att->setOnPath(currentVisible);
    }
    while (d->itemCache.count())
        d->releaseItem(d->itemCache.takeLast());
}

void QDeclarative1PathView::itemsInserted(int modelIndex, int count)
{
    //XXX support animated insertion
    Q_D(QDeclarative1PathView);
    if (!d->isValid() || !isComponentComplete())
        return;

    if (d->modelCount) {
        d->itemCache += d->items;
        d->items.clear();
        if (modelIndex <= d->currentIndex) {
            d->currentIndex += count;
            emit currentIndexChanged();
        } else if (d->offset != 0) {
            d->offset += count;
            d->offsetAdj += count;
        }
    }
    d->modelCount += count;
    if (d->flicking || d->moving) {
        d->regenerate();
        d->updateCurrent();
    } else {
        d->firstIndex = -1;
        d->updateMappedRange();
        d->scheduleLayout();
    }
    emit countChanged();
}

void QDeclarative1PathView::itemsRemoved(int modelIndex, int count)
{
    //XXX support animated removal
    Q_D(QDeclarative1PathView);
    if (!d->model || !d->modelCount || !d->model->isValid() || !d->path || !isComponentComplete())
        return;

    // fix current
    bool currentChanged = false;
    if (d->currentIndex >= modelIndex + count) {
        d->currentIndex -= count;
        currentChanged = true;
    } else if (d->currentIndex >= modelIndex && d->currentIndex < modelIndex + count) {
        // current item has been removed.
        d->currentIndex = qMin(modelIndex, d->modelCount-count-1);
        if (d->currentItem) {
            if (QDeclarative1PathViewAttached *att = d->attached(d->currentItem))
                att->setIsCurrentItem(true);
        }
        currentChanged = true;
    }

    d->itemCache += d->items;
    d->items.clear();

    bool changedOffset = false;
    if (modelIndex > d->currentIndex) {
        if (d->offset >= count) {
            changedOffset = true;
            d->offset -= count;
            d->offsetAdj -= count;
        }
    }

    d->modelCount -= count;
    if (!d->modelCount) {
        while (d->itemCache.count())
            d->releaseItem(d->itemCache.takeLast());
        d->offset = 0;
        changedOffset = true;
        d->tl.reset(d->moveOffset);
        update();
    } else {
        d->regenerate();
        d->updateCurrent();
        if (!d->flicking && !d->moving && d->haveHighlightRange && d->highlightRangeMode == QDeclarative1PathView::StrictlyEnforceRange)
            d->snapToCurrent();
    }
    if (changedOffset)
        emit offsetChanged();
    if (currentChanged)
        emit currentIndexChanged();
    emit countChanged();
}

void QDeclarative1PathView::itemsMoved(int /*from*/, int /*to*/, int /*count*/)
{
    Q_D(QDeclarative1PathView);
    if (!d->isValid() || !isComponentComplete())
        return;

    QList<QDeclarativeItem *> removedItems = d->items;
    d->items.clear();
    d->regenerate();
    while (removedItems.count())
        d->releaseItem(removedItems.takeLast());

    // Fix current index
    if (d->currentIndex >= 0 && d->currentItem) {
        int oldCurrent = d->currentIndex;
        d->currentIndex = d->model->indexOf(d->currentItem, this);
        if (oldCurrent != d->currentIndex)
            emit currentIndexChanged();
    }
    d->updateCurrent();
}

void QDeclarative1PathView::modelReset()
{
    Q_D(QDeclarative1PathView);
    d->modelCount = d->model->count();
    d->regenerate();
    emit countChanged();
}

void QDeclarative1PathView::createdItem(int index, QDeclarativeItem *item)
{
    Q_D(QDeclarative1PathView);
    if (d->requestedIndex != index) {
        if (!d->attType) {
            // pre-create one metatype to share with all attached objects
            d->attType = new QDeclarative1OpenMetaObjectType(&QDeclarative1PathViewAttached::staticMetaObject, qmlEngine(this));
            foreach(const QString &attr, d->path->attributes())
                d->attType->createProperty(attr.toUtf8());
        }
        qPathViewAttachedType = d->attType;
        QDeclarative1PathViewAttached *att = static_cast<QDeclarative1PathViewAttached *>(qmlAttachedPropertiesObject<QDeclarative1PathView>(item));
        qPathViewAttachedType = 0;
        if (att) {
            att->m_view = this;
            att->setOnPath(false);
        }
        item->setParentItem(this);
        d->updateItem(item, index < d->firstIndex ? 0.0 : 1.0);
    }
}

void QDeclarative1PathView::destroyingItem(QDeclarativeItem *item)
{
    Q_UNUSED(item);
}

void QDeclarative1PathView::ticked()
{
    Q_D(QDeclarative1PathView);
    d->updateCurrent();
}

void QDeclarative1PathView::movementEnding()
{
    Q_D(QDeclarative1PathView);
    if (d->flicking) {
        d->flicking = false;
        emit flickingChanged();
        emit flickEnded();
    }
    if (d->moving && !d->stealMouse) {
        d->moving = false;
        emit movingChanged();
        emit movementEnded();
    }
}

// find the item closest to the snap position
int QDeclarative1PathViewPrivate::calcCurrentIndex()
{
    int current = -1;
    if (modelCount && model && items.count()) {
        offset = qmlMod(offset, modelCount);
        if (offset < 0)
            offset += modelCount;
        current = qRound(qAbs(qmlMod(modelCount - offset, modelCount)));
        current = current % modelCount;
    }

    return current;
}

void QDeclarative1PathViewPrivate::updateCurrent()
{
    Q_Q(QDeclarative1PathView);
    if (moveReason != Mouse)
        return;
    if (!modelCount || !haveHighlightRange || highlightRangeMode != QDeclarative1PathView::StrictlyEnforceRange)
        return;

    int idx = calcCurrentIndex();
    if (model && idx != currentIndex) {
        int itemIndex = (currentIndex - firstIndex + modelCount) % modelCount;
        if (itemIndex < items.count()) {
            if (QDeclarativeItem *item = items.at(itemIndex)) {
                if (QDeclarative1PathViewAttached *att = attached(item))
                    att->setIsCurrentItem(false);
            }
        }
        currentIndex = idx;
        currentItem = 0;
        itemIndex = (idx - firstIndex + modelCount) % modelCount;
        if (itemIndex < items.count()) {
            currentItem = items.at(itemIndex);
            currentItem->setFocus(true);
            if (QDeclarative1PathViewAttached *att = attached(currentItem))
                att->setIsCurrentItem(true);
        }
        emit q->currentIndexChanged();
    }
}

void QDeclarative1PathViewPrivate::fixOffsetCallback(void *d)
{
    ((QDeclarative1PathViewPrivate *)d)->fixOffset();
}

void QDeclarative1PathViewPrivate::fixOffset()
{
    Q_Q(QDeclarative1PathView);
    if (model && items.count()) {
        if (haveHighlightRange && highlightRangeMode == QDeclarative1PathView::StrictlyEnforceRange) {
            int curr = calcCurrentIndex();
            if (curr != currentIndex)
                q->setCurrentIndex(curr);
            else
                snapToCurrent();
        }
    }
}

void QDeclarative1PathViewPrivate::snapToCurrent()
{
    if (!model || modelCount <= 0)
        return;

    qreal targetOffset = qmlMod(modelCount - currentIndex, modelCount);

    moveReason = Other;
    offsetAdj = 0.0;
    tl.reset(moveOffset);
    moveOffset.setValue(offset);

    const int duration = highlightMoveDuration;

    if (moveDirection == Positive || (moveDirection == Shortest && targetOffset - offset > modelCount/2)) {
        qreal distance = modelCount - targetOffset + offset;
        if (targetOffset > moveOffset) {
            tl.move(moveOffset, 0.0, QEasingCurve(QEasingCurve::InQuad), int(duration * offset / distance));
            tl.set(moveOffset, modelCount);
            tl.move(moveOffset, targetOffset, QEasingCurve(offset == 0.0 ? QEasingCurve::InOutQuad : QEasingCurve::OutQuad), int(duration * (modelCount-targetOffset) / distance));
        } else {
            tl.move(moveOffset, targetOffset, QEasingCurve(QEasingCurve::InOutQuad), duration);
        }
    } else if (moveDirection == Negative || targetOffset - offset <= -modelCount/2) {
        qreal distance = modelCount - offset + targetOffset;
        if (targetOffset < moveOffset) {
            tl.move(moveOffset, modelCount, QEasingCurve(targetOffset == 0 ? QEasingCurve::InOutQuad : QEasingCurve::InQuad), int(duration * (modelCount-offset) / distance));
            tl.set(moveOffset, 0.0);
            tl.move(moveOffset, targetOffset, QEasingCurve(QEasingCurve::OutQuad), int(duration * targetOffset / distance));
        } else {
            tl.move(moveOffset, targetOffset, QEasingCurve(QEasingCurve::InOutQuad), duration);
        }
    } else {
        tl.move(moveOffset, targetOffset, QEasingCurve(QEasingCurve::InOutQuad), duration);
    }
    moveDirection = Shortest;
}

QDeclarative1PathViewAttached *QDeclarative1PathView::qmlAttachedProperties(QObject *obj)
{
    return new QDeclarative1PathViewAttached(obj);
}



QT_END_NAMESPACE

