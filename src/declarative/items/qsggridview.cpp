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

#include "qsggridview_p.h"
#include "qsgvisualitemmodel_p.h"
#include "qsgflickable_p_p.h"
#include "qsgitemview_p_p.h"

#include <private/qdeclarativesmoothedanimation_p_p.h>
#include <private/qlistmodelinterface_p.h>

#include <QtGui/qevent.h>
#include <QtCore/qmath.h>
#include <QtCore/qcoreapplication.h>
#include <math.h>
#include "qplatformdefs.h"

QT_BEGIN_NAMESPACE

#ifndef QML_FLICK_SNAPONETHRESHOLD
#define QML_FLICK_SNAPONETHRESHOLD 30
#endif

//----------------------------------------------------------------------------

class FxGridItemSG : public FxViewItem
{
public:
    FxGridItemSG(QSGItem *i, QSGGridView *v, bool own) : FxViewItem(i, own), view(v) {
        attached = static_cast<QSGGridViewAttached*>(qmlAttachedPropertiesObject<QSGGridView>(item));
        if (attached)
            static_cast<QSGGridViewAttached*>(attached)->setView(view);
    }

    ~FxGridItemSG() {}

    qreal position() const {
        return rowPos();
    }

    qreal endPosition() const {
        return endRowPos();
    }

    qreal size() const {
        return view->flow() == QSGGridView::LeftToRight ? view->cellHeight() : view->cellWidth();
    }

    qreal sectionSize() const {
        return 0.0;
    }

    qreal rowPos() const {
        if (view->flow() == QSGGridView::LeftToRight)
            return item->y();
        else
            return (view->effectiveLayoutDirection() == Qt::RightToLeft ? -view->cellWidth()-item->x() : item->x());
    }

    qreal colPos() const {
        if (view->flow() == QSGGridView::LeftToRight) {
            if (view->effectiveLayoutDirection() == Qt::RightToLeft) {
                qreal colSize = view->cellWidth();
                int columns = view->width()/colSize;
                return colSize * (columns-1) - item->x();
            } else {
                return item->x();
            }
        } else {
            return item->y();
        }
    }
    qreal endRowPos() const {
        if (view->flow() == QSGGridView::LeftToRight) {
            return item->y() + view->cellHeight();
        } else {
            if (view->effectiveLayoutDirection() == Qt::RightToLeft)
                return -item->x();
            else
                return item->x() + view->cellWidth();
        }
    }
    void setPosition(qreal col, qreal row) {
        if (view->effectiveLayoutDirection() == Qt::RightToLeft) {
            if (view->flow() == QSGGridView::LeftToRight) {
                int columns = view->width()/view->cellWidth();
                item->setPos(QPointF((view->cellWidth() * (columns-1) - col), row));
            } else {
                item->setPos(QPointF(-view->cellWidth()-row, col));
            }
        } else {
            if (view->flow() == QSGGridView::LeftToRight)
                item->setPos(QPointF(col, row));
            else
                item->setPos(QPointF(row, col));
        }
    }
    bool contains(qreal x, qreal y) const {
        return (x >= item->x() && x < item->x() + view->cellWidth() &&
                y >= item->y() && y < item->y() + view->cellHeight());
    }

    QSGGridView *view;
};

//----------------------------------------------------------------------------

class QSGGridViewPrivate : public QSGItemViewPrivate
{
    Q_DECLARE_PUBLIC(QSGGridView)

public:
    virtual Qt::Orientation layoutOrientation() const;
    virtual bool isContentFlowReversed() const;
    bool isRightToLeftTopToBottom() const;

    virtual qreal positionAt(int index) const;
    virtual qreal endPositionAt(int index) const;
    virtual qreal originPosition() const;
    virtual qreal lastPosition() const;

    qreal rowSize() const;
    qreal colSize() const;
    qreal colPosAt(int modelIndex) const;
    qreal rowPosAt(int modelIndex) const;
    qreal snapPosAt(qreal pos) const;
    FxViewItem *snapItemAt(qreal pos) const;
    int snapIndex() const;

    virtual bool addVisibleItems(qreal fillFrom, qreal fillTo, bool doBuffer);
    virtual bool removeNonVisibleItems(qreal bufferFrom, qreal bufferTo);
    virtual void visibleItemsChanged();

    virtual FxViewItem *newViewItem(int index, QSGItem *item);
    virtual void repositionPackageItemAt(QSGItem *item, int index);
    virtual void resetItemPosition(FxViewItem *item, FxViewItem *toItem);
    virtual void resetFirstItemPosition();
    virtual void moveItemBy(FxViewItem *item, const QList<FxViewItem *> &items, const QList<FxViewItem *> &movedBackwards);

    virtual void createHighlight();
    virtual void updateHighlight();
    virtual void resetHighlightPosition();

    virtual void setPosition(qreal pos);
    virtual void layoutVisibleItems();
    bool applyInsertionChange(const QDeclarativeChangeSet::Insert &, QList<FxViewItem *> *, QList<FxViewItem *>*, FxViewItem *);

    virtual qreal headerSize() const;
    virtual qreal footerSize() const;
    virtual bool showHeaderForIndex(int index) const;
    virtual bool showFooterForIndex(int index) const;
    virtual void updateHeader();
    virtual void updateFooter();

    virtual void changedVisibleIndex(int newIndex);
    virtual void initializeCurrentItem();

