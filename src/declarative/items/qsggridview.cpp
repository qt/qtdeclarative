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

QT_BEGIN_NAMESPACE

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
                int colSize = view->cellWidth();
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

    int rowSize() const;
    int colSize() const;
    qreal colPosAt(int modelIndex) const;
    qreal rowPosAt(int modelIndex) const;
    qreal snapPosAt(qreal pos) const;
    FxViewItem *snapItemAt(qreal pos) const;
    int snapIndex() const;

    virtual bool addVisibleItems(int fillFrom, int fillTo, bool doBuffer);
    virtual bool removeNonVisibleItems(int bufferFrom, int bufferTo);
    virtual void visibleItemsChanged();

    virtual FxViewItem *newViewItem(int index, QSGItem *item);
    virtual void repositionPackageItemAt(QSGItem *item, int index);

    virtual void createHighlight();
    virtual void updateHighlight();
    virtual void resetHighlightPosition();

    virtual void setPosition(qreal pos);
    virtual void layoutVisibleItems();

    virtual qreal headerSize() const;
    virtual qreal footerSize() const;
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
    int cellWidth;
    int cellHeight;
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

int QSGGridViewPrivate::rowSize() const {
    return flow == QSGGridView::LeftToRight ? cellHeight : cellWidth;
}
int QSGGridViewPrivate::colSize() const {
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
        pos += rowSize()/2;
        snapPos = static_cast<FxGridItemSG*>(visibleItems.first())->rowPos() - visibleIndex / columns * rowSize();
        snapPos = pos - fmodf(pos - snapPos, qreal(rowSize()));
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

bool QSGGridViewPrivate::addVisibleItems(int fillFrom, int fillTo, bool doBuffer)
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

bool QSGGridViewPrivate::removeNonVisibleItems(int bufferFrom, int bufferTo)
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
    if ((!currentItem && highlight) || (currentItem && !highlight))
        createHighlight();
    if (currentItem && autoHighlight && highlight && !movingHorizontally && !movingVertically) {
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
            scheduleLayout();
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

    if (snapMode != QSGGridView::NoSnap) {
        qreal tempPosition = isRightToLeftTopToBottom() ? -position()-size() : position();
        FxViewItem *topItem = snapItemAt(tempPosition+highlightStart);
        FxViewItem *bottomItem = snapItemAt(tempPosition+highlightEnd);
        qreal pos;
        if (topItem && bottomItem && haveHighlightRange && highlightRange == QSGGridView::StrictlyEnforceRange) {
            qreal topPos = qMin(topItem->position() - highlightStart, -maxExtent);
            qreal bottomPos = qMax(bottomItem->position() - highlightEnd, -minExtent);
            pos = qAbs(data.move + topPos) < qAbs(data.move + bottomPos) ? topPos : bottomPos;
        } else if (topItem) {
            qreal headerPos = 0;
            if (header)
                headerPos = isRightToLeftTopToBottom() ? static_cast<FxGridItemSG*>(header)->rowPos() + cellWidth - headerSize() : static_cast<FxGridItemSG*>(header)->rowPos();
            if (topItem->index == 0 && header && tempPosition+highlightStart < headerPos+headerSize()/2) {
                pos = isRightToLeftTopToBottom() ? - headerPos + highlightStart - size() : headerPos - highlightStart;
            } else {
                if (isRightToLeftTopToBottom())
                    pos = qMax(qMin(-topItem->position() + highlightStart - size(), -maxExtent), -minExtent);
                else
                    pos = qMax(qMin(topItem->position() - highlightStart, -maxExtent), -minExtent);
            }
        } else if (bottomItem) {
            if (isRightToLeftTopToBottom())
                pos = qMax(qMin(-bottomItem->position() + highlightStart - size(), -maxExtent), -minExtent);
            else
                pos = qMax(qMin(bottomItem->position() - highlightStart, -maxExtent), -minExtent);
        } else {
            QSGItemViewPrivate::fixup(data, minExtent, maxExtent);
            return;
        }
        if (currentItem && haveHighlightRange && highlightRange == QSGGridView::StrictlyEnforceRange) {
            updateHighlight();
            qreal currPos = static_cast<FxGridItemSG*>(currentItem)->rowPos();
            if (isRightToLeftTopToBottom())
                pos = -pos-size(); // Transform Pos if required
            if (pos < currPos + rowSize() - highlightEnd)
                pos = currPos + rowSize() - highlightEnd;
            if (pos > currPos - highlightStart)
                pos = currPos - highlightStart;
            if (isRightToLeftTopToBottom())
                pos = -pos-size(); // Untransform
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
                if (FxViewItem *item = firstVisibleItem())
                    maxDistance = qAbs(item->position() + dataValue);
            } else {
                maxDistance = qAbs(minExtent - data.move.value());
            }
        }
        if (snapMode == QSGGridView::NoSnap && highlightRange != QSGGridView::StrictlyEnforceRange)
            data.flickTarget = minExtent;
    } else {
        if (data.move.value() > maxExtent) {
            if (snapMode == QSGGridView::SnapOneRow) {
                qreal pos = snapPosAt(-dataValue) + (isRightToLeftTopToBottom() ? 0 : rowSize());
                maxDistance = qAbs(pos + dataValue);
            } else {
                maxDistance = qAbs(maxExtent - data.move.value());
            }
        }
        if (snapMode == QSGGridView::NoSnap && highlightRange != QSGGridView::StrictlyEnforceRange)
            data.flickTarget = maxExtent;
    }
    bool overShoot = boundsBehavior == QSGFlickable::DragAndOvershootBounds;
    qreal highlightStart = isRightToLeftTopToBottom() && highlightRangeStartValid ? size()-highlightRangeEnd : highlightRangeStart;
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
            qreal distTemp = isRightToLeftTopToBottom() ? -dist : dist;
            data.flickTarget = -snapPosAt(-(dataValue - highlightStart) + distTemp) + highlightStart;
            data.flickTarget = isRightToLeftTopToBottom() ? -data.flickTarget+size() : data.flickTarget;
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
        if (!flickingHorizontally && q->xflick()) {
            flickingHorizontally = true;
            emit q->flickingChanged();
            emit q->flickingHorizontallyChanged();
            emit q->flickStarted();
        }
        if (!flickingVertically && q->yflick()) {
            flickingVertically = true;
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


int QSGGridView::cellWidth() const
{
    Q_D(const QSGGridView);
    return d->cellWidth;
}

void QSGGridView::setCellWidth(int cellWidth)
{
    Q_D(QSGGridView);
    if (cellWidth != d->cellWidth && cellWidth > 0) {
        d->cellWidth = qMax(1, cellWidth);
        d->updateViewport();
        emit cellWidthChanged();
        d->layout();
    }
}

int QSGGridView::cellHeight() const
{
    Q_D(const QSGGridView);
    return d->cellHeight;
}

void QSGGridView::setCellHeight(int cellHeight)
{
    Q_D(QSGGridView);
    if (cellHeight != d->cellHeight && cellHeight > 0) {
        d->cellHeight = qMax(1, cellHeight);
        d->updateViewport();
        emit cellHeightChanged();
        d->layout();
    }
}

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
    if (d->flickingHorizontally || d->flickingVertically) {
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
    if (d->flickingHorizontally || d->flickingVertically || d->movingHorizontally || d->movingVertically)
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

            static_cast<FxGridItemSG*>(d->highlight)->setPosition(static_cast<FxGridItemSG*>(d->highlight)->colPos(), qRound(pos));

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
    d->moveReason = QSGGridViewPrivate::Other;
    event->ignore();
    QSGItemView::keyPressEvent(event);
}

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


void QSGGridView::itemsInserted(int modelIndex, int count)
{
    Q_D(QSGGridView);
    if (!isComponentComplete())
        return;

    int index = d->visibleItems.count() ? d->mapFromModel(modelIndex) : 0;
    if (index < 0) {
        int i = d->visibleItems.count() - 1;
        while (i > 0 && d->visibleItems.at(i)->index == -1)
            --i;
        if (d->visibleItems.at(i)->index + 1 == modelIndex) {
            // Special case of appending an item to the model.
            index = d->visibleIndex + d->visibleItems.count();
        } else {
            if (modelIndex <= d->visibleIndex) {
                // Insert before visible items
                d->visibleIndex += count;
                for (int i = 0; i < d->visibleItems.count(); ++i) {
                    FxViewItem *item = d->visibleItems.at(i);
                    if (item->index != -1 && item->index >= modelIndex)
                        item->index += count;
                }
            }
            if (d->currentIndex >= modelIndex) {
                // adjust current item index
                d->currentIndex += count;
                if (d->currentItem)
                    d->currentItem->index = d->currentIndex;
                emit currentIndexChanged();
            }
            d->scheduleLayout();
            d->itemCount += count;
            emit countChanged();
            return;
        }
    }

    int insertCount = count;
    if (index < d->visibleIndex && d->visibleItems.count()) {
        insertCount -= d->visibleIndex - index;
        index = d->visibleIndex;
        modelIndex = d->visibleIndex;
    }

    qreal tempPos = d->isRightToLeftTopToBottom() ? -d->position()-d->size()+width()+1 : d->position();
    int to = d->buffer+tempPos+d->size()-1;
    int colPos = 0;
    int rowPos = 0;
    if (d->visibleItems.count()) {
        index -= d->visibleIndex;
        if (index < d->visibleItems.count()) {
            FxGridItemSG *gridItem = static_cast<FxGridItemSG*>(d->visibleItems.at(index));
            colPos = gridItem->colPos();
            rowPos = gridItem->rowPos();
        } else {
            // appending items to visible list
            FxGridItemSG *gridItem = static_cast<FxGridItemSG*>(d->visibleItems.at(index-1));
            colPos = gridItem->colPos() + d->colSize();
            rowPos = gridItem->rowPos();
            if (colPos > d->colSize() * (d->columns-1)) {
                colPos = 0;
                rowPos += d->rowSize();
            }
        }
    }

    // Update the indexes of the following visible items.
    for (int i = 0; i < d->visibleItems.count(); ++i) {
        FxViewItem *item = d->visibleItems.at(i);
        if (item->index != -1 && item->index >= modelIndex)
            item->index += count;
    }

    bool addedVisible = false;
    QList<FxGridItemSG*> added;
    int i = 0;
    while (i < insertCount && rowPos <= to + d->rowSize()*(d->columns - (colPos/d->colSize()))/qreal(d->columns)) {
        if (!addedVisible) {
            d->scheduleLayout();
            addedVisible = true;
        }
        FxGridItemSG *item = static_cast<FxGridItemSG*>(d->createItem(modelIndex + i));
        d->visibleItems.insert(index, item);
        item->setPosition(colPos, rowPos);
        added.append(item);
        colPos += d->colSize();
        if (colPos > d->colSize() * (d->columns-1)) {
            colPos = 0;
            rowPos += d->rowSize();
        }
        ++index;
        ++i;
    }
    if (i < insertCount) {
        // We didn't insert all our new items, which means anything
        // beyond the current index is not visible - remove it.
        while (d->visibleItems.count() > index) {
            d->releaseItem(d->visibleItems.takeLast());
        }
    }

    // update visibleIndex
    d->visibleIndex = 0;
    for (QList<FxViewItem*>::Iterator it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            break;
        }
    }

    if (d->itemCount && d->currentIndex >= modelIndex) {
        // adjust current item index
        d->currentIndex += count;
        if (d->currentItem) {
            d->currentItem->index = d->currentIndex;
            static_cast<FxGridItemSG*>(d->currentItem)->setPosition(d->colPosAt(d->currentIndex), d->rowPosAt(d->currentIndex));
        }
        emit currentIndexChanged();
    } else if (d->itemCount == 0 && (!d->currentIndex || (d->currentIndex < 0 && !d->currentIndexCleared))) {
        setCurrentIndex(0);
    }

    // everything is in order now - emit add() signal
    for (int j = 0; j < added.count(); ++j)
        added.at(j)->attached->emitAdd();

    d->itemCount += count;
    emit countChanged();
}

void QSGGridView::itemsRemoved(int modelIndex, int count)
{
    Q_D(QSGGridView);
    if (!isComponentComplete())
        return;

    d->itemCount -= count;
    bool currentRemoved = d->currentIndex >= modelIndex && d->currentIndex < modelIndex + count;
    bool removedVisible = false;

    // Remove the items from the visible list, skipping anything already marked for removal
    QList<FxViewItem*>::Iterator it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxViewItem *item = *it;
        if (item->index == -1 || item->index < modelIndex) {
            // already removed, or before removed items
            if (item->index < modelIndex && !removedVisible) {
                d->scheduleLayout();
                removedVisible = true;
            }
            ++it;
        } else if (item->index >= modelIndex + count) {
            // after removed items
            item->index -= count;
            ++it;
        } else {
            // removed item
            if (!removedVisible) {
                d->scheduleLayout();
                removedVisible = true;
            }
            item->attached->emitRemove();
            if (item->attached->delayRemove()) {
                item->index = -1;
                connect(item->attached, SIGNAL(delayRemoveChanged()), this, SLOT(destroyRemoved()), Qt::QueuedConnection);
                ++it;
            } else {
                it = d->visibleItems.erase(it);
                d->releaseItem(item);
            }
        }
    }

    // fix current
    if (d->currentIndex >= modelIndex + count) {
        d->currentIndex -= count;
        if (d->currentItem)
            d->currentItem->index -= count;
        emit currentIndexChanged();
    } else if (currentRemoved) {
        // current item has been removed.
        d->releaseItem(d->currentItem);
        d->currentItem = 0;
        d->currentIndex = -1;
        if (d->itemCount)
            d->updateCurrent(qMin(modelIndex, d->itemCount-1));
        else
            emit currentIndexChanged();
    }

    // update visibleIndex
    d->visibleIndex = 0;
    for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            break;
        }
    }

    if (removedVisible && d->visibleItems.isEmpty()) {
        d->timeline.clear();
        if (d->itemCount == 0) {
            d->setPosition(d->contentStartPosition());
            d->updateHeader();
            d->updateFooter();
        }
    }

    emit countChanged();
}