    virtual void updateViewport();
    virtual void itemGeometryChanged(QSGItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
    virtual void fixupPosition();
    virtual void fixup(AxisData &data, qreal minExtent, qreal maxExtent);
    virtual void flick(QSGItemViewPrivate::AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                        QDeclarativeTimeLineCallback::Callback fixupCallback, qreal velocity);

    QSGGridView::Flow flow;
    qreal cellWidth;
    qreal cellHeight;
    int columns;
    QSGGridView::SnapMode snapMode;

    QSmoothedAnimation *highlightXAnimator;
    QSmoothedAnimation *highlightYAnimator;

    QSGGridViewPrivate()
        : flow(QSGGridView::LeftToRight)
        , cellWidth(100), cellHeight(100), columns(1)
        , snapMode(QSGGridView::NoSnap)
        , highlightXAnimator(0), highlightYAnimator(0)
    {}
};

Qt::Orientation QSGGridViewPrivate::layoutOrientation() const
{
    return flow == QSGGridView::LeftToRight ? Qt::Vertical : Qt::Horizontal;
}

bool QSGGridViewPrivate::isContentFlowReversed() const
{
    return isRightToLeftTopToBottom();
}

bool QSGGridViewPrivate::isRightToLeftTopToBottom() const
{
    Q_Q(const QSGGridView);
    return flow == QSGGridView::TopToBottom && q->effectiveLayoutDirection() == Qt::RightToLeft;
}

void QSGGridViewPrivate::changedVisibleIndex(int newIndex)
{
    visibleIndex = newIndex / columns * columns;
}

void QSGGridViewPrivate::setPosition(qreal pos)
{
    Q_Q(QSGGridView);
    if (flow == QSGGridView::LeftToRight) {
        q->QSGFlickable::setContentY(pos);
        q->QSGFlickable::setContentX(0);
    } else {
        if (q->effectiveLayoutDirection() == Qt::LeftToRight)
            q->QSGFlickable::setContentX(pos);
        else
            q->QSGFlickable::setContentX(-pos-size());
        q->QSGFlickable::setContentY(0);
    }
}

qreal QSGGridViewPrivate::originPosition() const
{
    qreal pos = 0;
    if (!visibleItems.isEmpty())
        pos = static_cast<FxGridItemSG*>(visibleItems.first())->rowPos() - visibleIndex / columns * rowSize();
    return pos;
}

qreal QSGGridViewPrivate::lastPosition() const
{
    qreal pos = 0;
    if (model && model->count()) {
        // get end position of last item
        pos = (rowPosAt(model->count() - 1) + rowSize());
    }
    return pos;
}

qreal QSGGridViewPrivate::positionAt(int index) const
{
    return rowPosAt(index);
}

qreal QSGGridViewPrivate::endPositionAt(int index) const
{
    return rowPosAt(index) + rowSize();
}

qreal QSGGridViewPrivate::rowSize() const {
    return flow == QSGGridView::LeftToRight ? cellHeight : cellWidth;
}
qreal QSGGridViewPrivate::colSize() const {
    return flow == QSGGridView::LeftToRight ? cellWidth : cellHeight;
}

qreal QSGGridViewPrivate::colPosAt(int modelIndex) const
{
    if (FxViewItem *item = visibleItem(modelIndex))
        return static_cast<FxGridItemSG*>(item)->colPos();
    if (!visibleItems.isEmpty()) {
        if (modelIndex < visibleIndex) {
            int count = (visibleIndex - modelIndex) % columns;
            int col = static_cast<FxGridItemSG*>(visibleItems.first())->colPos() / colSize();
            col = (columns - count + col) % columns;
            return col * colSize();
        } else {
            int count = columns - 1 - (modelIndex - visibleItems.last()->index - 1) % columns;
            return static_cast<FxGridItemSG*>(visibleItems.last())->colPos() - count * colSize();
        }
    }
    return (modelIndex % columns) * colSize();
}

qreal QSGGridViewPrivate::rowPosAt(int modelIndex) const
{
    if (FxViewItem *item = visibleItem(modelIndex))
        return static_cast<FxGridItemSG*>(item)->rowPos();
    if (!visibleItems.isEmpty()) {
        if (modelIndex < visibleIndex) {
            FxGridItemSG *firstItem = static_cast<FxGridItemSG*>(visibleItems.first());
            int firstCol = firstItem->colPos() / colSize();
            int col = visibleIndex - modelIndex + (columns - firstCol - 1);
            int rows = col / columns;
            return firstItem->rowPos() - rows * rowSize();
        } else {
            FxGridItemSG *lastItem = static_cast<FxGridItemSG*>(visibleItems.last());
            int count = modelIndex - lastItem->index;
            int col = lastItem->colPos() + count * colSize();
            int rows = col / (columns * colSize());
            return lastItem->rowPos() + rows * rowSize();
        }
    }
    return (modelIndex / columns) * rowSize();
}


qreal QSGGridViewPrivate::snapPosAt(qreal pos) const
{
    Q_Q(const QSGGridView);
    qreal snapPos = 0;
    if (!visibleItems.isEmpty()) {
        qreal highlightStart = isRightToLeftTopToBottom() && highlightRangeStartValid ? size()-highlightRangeEnd : highlightRangeStart;
        pos += highlightStart;
        pos += rowSize()/2;
        snapPos = static_cast<FxGridItemSG*>(visibleItems.first())->rowPos() - visibleIndex / columns * rowSize();
        snapPos = pos - fmodf(pos - snapPos, qreal(rowSize()));
        snapPos -= highlightStart;
        qreal maxExtent;
        qreal minExtent;
        if (isRightToLeftTopToBottom()) {
            maxExtent = q->minXExtent();
            minExtent = q->maxXExtent();
        } else {
            maxExtent = flow == QSGGridView::LeftToRight ? -q->maxYExtent() : -q->maxXExtent();
            minExtent = flow == QSGGridView::LeftToRight ? -q->minYExtent() : -q->minXExtent();
        }
        if (snapPos > maxExtent)
            snapPos = maxExtent;
        if (snapPos < minExtent)
            snapPos = minExtent;
    }
    return snapPos;
}

FxViewItem *QSGGridViewPrivate::snapItemAt(qreal pos) const
{
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index == -1)
            continue;
        qreal itemTop = item->position();
        if (itemTop+rowSize()/2 >= pos && itemTop - rowSize()/2 <= pos)
            return item;
    }
    return 0;
}

int QSGGridViewPrivate::snapIndex() const
{
    int index = currentIndex;
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxGridItemSG *item = static_cast<FxGridItemSG*>(visibleItems.at(i));
        if (item->index == -1)
            continue;
        qreal itemTop = item->position();
        FxGridItemSG *hItem = static_cast<FxGridItemSG*>(highlight);
        if (itemTop >= hItem->rowPos()-rowSize()/2 && itemTop < hItem->rowPos()+rowSize()/2) {
            index = item->index;
            if (item->colPos() >= hItem->colPos()-colSize()/2 && item->colPos() < hItem->colPos()+colSize()/2)
                return item->index;
        }
    }
    return index;
}

FxViewItem *QSGGridViewPrivate::newViewItem(int modelIndex, QSGItem *item)
{
    Q_Q(QSGGridView);
    Q_UNUSED(modelIndex);
    return new FxGridItemSG(item, q, false);
}

bool QSGGridViewPrivate::addVisibleItems(qreal fillFrom, qreal fillTo, bool doBuffer)
{
    int colPos = colPosAt(visibleIndex);
    int rowPos = rowPosAt(visibleIndex);
    if (visibleItems.count()) {
        FxGridItemSG *lastItem = static_cast<FxGridItemSG*>(visibleItems.last());
        rowPos = lastItem->rowPos();
        colPos = lastItem->colPos() + colSize();
        if (colPos > colSize() * (columns-1)) {
            colPos = 0;
            rowPos += rowSize();
        }
    }

    int modelIndex = findLastVisibleIndex();
    modelIndex = modelIndex < 0 ? visibleIndex : modelIndex + 1;

    if (visibleItems.count() && (fillFrom > rowPos + rowSize()*2
        || fillTo < rowPosAt(visibleIndex) - rowSize())) {
        // We've jumped more than a page.  Estimate which items are now
        // visible and fill from there.
        int count = (fillFrom - (rowPos + rowSize())) / (rowSize()) * columns;
        for (int i = 0; i < visibleItems.count(); ++i)
            releaseItem(visibleItems.at(i));
        visibleItems.clear();
        modelIndex += count;
        if (modelIndex >= model->count())
            modelIndex = model->count() - 1;
        else if (modelIndex < 0)
            modelIndex = 0;
        modelIndex = modelIndex / columns * columns;
        visibleIndex = modelIndex;
        colPos = colPosAt(visibleIndex);
        rowPos = rowPosAt(visibleIndex);
    }

    int colNum = colPos / colSize();
    FxGridItemSG *item = 0;
    bool changed = false;

    while (modelIndex < model->count() && rowPos <= fillTo + rowSize()*(columns - colNum)/(columns+1)) {
//        qDebug() << "refill: append item" << modelIndex;
        if (!(item = static_cast<FxGridItemSG*>(createItem(modelIndex))))
            break;
        item->setPosition(colPos, rowPos);
        visibleItems.append(item);
        colPos += colSize();
        colNum++;
        if (colPos > colSize() * (columns-1)) {
            colPos = 0;
            colNum = 0;
            rowPos += rowSize();
        }
        ++modelIndex;
        changed = true;
        if (doBuffer) // never buffer more than one item per frame
            break;
    }

    if (visibleItems.count()) {
        FxGridItemSG *firstItem = static_cast<FxGridItemSG*>(visibleItems.first());
        rowPos = firstItem->rowPos();
        colPos = firstItem->colPos() - colSize();
        if (colPos < 0) {
            colPos = colSize() * (columns - 1);
            rowPos -= rowSize();
        }
    }

    colNum = colPos / colSize();
    while (visibleIndex > 0 && rowPos + rowSize() - 1 >= fillFrom - rowSize()*(colNum+1)/(columns+1)){
//        qDebug() << "refill: prepend item" << visibleIndex-1 << "top pos" << rowPos << colPos;
        if (!(item = static_cast<FxGridItemSG*>(createItem(visibleIndex-1))))
            break;
        --visibleIndex;
        item->setPosition(colPos, rowPos);
        visibleItems.prepend(item);
        colPos -= colSize();
        colNum--;
        if (colPos < 0) {
            colPos = colSize() * (columns - 1);
            colNum = columns-1;
            rowPos -= rowSize();
        }
        changed = true;
        if (doBuffer) // never buffer more than one item per frame
            break;
    }

    return changed;
}

bool QSGGridViewPrivate::removeNonVisibleItems(qreal bufferFrom, qreal bufferTo)
{
    FxGridItemSG *item = 0;
    bool changed = false;

    while (visibleItems.count() > 1
           && (item = static_cast<FxGridItemSG*>(visibleItems.first()))
                && item->rowPos()+rowSize()-1 < bufferFrom - rowSize()*(item->colPos()/colSize()+1)/(columns+1)) {
        if (item->attached->delayRemove())
            break;
//            qDebug() << "refill: remove first" << visibleIndex << "top end pos" << item->endRowPos();
        if (item->index != -1)
            visibleIndex++;
        visibleItems.removeFirst();
        releaseItem(item);
        changed = true;
    }
    while (visibleItems.count() > 1
           && (item = static_cast<FxGridItemSG*>(visibleItems.last()))
                && item->rowPos() > bufferTo + rowSize()*(columns - item->colPos()/colSize())/(columns+1)) {
        if (item->attached->delayRemove())
            break;
//            qDebug() << "refill: remove last" << visibleIndex+visibleItems.count()-1;
        visibleItems.removeLast();
        releaseItem(item);
        changed = true;
    }

    return changed;
}

void QSGGridViewPrivate::visibleItemsChanged()
{
    updateHeader();
    updateFooter();
    updateViewport();
}

void QSGGridViewPrivate::updateViewport()
{
    Q_Q(QSGGridView);
    columns = (int)qMax((flow == QSGGridView::LeftToRight ? q->width() : q->height()) / colSize(), qreal(1.));
    QSGItemViewPrivate::updateViewport();
}

void QSGGridViewPrivate::layoutVisibleItems()
{
    if (visibleItems.count()) {
        FxGridItemSG *firstItem = static_cast<FxGridItemSG*>(visibleItems.first());
        qreal rowPos = firstItem->rowPos();
        qreal colPos = firstItem->colPos();
        int col = visibleIndex % columns;
        if (colPos != col * colSize()) {
            colPos = col * colSize();
            firstItem->setPosition(colPos, rowPos);
        }
        for (int i = 1; i < visibleItems.count(); ++i) {
            FxGridItemSG *item = static_cast<FxGridItemSG*>(visibleItems.at(i));
            colPos += colSize();
            if (colPos > colSize() * (columns-1)) {
                colPos = 0;
                rowPos += rowSize();
            }
            item->setPosition(colPos, rowPos);
        }
    }
}

void QSGGridViewPrivate::repositionPackageItemAt(QSGItem *item, int index)
{
    Q_Q(QSGGridView);
    qreal pos = position();
    if (flow == QSGGridView::LeftToRight) {
        if (item->y() + item->height() > pos && item->y() < pos + q->height())
            item->setPos(QPointF(colPosAt(index), rowPosAt(index)));
    } else {
        if (item->x() + item->width() > pos && item->x() < pos + q->width()) {
            if (isRightToLeftTopToBottom())
                item->setPos(QPointF(-rowPosAt(index)-item->width(), colPosAt(index)));
            else
                item->setPos(QPointF(rowPosAt(index), colPosAt(index)));
        }
    }
}

void QSGGridViewPrivate::resetItemPosition(FxViewItem *item, FxViewItem *toItem)
{
    FxGridItemSG *toGridItem = static_cast<FxGridItemSG*>(toItem);
    static_cast<FxGridItemSG*>(item)->setPosition(toGridItem->colPos(), toGridItem->rowPos());
}

void QSGGridViewPrivate::resetFirstItemPosition()
{
    FxGridItemSG *item = static_cast<FxGridItemSG*>(visibleItems.first());
    item->setPosition(0, 0);
}

void QSGGridViewPrivate::moveItemBy(FxViewItem *item, const QList<FxViewItem *> &forwards, const QList<FxViewItem *> &backwards)
{
    int moveCount = forwards.count() - backwards.count();
    FxGridItemSG *gridItem = static_cast<FxGridItemSG*>(item);
    gridItem->setPosition(gridItem->colPos(), gridItem->rowPos() + ((moveCount / columns) * rowSize()));
}

void QSGGridViewPrivate::createHighlight()
{
    Q_Q(QSGGridView);
    bool changed = false;
    if (highlight) {
        if (trackedItem == highlight)
            trackedItem = 0;
        delete highlight;
        highlight = 0;

        delete highlightXAnimator;
        delete highlightYAnimator;
        highlightXAnimator = 0;
        highlightYAnimator = 0;

        changed = true;
    }

    if (currentItem) {
        QSGItem *item = createHighlightItem();
        if (item) {
            FxGridItemSG *newHighlight = new FxGridItemSG(item, q, true);
            if (autoHighlight)
                resetHighlightPosition();
            highlightXAnimator = new QSmoothedAnimation(q);
            highlightXAnimator->target = QDeclarativeProperty(item, QLatin1String("x"));
            highlightXAnimator->userDuration = highlightMoveDuration;
            highlightYAnimator = new QSmoothedAnimation(q);
            highlightYAnimator->target = QDeclarativeProperty(item, QLatin1String("y"));
            highlightYAnimator->userDuration = highlightMoveDuration;

            highlight = newHighlight;
            changed = true;
        }
    }
    if (changed)
        emit q->highlightItemChanged();
}

void QSGGridViewPrivate::updateHighlight()
{
    applyPendingChanges();

    if ((!currentItem && highlight) || (currentItem && !highlight))
        createHighlight();
    bool strictHighlight = haveHighlightRange && highlightRange == QSGGridView::StrictlyEnforceRange;
    if (currentItem && autoHighlight && highlight && (!strictHighlight || !pressed)) {
        // auto-update highlight
        highlightXAnimator->to = currentItem->item->x();
        highlightYAnimator->to = currentItem->item->y();
        highlight->item->setWidth(currentItem->item->width());
        highlight->item->setHeight(currentItem->item->height());

        highlightXAnimator->restart();
        highlightYAnimator->restart();
    }
    updateTrackedItem();
}

void QSGGridViewPrivate::resetHighlightPosition()
{
    if (highlight && currentItem) {
        FxGridItemSG *cItem = static_cast<FxGridItemSG*>(currentItem);
        static_cast<FxGridItemSG*>(highlight)->setPosition(cItem->colPos(), cItem->rowPos());
    }
}

qreal QSGGridViewPrivate::headerSize() const
{
    if (!header)
        return 0.0;
    return flow == QSGGridView::LeftToRight ? header->item->height() : header->item->width();
}

qreal QSGGridViewPrivate::footerSize() const
{
    if (!footer)
        return 0.0;
    return flow == QSGGridView::LeftToRight? footer->item->height() : footer->item->width();
}

bool QSGGridViewPrivate::showHeaderForIndex(int index) const
{
    return index / columns == 0;
}

bool QSGGridViewPrivate::showFooterForIndex(int index) const
{
    return index / columns == (model->count()-1) / columns;
}

void QSGGridViewPrivate::updateFooter()
{
    Q_Q(QSGGridView);
    bool created = false;
    if (!footer) {
        QSGItem *item = createComponentItem(footerComponent, true);
        if (!item)
            return;
        item->setZ(1);
        footer = new FxGridItemSG(item, q, true);
        created = true;
    }

    FxGridItemSG *gridItem = static_cast<FxGridItemSG*>(footer);
    qreal colOffset = 0;
    qreal rowOffset = 0;
    if (q->effectiveLayoutDirection() == Qt::RightToLeft) {
        if (flow == QSGGridView::TopToBottom)
            rowOffset = gridItem->item->width() - cellWidth;
        else
            colOffset = gridItem->item->width() - cellWidth;
    }
    if (visibleItems.count()) {
        qreal endPos = lastPosition();
        if (findLastVisibleIndex() == model->count()-1) {
            gridItem->setPosition(colOffset, endPos + rowOffset);
        } else {
            qreal visiblePos = isRightToLeftTopToBottom() ? -position() : position() + size();
            if (endPos <= visiblePos || gridItem->endPosition() <= endPos + rowOffset)
                gridItem->setPosition(colOffset, endPos + rowOffset);
        }
    } else {
        gridItem->setPosition(colOffset, rowOffset);
    }

    if (created)
        emit q->footerItemChanged();
}