void QSGGridView::itemsMoved(int from, int to, int count)
{
    Q_D(QSGGridView);
    if (!isComponentComplete())
        return;
    d->updateUnrequestedIndexes();

    if (d->visibleItems.isEmpty()) {
        d->refill();
        return;
    }

    d->moveReason = QSGGridViewPrivate::Other;

    bool movingBackwards = from > to;
    d->adjustMoveParameters(&from, &to, &count);

    QHash<int,FxGridItemSG*> moved;
    int moveByCount = 0;
    FxGridItemSG *firstVisible = static_cast<FxGridItemSG*>(d->firstVisibleItem());
    int firstItemIndex = firstVisible ? firstVisible->index : -1;

    // if visibleItems.first() is above the content start pos, and the items
    // beneath it are moved, ensure this first item is later repositioned correctly
    // (to above the next visible item) so that subsequent layout() is correct
    bool repositionFirstItem = firstVisible
            && d->visibleItems.first()->position() < firstVisible->position()
            && from > d->visibleItems.first()->index;

    QList<FxViewItem*>::Iterator it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxViewItem *item = *it;
        if (item->index >= from && item->index < from + count) {
            // take the items that are moving
            item->index += (to-from);
            moved.insert(item->index, static_cast<FxGridItemSG*>(item));
            if (repositionFirstItem)
                moveByCount++;
            it = d->visibleItems.erase(it);
        } else {
            if (item->index > from && item->index != -1) {
                // move everything after the moved items.
                item->index -= count;
                if (item->index < d->visibleIndex)
                    d->visibleIndex = item->index;
            }
            ++it;
        }
    }

    int movedCount = 0;
    int endIndex = d->visibleIndex;
    it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxViewItem *item = *it;
        if (movedCount < count && item->index >= to && item->index < to + count) {
            // place items in the target position, reusing any existing items
            int targetIndex = item->index + movedCount;
            FxGridItemSG *movedItem = moved.take(targetIndex);
            if (!movedItem)
                movedItem = static_cast<FxGridItemSG*>(d->createItem(targetIndex));
            it = d->visibleItems.insert(it, movedItem);
            ++it;
            ++movedCount;
        } else {
            if (item->index != -1) {
                if (item->index >= to) {
                    // update everything after the moved items.
                    item->index += count;
                }
                endIndex = item->index;
            }
            ++it;
        }
    }

    // If we have moved items to the end of the visible items
    // then add any existing moved items that we have
    while (FxGridItemSG *item = moved.take(endIndex+1)) {
        d->visibleItems.append(item);
        ++endIndex;
    }

    // update visibleIndex
    for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            break;
        }
    }

    // if first visible item is moving but another item is moving up to replace it,
    // do this positioning now to avoid shifting all content forwards
    if (movingBackwards && firstItemIndex >= 0) {
        for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
            if ((*it)->index == firstItemIndex) {
                static_cast<FxGridItemSG*>(*it)->setPosition(firstVisible->colPos(),
                                                             firstVisible->rowPos());
                break;
            }
        }
    }

    // Fix current index
    if (d->currentIndex >= 0 && d->currentItem) {
        int oldCurrent = d->currentIndex;
        d->currentIndex = d->model->indexOf(d->currentItem->item, this);
        if (oldCurrent != d->currentIndex) {
            d->currentItem->index = d->currentIndex;
            emit currentIndexChanged();
        }
    }

    // Whatever moved items remain are no longer visible items.
    while (moved.count()) {
        int idx = moved.begin().key();
        FxGridItemSG *item = moved.take(idx);
        if (d->currentItem && item->item == d->currentItem->item)
            item->setPosition(d->colPosAt(idx), d->rowPosAt(idx));
        d->releaseItem(item);
    }

    // Ensure we don't cause an ugly list scroll.
    if (d->visibleItems.count() && moveByCount > 0) {
        FxGridItemSG *first = static_cast<FxGridItemSG*>(d->visibleItems.first());
        first->setPosition(first->colPos(), first->rowPos() + ((moveByCount / d->columns) * d->rowSize()));
    }

    d->layout();
}


QSGGridViewAttached *QSGGridView::qmlAttachedProperties(QObject *obj)
{
    return new QSGGridViewAttached(obj);
}

QT_END_NAMESPACE