void QSGGridViewPrivate::updateHeader()
{
    Q_Q(QSGGridView);
    bool created = false;
    if (!header) {
        QSGItem *item = createComponentItem(headerComponent, true);
        if (!item)
            return;
        item->setZ(1);
        header = new FxGridItemSG(item, q, true);
        created = true;
    }

    FxGridItemSG *gridItem = static_cast<FxGridItemSG*>(header);
    qreal colOffset = 0;
    qreal rowOffset = -headerSize();
    if (q->effectiveLayoutDirection() == Qt::RightToLeft) {
        if (flow == QSGGridView::TopToBottom)
            rowOffset += gridItem->item->width()-cellWidth;
        else
            colOffset = gridItem->item->width()-cellWidth;
    }
    if (visibleItems.count()) {
        qreal startPos = originPosition();
        if (visibleIndex == 0) {
            gridItem->setPosition(colOffset, startPos + rowOffset);
        } else {
            qreal tempPos = isRightToLeftTopToBottom() ? -position()-size() : position();
            qreal headerPos = isRightToLeftTopToBottom() ? gridItem->rowPos() + cellWidth - headerSize() : gridItem->rowPos();
            if (tempPos <= startPos || headerPos > startPos + rowOffset)
                gridItem->setPosition(colOffset, startPos + rowOffset);
        }
    } else {
        if (isRightToLeftTopToBottom())
            gridItem->setPosition(colOffset, rowOffset);
        else
            gridItem->setPosition(colOffset, -headerSize());
    }

    if (created)
        emit q->headerItemChanged();
}

void QSGGridViewPrivate::initializeCurrentItem()
{
    if (currentItem && currentIndex >= 0) {
        FxGridItemSG *gridItem = static_cast<FxGridItemSG*>(currentItem);
        if (gridItem)
            gridItem->setPosition(colPosAt(currentIndex), rowPosAt(currentIndex));
    }
}

void QSGGridViewPrivate::itemGeometryChanged(QSGItem *item, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_Q(QSGGridView);
    QSGItemViewPrivate::itemGeometryChanged(item, newGeometry, oldGeometry);
    if (!q->isComponentComplete())
        return;
    if (item == q) {
        if (newGeometry.height() != oldGeometry.height() || newGeometry.width() != oldGeometry.width()) {
            updateViewport();
            q->polish();
        }
    }
}

void QSGGridViewPrivate::fixupPosition()
{
    moveReason = Other;
    if (flow == QSGGridView::LeftToRight)
        fixupY();
    else
        fixupX();
}

void QSGGridViewPrivate::fixup(AxisData &data, qreal minExtent, qreal maxExtent)
{
    if ((flow == QSGGridView::TopToBottom && &data == &vData)
        || (flow == QSGGridView::LeftToRight && &data == &hData))
        return;

    fixupMode = moveReason == Mouse ? fixupMode : Immediate;

    qreal highlightStart;
    qreal highlightEnd;
    qreal viewPos;
    if (isRightToLeftTopToBottom()) {
        // Handle Right-To-Left exceptions
        viewPos = -position()-size();
        highlightStart = highlightRangeStartValid ? size()-highlightRangeEnd : highlightRangeStart;
        highlightEnd = highlightRangeEndValid ? size()-highlightRangeStart : highlightRangeEnd;
    } else {
        viewPos = position();
        highlightStart = highlightRangeStart;
        highlightEnd = highlightRangeEnd;
    }

    bool strictHighlightRange = haveHighlightRange && highlightRange == QSGGridView::StrictlyEnforceRange;
    if (snapMode != QSGGridView::NoSnap) {
        qreal tempPosition = isRightToLeftTopToBottom() ? -position()-size() : position();
        if (snapMode == QSGGridView::SnapOneRow && moveReason == Mouse) {
            // if we've been dragged < rowSize()/2 then bias towards the next row
            qreal dist = data.move.value() - (data.pressPos - data.dragStartOffset);
            qreal bias = 0;
            if (data.velocity > 0 && dist > QML_FLICK_SNAPONETHRESHOLD && dist < rowSize()/2)
                bias = rowSize()/2;
            else if (data.velocity < 0 && dist < -QML_FLICK_SNAPONETHRESHOLD && dist > -rowSize()/2)
                bias = -rowSize()/2;
            if (isRightToLeftTopToBottom())
                bias = -bias;
            tempPosition -= bias;
        }
        FxViewItem *topItem = snapItemAt(tempPosition+highlightStart);
        if (!topItem && strictHighlightRange && currentItem) {
            // StrictlyEnforceRange always keeps an item in range
            updateHighlight();
            topItem = currentItem;
        }
        FxViewItem *bottomItem = snapItemAt(tempPosition+highlightEnd);
        if (!bottomItem && strictHighlightRange && currentItem) {
            // StrictlyEnforceRange always keeps an item in range
            updateHighlight();
            bottomItem = currentItem;
        }
        qreal pos;
        if (topItem && bottomItem && strictHighlightRange) {
            qreal topPos = qMin(topItem->position() - highlightStart, -maxExtent);
            qreal bottomPos = qMax(bottomItem->position() - highlightEnd, -minExtent);
            pos = qAbs(data.move + topPos) < qAbs(data.move + bottomPos) ? topPos : bottomPos;
        } else if (topItem) {
            qreal headerPos = 0;
            if (header)
                headerPos = isRightToLeftTopToBottom() ? static_cast<FxGridItemSG*>(header)->rowPos() + cellWidth - headerSize() : static_cast<FxGridItemSG*>(header)->rowPos();
            if (topItem->index == 0 && header && tempPosition+highlightStart < headerPos+headerSize()/2 && !strictHighlightRange) {
                pos = isRightToLeftTopToBottom() ? - headerPos + highlightStart - size() : headerPos - highlightStart;
            } else {
                if (isRightToLeftTopToBottom())
                    pos = qMax(qMin(-topItem->position() + highlightStart - size(), -maxExtent), -minExtent);
                else
                    pos = qMax(qMin(topItem->position() - highlightStart, -maxExtent), -minExtent);
            }
        } else if (bottomItem) {
            if (isRightToLeftTopToBottom())
                pos = qMax(qMin(-bottomItem->position() + highlightEnd - size(), -maxExtent), -minExtent);
            else
                pos = qMax(qMin(bottomItem->position() - highlightEnd, -maxExtent), -minExtent);
        } else {
            QSGItemViewPrivate::fixup(data, minExtent, maxExtent);
            return;
        }

        qreal dist = qAbs(data.move + pos);
        if (dist > 0) {
            timeline.reset(data.move);
            if (fixupMode != Immediate) {
                timeline.move(data.move, -pos, QEasingCurve(QEasingCurve::InOutQuad), fixupDuration/2);
                data.fixingUp = true;
            } else {
                timeline.set(data.move, -pos);
            }
            vTime = timeline.time();
        }
    } else if (haveHighlightRange && highlightRange == QSGGridView::StrictlyEnforceRange) {
        if (currentItem) {
            updateHighlight();
            qreal pos = static_cast<FxGridItemSG*>(currentItem)->rowPos();
            if (viewPos < pos + rowSize() - highlightEnd)
                viewPos = pos + rowSize() - highlightEnd;
            if (viewPos > pos - highlightStart)
                viewPos = pos - highlightStart;
            if (isRightToLeftTopToBottom())
                viewPos = -viewPos-size();
            timeline.reset(data.move);
            if (viewPos != position()) {
                if (fixupMode != Immediate) {
                    timeline.move(data.move, -viewPos, QEasingCurve(QEasingCurve::InOutQuad), fixupDuration/2);
                    data.fixingUp = true;
                } else {
                    timeline.set(data.move, -viewPos);
                }
            }
            vTime = timeline.time();
        }
    } else {
        QSGItemViewPrivate::fixup(data, minExtent, maxExtent);
    }
    data.inOvershoot = false;
    fixupMode = Normal;
}

void QSGGridViewPrivate::flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                                        QDeclarativeTimeLineCallback::Callback fixupCallback, qreal velocity)
{
    Q_Q(QSGGridView);
    data.fixingUp = false;
    moveReason = Mouse;
    if ((!haveHighlightRange || highlightRange != QSGGridView::StrictlyEnforceRange)
        && snapMode == QSGGridView::NoSnap) {
        QSGItemViewPrivate::flick(data, minExtent, maxExtent, vSize, fixupCallback, velocity);
        return;
    }
    qreal maxDistance = 0;
    qreal dataValue = isRightToLeftTopToBottom() ? -data.move.value()+size() : data.move.value();
    // -ve velocity means list is moving up/left
    if (velocity > 0) {
        if (data.move.value() < minExtent) {
            if (snapMode == QSGGridView::SnapOneRow) {
                // if we've been dragged < averageSize/2 then bias towards the next item
                qreal dist = data.move.value() - (data.pressPos - data.dragStartOffset);
                qreal bias = dist < rowSize()/2 ? rowSize()/2 : 0;
                if (isRightToLeftTopToBottom())
                    bias = -bias;
                data.flickTarget = -snapPosAt(-dataValue - bias);
                maxDistance = qAbs(data.flickTarget - data.move.value());
                velocity = maxVelocity;
            } else {
                maxDistance = qAbs(minExtent - data.move.value());
            }
        }
        if (snapMode == QSGGridView::NoSnap && highlightRange != QSGGridView::StrictlyEnforceRange)
            data.flickTarget = minExtent;
    } else {
        if (data.move.value() > maxExtent) {
            if (snapMode == QSGGridView::SnapOneRow) {
                // if we've been dragged < averageSize/2 then bias towards the next item
                qreal dist = data.move.value() - (data.pressPos - data.dragStartOffset);
                qreal bias = -dist < rowSize()/2 ? rowSize()/2 : 0;
                if (isRightToLeftTopToBottom())
                    bias = -bias;
                data.flickTarget = -snapPosAt(-dataValue + bias);
                maxDistance = qAbs(data.flickTarget - data.move.value());
                velocity = -maxVelocity;
            } else {
                maxDistance = qAbs(maxExtent - data.move.value());
            }
        }
        if (snapMode == QSGGridView::NoSnap && highlightRange != QSGGridView::StrictlyEnforceRange)
            data.flickTarget = maxExtent;
    }
    bool overShoot = boundsBehavior == QSGFlickable::DragAndOvershootBounds;
    if (maxDistance > 0 || overShoot) {
        // This mode requires the grid to stop exactly on a row boundary.
        qreal v = velocity;
        if (maxVelocity != -1 && maxVelocity < qAbs(v)) {
            if (v < 0)
                v = -maxVelocity;
            else
                v = maxVelocity;
        }
        qreal accel = deceleration;
        qreal v2 = v * v;
        qreal overshootDist = 0.0;
        if ((maxDistance > 0.0 && v2 / (2.0f * maxDistance) < accel) || snapMode == QSGGridView::SnapOneRow) {
            // + rowSize()/4 to encourage moving at least one item in the flick direction
            qreal dist = v2 / (accel * 2.0) + rowSize()/4;
            dist = qMin(dist, maxDistance);
            if (v > 0)
                dist = -dist;
            if (snapMode != QSGGridView::SnapOneRow) {
                qreal distTemp = isRightToLeftTopToBottom() ? -dist : dist;
                data.flickTarget = -snapPosAt(-dataValue + distTemp);
            }
            data.flickTarget = isRightToLeftTopToBottom() ? -data.flickTarget+size() : data.flickTarget;
            if (overShoot) {
                if (data.flickTarget >= minExtent) {
                    overshootDist = overShootDistance(vSize);
                    data.flickTarget += overshootDist;
                } else if (data.flickTarget <= maxExtent) {
                    overshootDist = overShootDistance(vSize);
                    data.flickTarget -= overshootDist;
                }
            }
            qreal adjDist = -data.flickTarget + data.move.value();
            if (qAbs(adjDist) > qAbs(dist)) {
                // Prevent painfully slow flicking - adjust velocity to suit flickDeceleration
                qreal adjv2 = accel * 2.0f * qAbs(adjDist);
                if (adjv2 > v2) {
                    v2 = adjv2;
                    v = qSqrt(v2);
                    if (dist > 0)
                        v = -v;
                }
            }
            dist = adjDist;
            accel = v2 / (2.0f * qAbs(dist));
        } else {
            data.flickTarget = velocity > 0 ? minExtent : maxExtent;
            overshootDist = overShoot ? overShootDistance(vSize) : 0;
        }
        timeline.reset(data.move);
        timeline.accel(data.move, v, accel, maxDistance + overshootDist);
        timeline.callback(QDeclarativeTimeLineCallback(&data.move, fixupCallback, this));
        if (!hData.flicking && q->xflick()) {
            hData.flicking = true;
            emit q->flickingChanged();
            emit q->flickingHorizontallyChanged();
            emit q->flickStarted();
        }
        if (!vData.flicking && q->yflick()) {
            vData.flicking = true;
            emit q->flickingChanged();
            emit q->flickingVerticallyChanged();
            emit q->flickStarted();
        }
    } else {
        timeline.reset(data.move);
        fixup(data, minExtent, maxExtent);
    }
}


//----------------------------------------------------------------------------
/*!
    \qmlclass GridView QSGGridView
    \inqmlmodule QtQuick 2
    \ingroup qml-view-elements

    \inherits Flickable
    \brief The GridView item provides a grid view of items provided by a model.

    A GridView displays data from models created from built-in QML elements like ListModel
    and XmlListModel, or custom model classes defined in C++ that inherit from
    QAbstractListModel.

    A GridView has a \l model, which defines the data to be displayed, and
    a \l delegate, which defines how the data should be displayed. Items in a
    GridView are laid out horizontally or vertically. Grid views are inherently flickable
    as GridView inherits from \l Flickable.

    \section1 Example Usage

    The following example shows the definition of a simple list model defined
    in a file called \c ContactModel.qml:

    \snippet doc/src/snippets/declarative/gridview/ContactModel.qml 0

    \div {class="float-right"}
    \inlineimage gridview-simple.png
    \enddiv

    This model can be referenced as \c ContactModel in other QML files. See \l{QML Modules}
    for more information about creating reusable components like this.

    Another component can display this model data in a GridView, as in the following
    example, which creates a \c ContactModel component for its model, and a \l Column element
    (containing \l Image and \l Text elements) for its delegate.

    \clearfloat
    \snippet doc/src/snippets/declarative/gridview/gridview.qml import
    \codeline
    \snippet doc/src/snippets/declarative/gridview/gridview.qml classdocs simple

    \div {class="float-right"}
    \inlineimage gridview-highlight.png
    \enddiv

    The view will create a new delegate for each item in the model. Note that the delegate
    is able to access the model's \c name and \c portrait data directly.

    An improved grid view is shown below. The delegate is visually improved and is moved
    into a separate \c contactDelegate component.

    \clearfloat
    \snippet doc/src/snippets/declarative/gridview/gridview.qml classdocs advanced

    The currently selected item is highlighted with a blue \l Rectangle using the \l highlight property,
    and \c focus is set to \c true to enable keyboard navigation for the grid view.
    The grid view itself is a focus scope (see \l{qmlfocus#Acquiring Focus and Focus Scopes}{the focus documentation page} for more details).

    Delegates are instantiated as needed and may be destroyed at any time.
    State should \e never be stored in a delegate.

    GridView attaches a number of properties to the root item of the delegate, for example
    \c {GridView.isCurrentItem}.  In the following example, the root delegate item can access
    this attached property directly as \c GridView.isCurrentItem, while the child
    \c contactInfo object must refer to this property as \c wrapper.GridView.isCurrentItem.

    \snippet doc/src/snippets/declarative/gridview/gridview.qml isCurrentItem

    \note Views do not set the \l{Item::}{clip} property automatically.
    If the view is not clipped by another item or the screen, it will be necessary
    to set this property to true in order to clip the items that are partially or
    fully outside the view.

    \sa {declarative/modelviews/gridview}{GridView example}
*/

QSGGridView::QSGGridView(QSGItem *parent)
    : QSGItemView(*(new QSGGridViewPrivate), parent)
{
}

QSGGridView::~QSGGridView()
{
}

void QSGGridView::setHighlightFollowsCurrentItem(bool autoHighlight)
{
    Q_D(QSGGridView);
    if (d->autoHighlight != autoHighlight) {
        if (!autoHighlight && d->highlightXAnimator) {
            d->highlightXAnimator->stop();
            d->highlightYAnimator->stop();
        }
        QSGItemView::setHighlightFollowsCurrentItem(autoHighlight);
    }
}

/*!
    \qmlattachedproperty bool QtQuick2::GridView::isCurrentItem
    This attached property is true if this delegate is the current item; otherwise false.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty GridView QtQuick2::GridView::view
    This attached property holds the view that manages this delegate instance.

    It is attached to each instance of the delegate.

    \snippet doc/src/snippets/declarative/gridview/gridview.qml isCurrentItem
*/

/*!
    \qmlattachedproperty bool QtQuick2::GridView::delayRemove
    This attached property holds whether the delegate may be destroyed.

    It is attached to each instance of the delegate.

    It is sometimes necessary to delay the destruction of an item
    until an animation completes.

    The example below ensures that the animation completes before
    the item is removed from the grid.

    \snippet doc/src/snippets/declarative/gridview/gridview.qml delayRemove
*/

/*!
    \qmlattachedsignal QtQuick2::GridView::onAdd()
    This attached handler is called immediately after an item is added to the view.
*/

/*!
    \qmlattachedsignal QtQuick2::GridView::onRemove()
    This attached handler is called immediately before an item is removed from the view.
*/


/*!
  \qmlproperty model QtQuick2::GridView::model
  This property holds the model providing data for the grid.

    The model provides the set of data that is used to create the items
    in the view. Models can be created directly in QML using \l ListModel, \l XmlListModel
    or \l VisualItemModel, or provided by C++ model classes. If a C++ model class is
    used, it must be a subclass of \l QAbstractItemModel or a simple list.

  \sa {qmlmodels}{Data Models}
*/

/*!
    \qmlproperty Component QtQuick2::GridView::delegate

    The delegate provides a template defining each item instantiated by the view.
    The index is exposed as an accessible \c index property.  Properties of the
    model are also available depending upon the type of \l {qmlmodels}{Data Model}.

    The number of elements in the delegate has a direct effect on the
    flicking performance of the view.  If at all possible, place functionality
    that is not needed for the normal display of the delegate in a \l Loader which
    can load additional elements when needed.

    The GridView will layout the items based on the size of the root item
    in the delegate.

    \note Delegates are instantiated as needed and may be destroyed at any time.
    State should \e never be stored in a delegate.
*/

/*!
  \qmlproperty int QtQuick2::GridView::currentIndex
  \qmlproperty Item QtQuick2::GridView::currentItem

    The \c currentIndex property holds the index of the current item, and
    \c currentItem holds the current item.  Setting the currentIndex to -1
    will clear the highlight and set currentItem to null.

    If highlightFollowsCurrentItem is \c true, setting either of these
    properties will smoothly scroll the GridView so that the current
    item becomes visible.

    Note that the position of the current item
    may only be approximate until it becomes visible in the view.
*/


/*!
  \qmlproperty Item QtQuick2::GridView::highlightItem

  This holds the highlight item created from the \l highlight component.

  The highlightItem is managed by the view unless
  \l highlightFollowsCurrentItem is set to false.

  \sa highlight, highlightFollowsCurrentItem
*/


/*!
  \qmlproperty int QtQuick2::GridView::count
  This property holds the number of items in the view.
*/


/*!
  \qmlproperty Component QtQuick2::GridView::highlight
  This property holds the component to use as the highlight.

  An instance of the highlight component is created for each view.
  The geometry of the resulting component instance will be managed by the view
  so as to stay with the current item, unless the highlightFollowsCurrentItem property is false.

  \sa highlightItem, highlightFollowsCurrentItem
*/

/*!
  \qmlproperty bool QtQuick2::GridView::highlightFollowsCurrentItem
  This property sets whether the highlight is managed by the view.

    If this property is true (the default value), the highlight is moved smoothly
    to follow the current item.  Otherwise, the
    highlight is not moved by the view, and any movement must be implemented
    by the highlight.

    Here is a highlight with its motion defined by a \l {SpringAnimation} item:

    \snippet doc/src/snippets/declarative/gridview/gridview.qml highlightFollowsCurrentItem
*/


/*!
    \qmlproperty int QtQuick2::GridView::highlightMoveDuration
    This property holds the move animation duration of the highlight delegate.

    highlightFollowsCurrentItem must be true for this property
    to have effect.

    The default value for the duration is 150ms.

    \sa highlightFollowsCurrentItem
*/

/*!
    \qmlproperty real QtQuick2::GridView::preferredHighlightBegin
    \qmlproperty real QtQuick2::GridView::preferredHighlightEnd
    \qmlproperty enumeration QtQuick2::GridView::highlightRangeMode

    These properties define the preferred range of the highlight (for the current item)
    within the view. The \c preferredHighlightBegin value must be less than the
    \c preferredHighlightEnd value.

    These properties affect the position of the current item when the view is scrolled.
    For example, if the currently selected item should stay in the middle of the
    view when it is scrolled, set the \c preferredHighlightBegin and
    \c preferredHighlightEnd values to the top and bottom coordinates of where the middle
    item would be. If the \c currentItem is changed programmatically, the view will
    automatically scroll so that the current item is in the middle of the view.
    Furthermore, the behavior of the current item index will occur whether or not a
    highlight exists.

    Valid values for \c highlightRangeMode are:

    \list
    \o GridView.ApplyRange - the view attempts to maintain the highlight within the range.
       However, the highlight can move outside of the range at the ends of the view or due
       to mouse interaction.
    \o GridView.StrictlyEnforceRange - the highlight never moves outside of the range.
       The current item changes if a keyboard or mouse action would cause the highlight to move
       outside of the range.
    \o GridView.NoHighlightRange - this is the default value.
    \endlist
*/


/*!
  \qmlproperty enumeration QtQuick2::GridView::layoutDirection
  This property holds the layout direction of the grid.

    Possible values:

  \list
  \o Qt.LeftToRight (default) - Items will be laid out starting in the top, left corner. The flow is
  dependent on the \l GridView::flow property.
  \o Qt.RightToLeft - Items will be laid out starting in the top, right corner. The flow is dependent
  on the \l GridView::flow property.
  \endlist

  \bold Note: If GridView::flow is set to GridView.LeftToRight, this is not to be confused if
  GridView::layoutDirection is set to Qt.RightToLeft. The GridView.LeftToRight flow value simply
  indicates that the flow is horizontal.
*/


/*!
    \qmlproperty enumeration QtQuick2::GridView::effectiveLayoutDirection
    This property holds the effective layout direction of the grid.

    When using the attached property \l {LayoutMirroring::enabled}{LayoutMirroring::enabled} for locale layouts,
    the visual layout direction of the grid will be mirrored. However, the
    property \l {GridView::layoutDirection}{layoutDirection} will remain unchanged.

    \sa GridView::layoutDirection, {LayoutMirroring}{LayoutMirroring}
*/
/*!
  \qmlproperty bool QtQuick2::GridView::keyNavigationWraps
  This property holds whether the grid wraps key navigation

    If this is true, key navigation that would move the current item selection
    past one end of the view instead wraps around and moves the selection to
    the other end of the view.

    By default, key navigation is not wrapped.
*/
/*!
    \qmlproperty int QtQuick2::GridView::cacheBuffer
    This property determines whether delegates are retained outside the
    visible area of the view.

    If non-zero the view will keep as many delegates
    instantiated as will fit within the buffer specified.  For example,
    if in a vertical view the delegate is 20 pixels high and \c cacheBuffer is
    set to 40, then up to 2 delegates above and 2 delegates below the visible
    area may be retained.

    Note that cacheBuffer is not a pixel buffer - it only maintains additional
    instantiated delegates.

    Setting this value can make scrolling the list smoother at the expense
    of additional memory usage.  It is not a substitute for creating efficient
    delegates; the fewer elements in a delegate, the faster a view may be
    scrolled.
*/
void QSGGridView::setHighlightMoveDuration(int duration)
{
    Q_D(QSGGridView);
    if (d->highlightMoveDuration != duration) {
        if (d->highlightYAnimator) {
            d->highlightXAnimator->userDuration = duration;
            d->highlightYAnimator->userDuration = duration;
        }
        QSGItemView::setHighlightMoveDuration(duration);
    }
}

/*!
  \qmlproperty enumeration QtQuick2::GridView::flow
  This property holds the flow of the grid.

    Possible values:

    \list
    \o GridView.LeftToRight (default) - Items are laid out from left to right, and the view scrolls vertically
    \o GridView.TopToBottom - Items are laid out from top to bottom, and the view scrolls horizontally
    \endlist
*/
QSGGridView::Flow QSGGridView::flow() const
{
    Q_D(const QSGGridView);
    return d->flow;
}

void QSGGridView::setFlow(Flow flow)
{
    Q_D(QSGGridView);
    if (d->flow != flow) {
        d->flow = flow;
        if (d->flow == LeftToRight) {
            setContentWidth(-1);
            setFlickableDirection(VerticalFlick);
        } else {
            setContentHeight(-1);
            setFlickableDirection(HorizontalFlick);
        }
        setContentX(0);
        setContentY(0);
        d->regenerate();
        emit flowChanged();
    }
}


/*!
  \qmlproperty real QtQuick2::GridView::cellWidth
  \qmlproperty real QtQuick2::GridView::cellHeight

  These properties holds the width and height of each cell in the grid.

  The default cell size is 100x100.
*/
qreal QSGGridView::cellWidth() const
{
    Q_D(const QSGGridView);
    return d->cellWidth;
}

void QSGGridView::setCellWidth(qreal cellWidth)
{
    Q_D(QSGGridView);
    if (cellWidth != d->cellWidth && cellWidth > 0) {
        d->cellWidth = qMax(qreal(1), cellWidth);
        d->updateViewport();
        emit cellWidthChanged();
        d->forceLayout = true;
        d->layout();
    }
}

qreal QSGGridView::cellHeight() const
{
    Q_D(const QSGGridView);
    return d->cellHeight;
}

void QSGGridView::setCellHeight(qreal cellHeight)
{
    Q_D(QSGGridView);
    if (cellHeight != d->cellHeight && cellHeight > 0) {
        d->cellHeight = qMax(qreal(1), cellHeight);
        d->updateViewport();
        emit cellHeightChanged();
        d->forceLayout = true;
        d->layout();
    }
}
/*!
    \qmlproperty enumeration QtQuick2::GridView::snapMode

    This property determines how the view scrolling will settle following a drag or flick.
    The possible values are:

    \list
    \o GridView.NoSnap (default) - the view stops anywhere within the visible area.
    \o GridView.SnapToRow - the view settles with a row (or column for \c GridView.TopToBottom flow)
    aligned with the start of the view.
    \o GridView.SnapOneRow - the view will settle no more than one row (or column for \c GridView.TopToBottom flow)
    away from the first visible row at the time the mouse button is released.
    This mode is particularly useful for moving one page at a time.
    \endlist

*/
QSGGridView::SnapMode QSGGridView::snapMode() const
{
    Q_D(const QSGGridView);
    return d->snapMode;
}

void QSGGridView::setSnapMode(SnapMode mode)
{
    Q_D(QSGGridView);
    if (d->snapMode != mode) {
        d->snapMode = mode;
        emit snapModeChanged();
    }
}


/*!
    \qmlproperty Component QtQuick2::GridView::footer
    This property holds the component to use as the footer.

    An instance of the footer component is created for each view.  The
    footer is positioned at the end of the view, after any items.

    \sa header
*/
/*!
    \qmlproperty Component QtQuick2::GridView::header
    This property holds the component to use as the header.

    An instance of the header component is created for each view.  The
    header is positioned at the beginning of the view, before any items.

    \sa footer
*/
void QSGGridView::viewportMoved()
{
    Q_D(QSGGridView);
    QSGItemView::viewportMoved();
    if (!d->itemCount)
        return;
    if (d->inViewportMoved)
        return;
    d->inViewportMoved = true;

    d->lazyRelease = true;
    if (d->hData.flicking || d->vData.flicking) {
        if (yflick()) {
            if (d->vData.velocity > 0)
                d->bufferMode = QSGGridViewPrivate::BufferBefore;
            else if (d->vData.velocity < 0)
                d->bufferMode = QSGGridViewPrivate::BufferAfter;
        }

        if (xflick()) {
            if (d->hData.velocity > 0)
                d->bufferMode = QSGGridViewPrivate::BufferBefore;
            else if (d->hData.velocity < 0)
                d->bufferMode = QSGGridViewPrivate::BufferAfter;
        }
    }
    d->refill();
    if (d->hData.flicking || d->vData.flicking || d->hData.moving || d->vData.moving)
        d->moveReason = QSGGridViewPrivate::Mouse;
    if (d->moveReason != QSGGridViewPrivate::SetIndex) {
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange && d->highlight) {
            // reposition highlight
            qreal pos = d->highlight->position();
            qreal viewPos;
            qreal highlightStart;
            qreal highlightEnd;
            if (d->isRightToLeftTopToBottom()) {
                viewPos = -d->position()-d->size();
                highlightStart = d->highlightRangeStartValid ? d->size()-d->highlightRangeEnd : d->highlightRangeStart;
                highlightEnd = d->highlightRangeEndValid ? d->size()-d->highlightRangeStart : d->highlightRangeEnd;
            } else {
                viewPos = d->position();
                highlightStart = d->highlightRangeStart;
                highlightEnd = d->highlightRangeEnd;
            }
            if (pos > viewPos + highlightEnd - d->highlight->size())
                pos = viewPos + highlightEnd - d->highlight->size();
            if (pos < viewPos + highlightStart)
                pos = viewPos + highlightStart;

            if (pos != d->highlight->position()) {
                d->highlightXAnimator->stop();
                d->highlightYAnimator->stop();
                static_cast<FxGridItemSG*>(d->highlight)->setPosition(static_cast<FxGridItemSG*>(d->highlight)->colPos(), pos);
            } else {
                d->updateHighlight();
            }

            // update current index
            int idx = d->snapIndex();
            if (idx >= 0 && idx != d->currentIndex) {
                d->updateCurrent(idx);
                if (d->currentItem && static_cast<FxGridItemSG*>(d->currentItem)->colPos() != static_cast<FxGridItemSG*>(d->highlight)->colPos() && d->autoHighlight) {
                    if (d->flow == LeftToRight)
                        d->highlightXAnimator->to = d->currentItem->item->x();
                    else
                        d->highlightYAnimator->to = d->currentItem->item->y();
                }
            }
        }
    }

    d->inViewportMoved = false;
}

void QSGGridView::keyPressEvent(QKeyEvent *event)
{
    Q_D(QSGGridView);
    if (d->model && d->model->count() && d->interactive) {
        d->moveReason = QSGGridViewPrivate::SetIndex;
        int oldCurrent = currentIndex();
        switch (event->key()) {
        case Qt::Key_Up:
            moveCurrentIndexUp();
            break;
        case Qt::Key_Down:
            moveCurrentIndexDown();
            break;
        case Qt::Key_Left:
            moveCurrentIndexLeft();
            break;
        case Qt::Key_Right:
            moveCurrentIndexRight();
            break;
        default:
            break;
        }
        if (oldCurrent != currentIndex()) {
            event->accept();
            return;
        }
    }
    event->ignore();
    QSGItemView::keyPressEvent(event);
}
/*!
    \qmlmethod QtQuick2::GridView::moveCurrentIndexUp()

    Move the currentIndex up one item in the view.
    The current index will wrap if keyNavigationWraps is true and it
    is currently at the end. This method has no effect if the \l count is zero.

    \bold Note: methods should only be called after the Component has completed.
*/


void QSGGridView::moveCurrentIndexUp()
{
    Q_D(QSGGridView);
    const int count = d->model ? d->model->count() : 0;
    if (!count)
        return;
    if (d->flow == QSGGridView::LeftToRight) {
        if (currentIndex() >= d->columns || d->wrap) {
            int index = currentIndex() - d->columns;
            setCurrentIndex((index >= 0 && index < count) ? index : count-1);
        }
    } else {
        if (currentIndex() > 0 || d->wrap) {
            int index = currentIndex() - 1;
            setCurrentIndex((index >= 0 && index < count) ? index : count-1);
        }
    }
}

/*!
    \qmlmethod QtQuick2::GridView::moveCurrentIndexDown()

    Move the currentIndex down one item in the view.
    The current index will wrap if keyNavigationWraps is true and it
    is currently at the end. This method has no effect if the \l count is zero.

    \bold Note: methods should only be called after the Component has completed.
*/
void QSGGridView::moveCurrentIndexDown()
{
    Q_D(QSGGridView);
    const int count = d->model ? d->model->count() : 0;
    if (!count)
        return;
    if (d->flow == QSGGridView::LeftToRight) {
        if (currentIndex() < count - d->columns || d->wrap) {
            int index = currentIndex()+d->columns;
            setCurrentIndex((index >= 0 && index < count) ? index : 0);
        }
    } else {
        if (currentIndex() < count - 1 || d->wrap) {
            int index = currentIndex() + 1;
            setCurrentIndex((index >= 0 && index < count) ? index : 0);
        }
    }
}

/*!
    \qmlmethod QtQuick2::GridView::moveCurrentIndexLeft()

    Move the currentIndex left one item in the view.
    The current index will wrap if keyNavigationWraps is true and it
    is currently at the end. This method has no effect if the \l count is zero.

    \bold Note: methods should only be called after the Component has completed.
*/
void QSGGridView::moveCurrentIndexLeft()
{
    Q_D(QSGGridView);
    const int count = d->model ? d->model->count() : 0;
    if (!count)
        return;
    if (effectiveLayoutDirection() == Qt::LeftToRight) {
        if (d->flow == QSGGridView::LeftToRight) {
            if (currentIndex() > 0 || d->wrap) {
                int index = currentIndex() - 1;
                setCurrentIndex((index >= 0 && index < count) ? index : count-1);
            }
        } else {
            if (currentIndex() >= d->columns || d->wrap) {
                int index = currentIndex() - d->columns;
                setCurrentIndex((index >= 0 && index < count) ? index : count-1);
            }
        }
    } else {
        if (d->flow == QSGGridView::LeftToRight) {
            if (currentIndex() < count - 1 || d->wrap) {
                int index = currentIndex() + 1;
                setCurrentIndex((index >= 0 && index < count) ? index : 0);
            }
        } else {
            if (currentIndex() < count - d->columns || d->wrap) {
                int index = currentIndex() + d->columns;
                setCurrentIndex((index >= 0 && index < count) ? index : 0);
            }
        }
    }
}


/*!
    \qmlmethod QtQuick2::GridView::moveCurrentIndexRight()

    Move the currentIndex right one item in the view.
    The current index will wrap if keyNavigationWraps is true and it
    is currently at the end. This method has no effect if the \l count is zero.

    \bold Note: methods should only be called after the Component has completed.
*/
void QSGGridView::moveCurrentIndexRight()
{
    Q_D(QSGGridView);
    const int count = d->model ? d->model->count() : 0;
    if (!count)
        return;
    if (effectiveLayoutDirection() == Qt::LeftToRight) {
        if (d->flow == QSGGridView::LeftToRight) {
            if (currentIndex() < count - 1 || d->wrap) {
                int index = currentIndex() + 1;
                setCurrentIndex((index >= 0 && index < count) ? index : 0);
            }
        } else {
            if (currentIndex() < count - d->columns || d->wrap) {
                int index = currentIndex()+d->columns;
                setCurrentIndex((index >= 0 && index < count) ? index : 0);
            }
        }
    } else {
        if (d->flow == QSGGridView::LeftToRight) {
            if (currentIndex() > 0 || d->wrap) {
                int index = currentIndex() - 1;
                setCurrentIndex((index >= 0 && index < count) ? index : count-1);
            }
        } else {
            if (currentIndex() >= d->columns || d->wrap) {
                int index = currentIndex() - d->columns;
                setCurrentIndex((index >= 0 && index < count) ? index : count-1);
            }
        }
    }
}

bool QSGGridViewPrivate::applyInsertionChange(const QDeclarativeChangeSet::Insert &change, QList<FxViewItem *> *movedBackwards, QList<FxViewItem *> *addedItems, FxViewItem *firstVisible)
{
    Q_Q(QSGGridView);

    int modelIndex = change.index;
    int count = change.count;

    int index = visibleItems.count() ? mapFromModel(modelIndex) : 0;

    if (index < 0) {
        int i = visibleItems.count() - 1;
        while (i > 0 && visibleItems.at(i)->index == -1)
            --i;
        if (visibleItems.at(i)->index + 1 == modelIndex) {
            // Special case of appending an item to the model.
            index = visibleIndex + visibleItems.count();
        } else {
            if (modelIndex <= visibleIndex) {
                // Insert before visible items
                visibleIndex += count;
                for (int i = 0; i < visibleItems.count(); ++i) {
                    FxViewItem *item = visibleItems.at(i);
                    if (item->index != -1 && item->index >= modelIndex)
                        item->index += count;
                }
            }
            return true;
        }
    }

    int insertCount = count;
    if (index < visibleIndex && visibleItems.count()) {
        insertCount -= visibleIndex - index;
        index = visibleIndex;
        modelIndex = visibleIndex;
    }

    qreal tempPos = isRightToLeftTopToBottom() ? -position()-size()+q->width()+1 : position();
    int to = buffer+tempPos+size()-1;
    int colPos = 0;
    int rowPos = 0;
    if (visibleItems.count()) {
        index -= visibleIndex;
        if (index < visibleItems.count()) {
            FxGridItemSG *gridItem = static_cast<FxGridItemSG*>(visibleItems.at(index));
            colPos = gridItem->colPos();
            rowPos = gridItem->rowPos();
        } else {
            // appending items to visible list
            FxGridItemSG *gridItem = static_cast<FxGridItemSG*>(visibleItems.at(index-1));
            colPos = gridItem->colPos() + colSize();
            rowPos = gridItem->rowPos();
            if (colPos > colSize() * (columns-1)) {
                colPos = 0;
                rowPos += rowSize();
            }
        }
    }

    // Update the indexes of the following visible items.
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index != -1 && item->index >= modelIndex)
            item->index += count;
    }

    int i = 0;
    bool prevAddedCount = addedItems->count();
    while (i < insertCount && rowPos <= to + rowSize()*(columns - (colPos/colSize()))/qreal(columns)) {
        FxViewItem *item = 0;
        if (change.isMove() && (item = currentChanges.removedItems.take(change.moveKey(modelIndex + i)))) {
            if (item->index > modelIndex + i)
                movedBackwards->append(item);
            item->index = modelIndex + i;
        }
        if (!item)
            item = createItem(modelIndex + i);
        visibleItems.insert(index, item);
        if (!change.isMove())
            addedItems->append(item);
        colPos += colSize();
        if (colPos > colSize() * (columns-1)) {
            colPos = 0;
            rowPos += rowSize();
        }
        ++index;
        ++i;
    }

    return addedItems->count() > prevAddedCount;
}

/*!
    \qmlmethod QtQuick2::GridView::positionViewAtIndex(int index, PositionMode mode)

    Positions the view such that the \a index is at the position specified by
    \a mode:

    \list
    \o GridView.Beginning - position item at the top (or left for \c GridView.TopToBottom flow) of the view.
    \o GridView.Center - position item in the center of the view.
    \o GridView.End - position item at bottom (or right for horizontal orientation) of the view.
    \o GridView.Visible - if any part of the item is visible then take no action, otherwise
    bring the item into view.
    \o GridView.Contain - ensure the entire item is visible.  If the item is larger than
    the view the item is positioned at the top (or left for \c GridView.TopToBottom flow) of the view.
    \endlist

    If positioning the view at the index would cause empty space to be displayed at
    the beginning or end of the view, the view will be positioned at the boundary.

    It is not recommended to use \l {Flickable::}{contentX} or \l {Flickable::}{contentY} to position the view
    at a particular index.  This is unreliable since removing items from the start
    of the view does not cause all other items to be repositioned.
    The correct way to bring an item into view is with \c positionViewAtIndex.

    \bold Note: methods should only be called after the Component has completed.  To position
    the view at startup, this method should be called by Component.onCompleted.  For
    example, to position the view at the end:

    \code
    Component.onCompleted: positionViewAtIndex(count - 1, GridView.Beginning)
    \endcode
*/

/*!
    \qmlmethod QtQuick2::GridView::positionViewAtBeginning()
    \qmlmethod QtQuick2::GridView::positionViewAtEnd()

    Positions the view at the beginning or end, taking into account any header or footer.

    It is not recommended to use \l {Flickable::}{contentX} or \l {Flickable::}{contentY} to position the view
    at a particular index.  This is unreliable since removing items from the start
    of the list does not cause all other items to be repositioned, and because
    the actual start of the view can vary based on the size of the delegates.

    \bold Note: methods should only be called after the Component has completed.  To position
    the view at startup, this method should be called by Component.onCompleted.  For
    example, to position the view at the end on startup:

    \code
    Component.onCompleted: positionViewAtEnd()
    \endcode
*/

/*!
    \qmlmethod int QtQuick2::GridView::indexAt(int x, int y)

    Returns the index of the visible item containing the point \a x, \a y in content
    coordinates.  If there is no item at the point specified, or the item is
    not visible -1 is returned.

    If the item is outside the visible area, -1 is returned, regardless of
    whether an item will exist at that point when scrolled into view.

    \bold Note: methods should only be called after the Component has completed.
*/

QSGGridViewAttached *QSGGridView::qmlAttachedProperties(QObject *obj)
{
    return new QSGGridViewAttached(obj);
}

QT_END_NAMESPACE
