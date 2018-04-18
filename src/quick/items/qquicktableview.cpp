/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktableview_p.h"
#include "qquicktableview_p_p.h"

#include <QtCore/qtimer.h>
#include <QtQml/private/qqmldelegatemodel_p.h>
#include <QtQml/private/qqmlincubator_p.h>
#include <QtQml/private/qqmlchangeset_p.h>
#include <QtQml/qqmlinfo.h>

#include <QtQuick/private/qquickflickable_p_p.h>
#include <QtQuick/private/qquickitemviewfxitem_p_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTableViewDelegateLifecycle, "qt.quick.tableview.lifecycle")

#define Q_TABLEVIEW_UNREACHABLE(output) { dumpTable(); qWarning() << "output:" << output; Q_UNREACHABLE(); }
#define Q_TABLEVIEW_ASSERT(cond, output) Q_ASSERT(cond || [&](){ dumpTable(); qWarning() << "output:" << output; return false;}())

static const Qt::Edge allTableEdges[] = { Qt::LeftEdge, Qt::RightEdge, Qt::TopEdge, Qt::BottomEdge };
static const int kBufferTimerInterval = 300;

static QLine rectangleEdge(const QRect &rect, Qt::Edge tableEdge)
{
    switch (tableEdge) {
    case Qt::LeftEdge:
        return QLine(rect.topLeft(), rect.bottomLeft());
    case Qt::RightEdge:
        return QLine(rect.topRight(), rect.bottomRight());
    case Qt::TopEdge:
        return QLine(rect.topLeft(), rect.topRight());
    case Qt::BottomEdge:
        return QLine(rect.bottomLeft(), rect.bottomRight());
    }
    return QLine();
}

static QRect expandedRect(const QRect &rect, Qt::Edge edge, int increment)
{
    switch (edge) {
    case Qt::LeftEdge:
        return rect.adjusted(-increment, 0, 0, 0);
    case Qt::RightEdge:
        return rect.adjusted(0, 0, increment, 0);
    case Qt::TopEdge:
        return rect.adjusted(0, -increment, 0, 0);
    case Qt::BottomEdge:
        return rect.adjusted(0, 0, 0, increment);
    }
    return QRect();
}

const QPoint QQuickTableViewPrivate::kLeft = QPoint(-1, 0);
const QPoint QQuickTableViewPrivate::kRight = QPoint(1, 0);
const QPoint QQuickTableViewPrivate::kUp = QPoint(0, -1);
const QPoint QQuickTableViewPrivate::kDown = QPoint(0, 1);

QQuickTableViewPrivate::QQuickTableViewPrivate()
    : QQuickFlickablePrivate()
{
    cacheBufferDelayTimer.setSingleShot(true);
    QObject::connect(&cacheBufferDelayTimer, &QTimer::timeout, [=]{ loadBuffer(); });
}

QQuickTableViewPrivate::~QQuickTableViewPrivate()
{
    clear();
}

QString QQuickTableViewPrivate::tableLayoutToString() const
{
    return QString(QLatin1String("table cells: (%1,%2) -> (%3,%4), item count: %5, table rect: %6,%7 x %8,%9"))
            .arg(loadedTable.topLeft().x()).arg(loadedTable.topLeft().y())
            .arg(loadedTable.bottomRight().x()).arg(loadedTable.bottomRight().y())
            .arg(loadedItems.count())
            .arg(loadedTableOuterRect.x())
            .arg(loadedTableOuterRect.y())
            .arg(loadedTableOuterRect.width())
            .arg(loadedTableOuterRect.height());
}

void QQuickTableViewPrivate::dumpTable() const
{
    auto listCopy = loadedItems;
    std::stable_sort(listCopy.begin(), listCopy.end(),
        [](const FxTableItem *lhs, const FxTableItem *rhs)
        { return lhs->index < rhs->index; });

    qWarning() << QStringLiteral("******* TABLE DUMP *******");
    for (int i = 0; i < listCopy.count(); ++i)
        qWarning() << static_cast<FxTableItem *>(listCopy.at(i))->cell;
    qWarning() << tableLayoutToString();

    QString filename = QStringLiteral("QQuickTableView_dumptable_capture.png");
    if (q_func()->window()->grabWindow().save(filename))
        qWarning() << "Window capture saved to:" << filename;
}

QQuickTableViewAttached *QQuickTableViewPrivate::getAttachedObject(const QObject *object) const
{
    QObject *attachedObject = qmlAttachedPropertiesObject<QQuickTableView>(object);
    return static_cast<QQuickTableViewAttached *>(attachedObject);
}

int QQuickTableViewPrivate::modelIndexAtCell(const QPoint &cell)
{
    int availableRows = tableSize.height();
    int modelIndex = cell.y() + (cell.x() * availableRows);
    Q_TABLEVIEW_ASSERT(modelIndex < model->count(), modelIndex << cell);
    return modelIndex;
}

QPoint QQuickTableViewPrivate::cellAtModelIndex(int modelIndex)
{
    int availableRows = tableSize.height();
    Q_TABLEVIEW_ASSERT(availableRows > 0, availableRows);
    int column = int(modelIndex / availableRows);
    int row = modelIndex % availableRows;
    return QPoint(column, row);
}

void QQuickTableViewPrivate::updateContentWidth()
{
    Q_Q(QQuickTableView);

    const qreal thresholdBeforeAdjust = 0.1;
    int currentRightColumn = loadedTable.right();

    if (currentRightColumn > contentSizeBenchMarkPoint.x()) {
        contentSizeBenchMarkPoint.setX(currentRightColumn);

        qreal currentWidth = loadedTableOuterRect.right();
        qreal averageCellSize = currentWidth / (currentRightColumn + 1);
        qreal averageSize = averageCellSize + cellSpacing.width();
        qreal estimatedWith = (tableSize.width() * averageSize) - cellSpacing.width();

        if (currentRightColumn >= tableSize.width() - 1) {
            // We are at the last column, and can set the exact width
            if (currentWidth != q->implicitWidth())
                q->setContentWidth(currentWidth);
        } else if (currentWidth >= q->implicitWidth()) {
            // We are at the estimated width, but there are still more columns
            q->setContentWidth(estimatedWith);
        } else {
            // Only set a new width if the new estimate is substantially different
            qreal diff = 1 - (estimatedWith / q->implicitWidth());
            if (qAbs(diff) > thresholdBeforeAdjust)
                q->setContentWidth(estimatedWith);
        }
    }
}

void QQuickTableViewPrivate::updateContentHeight()
{
    Q_Q(QQuickTableView);

    const qreal thresholdBeforeAdjust = 0.1;
    int currentBottomRow = loadedTable.bottom();

    if (currentBottomRow > contentSizeBenchMarkPoint.y()) {
        contentSizeBenchMarkPoint.setY(currentBottomRow);

        qreal currentHeight = loadedTableOuterRect.bottom();
        qreal averageCellSize = currentHeight / (currentBottomRow + 1);
        qreal averageSize = averageCellSize + cellSpacing.height();
        qreal estimatedHeight = (tableSize.height() * averageSize) - cellSpacing.height();

        if (currentBottomRow >= tableSize.height() - 1) {
            // We are at the last row, and can set the exact height
            if (currentHeight != q->implicitHeight())
                q->setContentHeight(currentHeight);
        } else if (currentHeight >= q->implicitHeight()) {
            // We are at the estimated height, but there are still more rows
            q->setContentHeight(estimatedHeight);
        } else {
            // Only set a new height if the new estimate is substantially different
            qreal diff = 1 - (estimatedHeight / q->implicitHeight());
            if (qAbs(diff) > thresholdBeforeAdjust)
                q->setContentHeight(estimatedHeight);
        }
    }
}

void QQuickTableViewPrivate::enforceFirstRowColumnAtOrigo()
{
    // Gaps before the first row/column can happen if rows/columns
    // changes size while flicking e.g because of spacing changes or
    // changes to a column maxWidth/row maxHeight. Check for this, and
    // move the whole table rect accordingly.
    bool layoutNeeded = false;
    const qreal flickMargin = 50;

    if (loadedTable.x() == 0 && loadedTableOuterRect.x() != 0) {
        // The table is at the beginning, but not at the edge of the
        // content view. So move the table to origo.
        loadedTableOuterRect.moveLeft(0);
        layoutNeeded = true;
    } else if (loadedTableOuterRect.x() < 0) {
        // The table is outside the beginning of the content view. Move
        // the whole table inside, and make some room for flicking.
        loadedTableOuterRect.moveLeft(loadedTable.x() == 0 ? 0 : flickMargin);
        layoutNeeded = true;
    }

    if (loadedTable.y() == 0 && loadedTableOuterRect.y() != 0) {
        loadedTableOuterRect.moveTop(0);
        layoutNeeded = true;
    } else if (loadedTableOuterRect.y() < 0) {
        loadedTableOuterRect.moveTop(loadedTable.y() == 0 ? 0 : flickMargin);
        layoutNeeded = true;
    }

    if (layoutNeeded)
        relayoutTableItems();
}

void QQuickTableViewPrivate::syncLoadedTableRectFromLoadedTable()
{
    QRectF topLeftRect = loadedTableItem(loadedTable.topLeft())->geometry();
    QRectF bottomRightRect = loadedTableItem(loadedTable.bottomRight())->geometry();
    loadedTableOuterRect = topLeftRect.united(bottomRightRect);
    loadedTableInnerRect = QRectF(topLeftRect.bottomRight(), bottomRightRect.topLeft());
}

void QQuickTableViewPrivate::syncLoadedTableFromLoadRequest()
{
    switch (loadRequest.edge()) {
    case Qt::LeftEdge:
    case Qt::TopEdge:
        loadedTable.setTopLeft(loadRequest.firstCell());
        break;
    case Qt::RightEdge:
    case Qt::BottomEdge:
        loadedTable.setBottomRight(loadRequest.lastCell());
        break;
    default:
        loadedTable = QRect(loadRequest.firstCell(), loadRequest.lastCell());
    }
}

FxTableItem *QQuickTableViewPrivate::itemNextTo(const FxTableItem *fxTableItem, const QPoint &direction) const
{
    return loadedTableItem(fxTableItem->cell + direction);
}

FxTableItem *QQuickTableViewPrivate::loadedTableItem(const QPoint &cell) const
{
    for (int i = 0; i < loadedItems.count(); ++i) {
        FxTableItem *item = loadedItems.at(i);
        if (item->cell == cell)
            return item;
    }

    Q_TABLEVIEW_UNREACHABLE(cell);
    return nullptr;
}

FxTableItem *QQuickTableViewPrivate::createFxTableItem(const QPoint &cell, QQmlIncubator::IncubationMode incubationMode)
{
    Q_Q(QQuickTableView);

    bool ownItem = false;
    int modelIndex = modelIndexAtCell(cell);

    QObject* object = model->object(modelIndex, incubationMode);
    if (!object) {
        if (model->incubationStatus(modelIndex) == QQmlIncubator::Loading) {
            // Item is incubating. Return nullptr for now, and let the table call this
            // function again once we get a callback to itemCreatedCallback().
            return nullptr;
        }

        qWarning() << "TableView: failed loading index:" << modelIndex;
        object = new QQuickItem();
        ownItem = true;
    }

    QQuickItem *item = qmlobject_cast<QQuickItem*>(object);
    if (!item) {
        // The model could not provide an QQuickItem for the
        // given index, so we create a placeholder instead.
        qWarning() << "TableView: delegate is not an item:" << modelIndex;
        model->release(object);
        item = new QQuickItem();
        ownItem = true;
    }

    item->setParentItem(q->contentItem());

    FxTableItem *fxTableItem = new FxTableItem(item, q, ownItem);
    fxTableItem->setVisible(false);
    fxTableItem->cell = cell;
    fxTableItem->index = modelIndex;
    return fxTableItem;
}

FxTableItem *QQuickTableViewPrivate::loadFxTableItem(const QPoint &cell, QQmlIncubator::IncubationMode incubationMode)
{
#ifdef QT_DEBUG
    // Since TableView needs to work flawlessly when e.g incubating inside an async
    // loader, being able to override all loading to async while debugging can be helpful.
    static const bool forcedAsync = forcedIncubationMode == QLatin1String("async");
    if (forcedAsync)
        incubationMode = QQmlIncubator::Asynchronous;
#endif

    // Note that even if incubation mode is asynchronous, the item might
    // be ready immediately since the model has a cache of items.
    QBoolBlocker guard(blockItemCreatedCallback);
    auto item = createFxTableItem(cell, incubationMode);
    qCDebug(lcTableViewDelegateLifecycle) << cell << "ready?" << bool(item);
    return item;
}

void QQuickTableViewPrivate::releaseLoadedItems() {
    // Make a copy and clear the list of items first to avoid destroyed
    // items being accessed during the loop (QTBUG-61294)
    auto const tmpList = loadedItems;
    loadedItems.clear();
    for (FxTableItem *item : tmpList)
        releaseItem(item);
}

void QQuickTableViewPrivate::releaseItem(FxTableItem *fxTableItem)
{
    if (fxTableItem->item) {
        if (fxTableItem->ownItem)
            delete fxTableItem->item;
        else if (model->release(fxTableItem->item) != QQmlInstanceModel::Destroyed)
            fxTableItem->item->setParentItem(nullptr);
    }

    delete fxTableItem;
}

void QQuickTableViewPrivate::clear()
{
    tableInvalid = true;
    tableRebuilding = false;
    if (loadRequest.isActive())
        cancelLoadRequest();

    releaseLoadedItems();
    loadedTable = QRect();
    loadedTableOuterRect = QRect();
    loadedTableInnerRect = QRect();
    columnWidths.clear();
    rowHeights.clear();
    contentSizeBenchMarkPoint = QPoint(-1, -1);

    updateContentWidth();
    updateContentHeight();
}

void QQuickTableViewPrivate::unloadItem(const QPoint &cell)
{
    FxTableItem *item = loadedTableItem(cell);
    loadedItems.removeOne(item);
    releaseItem(item);
}

void QQuickTableViewPrivate::unloadItems(const QLine &items)
{
    qCDebug(lcTableViewDelegateLifecycle) << items;

    if (items.dx()) {
        int y = items.p1().y();
        for (int x = items.p1().x(); x <= items.p2().x(); ++x)
            unloadItem(QPoint(x, y));
    } else {
        int x = items.p1().x();
        for (int y = items.p1().y(); y <= items.p2().y(); ++y)
            unloadItem(QPoint(x, y));
    }
}

bool QQuickTableViewPrivate::canLoadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const
{
    switch (tableEdge) {
    case Qt::LeftEdge:
        if (loadedTable.topLeft().x() == 0)
            return false;
        return loadedTableOuterRect.left() > fillRect.left();
    case Qt::RightEdge:
        if (loadedTable.bottomRight().x() >= tableSize.width() - 1)
            return false;
        return loadedTableOuterRect.right() < fillRect.right();
    case Qt::TopEdge:
        if (loadedTable.topLeft().y() == 0)
            return false;
        return loadedTableOuterRect.top() > fillRect.top();
    case Qt::BottomEdge:
        if (loadedTable.bottomRight().y() >= tableSize.height() - 1)
            return false;
        return loadedTableOuterRect.bottom() < fillRect.bottom();
    }

    return false;
}

bool QQuickTableViewPrivate::canUnloadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const
{
    // Note: if there is only one row or column left, we cannot unload, since
    // they are needed as anchor point for further layouting.
    const qreal floatingPointMargin = 1;

    switch (tableEdge) {
    case Qt::LeftEdge:
        if (loadedTable.width() <= 1)
            return false;
        return loadedTableInnerRect.left() < fillRect.left() - floatingPointMargin;
    case Qt::RightEdge:
        if (loadedTable.width() <= 1)
            return false;
        return loadedTableInnerRect.right() > fillRect.right() + floatingPointMargin;
    case Qt::TopEdge:
        if (loadedTable.height() <= 1)
            return false;
        return loadedTableInnerRect.top() < fillRect.top() - floatingPointMargin;
    case Qt::BottomEdge:
        if (loadedTable.height() <= 1)
            return false;
        return loadedTableInnerRect.bottom() > fillRect.bottom() + floatingPointMargin;
    }
    Q_TABLEVIEW_UNREACHABLE(tableEdge);
    return false;
}

Qt::Edge QQuickTableViewPrivate::nextEdgeToLoad(const QRectF rect)
{
    for (Qt::Edge edge : allTableEdges) {
        if (canLoadTableEdge(edge, rect))
            return edge;
    }
    return Qt::Edge(0);
}

Qt::Edge QQuickTableViewPrivate::nextEdgeToUnload(const QRectF rect)
{
    for (Qt::Edge edge : allTableEdges) {
        if (canUnloadTableEdge(edge, rect))
            return edge;
    }
    return Qt::Edge(0);
}

void QQuickTableViewPrivate::calculateColumnWidthsAfterRebuilding()
{
    qreal prevColumnWidth = 0;
    for (int column = loadedTable.left(); column <= loadedTable.right(); ++column) {
        qreal columnWidth = 0;
        for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row) {
            auto const item = loadedTableItem(QPoint(column, row));
            columnWidth = qMax(columnWidth, item->geometry().width());
        }

        if (columnWidth == prevColumnWidth)
            continue;

        columnWidths.append({column, columnWidth});
        prevColumnWidth = columnWidth;
    }

    if (columnWidths.isEmpty()) {
        // Add at least one column, wo we don't need
        // to check if the vector is empty elsewhere.
        columnWidths.append({0, 0});
    }
}

void QQuickTableViewPrivate::calculateRowHeightsAfterRebuilding()
{
    qreal prevRowHeight = 0;
    for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row) {
        qreal rowHeight = 0;
        for (int column = loadedTable.left(); column <= loadedTable.right(); ++column) {
            auto const item = loadedTableItem(QPoint(column, row));
            rowHeight = qMax(rowHeight, item->geometry().height());
        }

        if (rowHeight == prevRowHeight)
            continue;

        rowHeights.append({row, rowHeight});
    }

    if (rowHeights.isEmpty()) {
        // Add at least one row, wo we don't need
        // to check if the vector is empty elsewhere.
        rowHeights.append({0, 0});
    }
}

void QQuickTableViewPrivate::calculateColumnWidth(int column)
{
    if (column < columnWidths.last().index) {
        // We only do the calculation once, and then stick with the size.
        // See comments inside ColumnRowSize struct.
        return;
    }

    qreal columnWidth = 0;
    for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row) {
        auto const item = loadedTableItem(QPoint(column, row));
        columnWidth = qMax(columnWidth, item->geometry().width());
    }

    if (columnWidth == columnWidths.last().size)
        return;

    columnWidths.append({column, columnWidth});
}

void QQuickTableViewPrivate::calculateRowHeight(int row)
{
    if (row < rowHeights.last().index) {
        // We only do the calculation once, and then stick with the size.
        // See comments inside ColumnRowSize struct.
        return;
    }

    qreal rowHeight = 0;
    for (int column = loadedTable.left(); column <= loadedTable.right(); ++column) {
        auto const item = loadedTableItem(QPoint(column, row));
        rowHeight = qMax(rowHeight, item->geometry().height());
    }

    if (rowHeight == rowHeights.last().size)
        return;

    rowHeights.append({row, rowHeight});
}

void QQuickTableViewPrivate::calculateEdgeSizeFromLoadRequest()
{
    if (tableRebuilding)
        return;

    switch (loadRequest.edge()) {
    case Qt::LeftEdge:
    case Qt::TopEdge:
        // Flicking left or up through "never loaded" rows/columns is currently
        // not supported. You always need to start loading the table from the beginning.
        return;
    case Qt::RightEdge:
        if (tableSize.height() > 1)
            calculateColumnWidth(loadedTable.right());
        break;
    case Qt::BottomEdge:
        if (tableSize.width() > 1)
            calculateRowHeight(loadedTable.bottom());
        break;
    default:
        Q_TABLEVIEW_UNREACHABLE("This function should not be called when loading top-left item");
    }
}

void QQuickTableViewPrivate::calculateTableSize()
{
    // tableSize is the same as row and column count, and will always
    // be the same as the number of rows and columns in the model.
    Q_Q(QQuickTableView);
    QSize prevTableSize = tableSize;

    if (auto wrapper = wrapperModel())
        tableSize = QSize(wrapper->columns(), wrapper->rows());
    else if (model)
        tableSize = QSize(1, model->count());
    else
        tableSize = QSize(0, 0);

    if (prevTableSize.width() != tableSize.width())
        emit q->columnsChanged();
    if (prevTableSize.height() != tableSize.height())
        emit q->rowsChanged();
}

qreal QQuickTableViewPrivate::columnWidth(int column)
{
    if (!columnWidths.isEmpty()) {
        // Find the ColumnRowSize assignment before, or at, column
        auto iter = std::lower_bound(columnWidths.constBegin(), columnWidths.constEnd(),
                                     ColumnRowSize{column, -1}, ColumnRowSize::lessThan);

        // First check if we got an explicit assignment
        if (iter->index == column)
            return iter->size;

        // If the table is not a list, return the size of
        // ColumnRowSize element found before column.
        if (tableSize.height() > 1)
            return (iter - 1)->size;
    }

    // if we have an item loaded at column, return the width of the item.
    if (column >= loadedTable.left() && column <= loadedTable.right())
        return loadedTableItem(QPoint(column, 0))->geometry().width();

    return -1;
}

qreal QQuickTableViewPrivate::rowHeight(int row)
{
    if (!rowHeights.isEmpty()) {
        // Find the ColumnRowSize assignment before, or at, row
        auto iter = std::lower_bound(rowHeights.constBegin(), rowHeights.constEnd(),
                                     ColumnRowSize{row, -1}, ColumnRowSize::lessThan);

        // First check if we got an explicit assignment
        if (iter->index == row)
            return iter->size;

        // If the table is not a list, return the size of
        // ColumnRowSize element found before column.
        if (q_func()->columns() > 1)
            return (iter - 1)->size;
    }

    // if we have an item loaded at row, return the height of the item.
    if (row >= loadedTable.top() && row <= loadedTable.bottom())
        return loadedTableItem(QPoint(0, row))->geometry().height();

    return -1;
}

void QQuickTableViewPrivate::relayoutTable()
{
    relayoutTableItems();
    columnRowPositionsInvalid = false;

    syncLoadedTableRectFromLoadedTable();
    contentSizeBenchMarkPoint = QPoint(-1, -1);
    updateContentWidth();
    updateContentHeight();
}

void QQuickTableViewPrivate::relayoutTableItems()
{
    qCDebug(lcTableViewDelegateLifecycle);
    columnRowPositionsInvalid = false;

    qreal nextColumnX = loadedTableOuterRect.x();
    qreal nextRowY = loadedTableOuterRect.y();

    for (int column = loadedTable.left(); column <= loadedTable.right(); ++column) {
        // Adjust the geometry of all cells in the current column
        qreal width = columnWidth(column);
        for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row) {
            auto item = loadedTableItem(QPoint(column, row));
            QRectF geometry = item->geometry();
            geometry.moveLeft(nextColumnX);
            geometry.setWidth(width);
            item->setGeometry(geometry);
        }

        nextColumnX += width + cellSpacing.width();
    }

    for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row) {
        // Adjust the geometry of all cells in the current row
        qreal height = rowHeight(row);
        for (int column = loadedTable.left(); column <= loadedTable.right(); ++column) {
            auto item = loadedTableItem(QPoint(column, row));
            QRectF geometry = item->geometry();
            geometry.moveTop(nextRowY);
            geometry.setHeight(height);
            item->setGeometry(geometry);
        }

        nextRowY += height + cellSpacing.height();
    }

    if (Q_UNLIKELY(lcTableViewDelegateLifecycle().isDebugEnabled())) {
        for (int column = loadedTable.left(); column <= loadedTable.right(); ++column) {
            for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row) {
                QPoint cell = QPoint(column, row);
                qCDebug(lcTableViewDelegateLifecycle()) << "relayout item:" << cell << loadedTableItem(cell)->geometry();
            }
        }
    }
}

void QQuickTableViewPrivate::layoutVerticalEdge(Qt::Edge tableEdge, bool adjustSize)
{
    int column = (tableEdge == Qt::LeftEdge) ? loadedTable.left() : loadedTable.right();
    QPoint neighbourDirection = (tableEdge == Qt::LeftEdge) ? kRight : kLeft;
    qreal width = adjustSize ? columnWidth(column) : 0;
    qreal left = -1;

    for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row) {
        auto fxTableItem = loadedTableItem(QPoint(column, row));
        auto const neighbourItem = itemNextTo(fxTableItem, neighbourDirection);
        QRectF geometry = fxTableItem->geometry();

        if (adjustSize) {
            geometry.setWidth(width);
            geometry.setHeight(neighbourItem->geometry().height());
        }

        if (left == -1) {
            // left will be the same for all items in the
            // column, so do the calculation once.
            left = tableEdge == Qt::LeftEdge ?
                        neighbourItem->geometry().left() - cellSpacing.width() - geometry.width() :
                        neighbourItem->geometry().right() + cellSpacing.width();
        }

        geometry.moveLeft(left);
        geometry.moveTop(neighbourItem->geometry().top());

        fxTableItem->setGeometry(geometry);
        fxTableItem->setVisible(true);

        qCDebug(lcTableViewDelegateLifecycle()) << "layout item:"
                                            << QPoint(column, row)
                                            << fxTableItem->geometry()
                                            << "adjust size:" << adjustSize;
    }
}

void QQuickTableViewPrivate::layoutHorizontalEdge(Qt::Edge tableEdge, bool adjustSize)
{
    int row = (tableEdge == Qt::TopEdge) ? loadedTable.top() : loadedTable.bottom();
    QPoint neighbourDirection = (tableEdge == Qt::TopEdge) ? kDown : kUp;
    qreal height = adjustSize ? rowHeight(row) : 0;
    qreal top = -1;

    for (int column = loadedTable.left(); column <= loadedTable.right(); ++column) {
        auto fxTableItem = loadedTableItem(QPoint(column, row));
        auto const neighbourItem = itemNextTo(fxTableItem, neighbourDirection);
        QRectF geometry = fxTableItem->geometry();

        if (adjustSize) {
            geometry.setWidth(neighbourItem->geometry().width());
            geometry.setHeight(height);
        }

        if (top == -1) {
            // top will be the same for all items in the
            // row, so do the calculation once.
            top = tableEdge == Qt::TopEdge ?
                neighbourItem->geometry().top() - cellSpacing.height() - geometry.height() :
                neighbourItem->geometry().bottom() + cellSpacing.height();
        }

        geometry.moveTop(top);
        geometry.moveLeft(neighbourItem->geometry().left());

        fxTableItem->setGeometry(geometry);
        fxTableItem->setVisible(true);

        qCDebug(lcTableViewDelegateLifecycle()) << "layout item:"
                                            << QPoint(column, row)
                                            << fxTableItem->geometry()
                                            << "adjust size:" << adjustSize;
    }
}

void QQuickTableViewPrivate::layoutTableEdgeFromLoadRequest()
{
    // If tableRebuilding is true, we avoid adjusting cell sizes until all items
    // needed to fill up the viewport has been loaded. This way we can base the later
    // relayoutTable() on the original size of the items. But we still need to
    // position the items now at approximate positions so we know (roughly) how many
    // rows/columns we need to load before we can consider the viewport as filled up.
    // Only then will the table rebuilding be considered done, and relayoutTable() called.
    // The relayout might cause new rows and columns to be loaded/unloaded depending on
    // whether the new sizes reveals or hides already loaded rows/columns.
    const bool adjustSize = !tableRebuilding;

    switch (loadRequest.edge()) {
    case Qt::LeftEdge:
    case Qt::RightEdge:
        layoutVerticalEdge(loadRequest.edge(), adjustSize);
        break;
    case Qt::TopEdge:
    case Qt::BottomEdge:
        layoutHorizontalEdge(loadRequest.edge(), adjustSize);
        break;
    default:
        // Request loaded top-left item rather than an edge.
        // ###todo: support starting with other top-left items than 0,0
        Q_TABLEVIEW_ASSERT(loadRequest.firstCell() == QPoint(0, 0), loadRequest.toString());
        loadedTableItem(QPoint(0, 0))->setVisible(true);
        qCDebug(lcTableViewDelegateLifecycle) << "top-left item geometry:" << loadedTableItem(QPoint(0, 0))->geometry();
        break;
    }
}

void QQuickTableViewPrivate::cancelLoadRequest()
{
    loadRequest.markAsDone();
    model->cancel(modelIndexAtCell(loadRequest.currentCell()));

    if (tableInvalid) {
        // No reason to rollback already loaded edge items
        // since we anyway are about to reload all items.
        return;
    }

    if (loadRequest.atBeginning()) {
        // No items have yet been loaded, so nothing to unload
        return;
    }

    QLine rollbackItems;
    rollbackItems.setP1(loadRequest.firstCell());
    rollbackItems.setP2(loadRequest.previousCell());
    qCDebug(lcTableViewDelegateLifecycle()) << "rollback:" << rollbackItems << tableLayoutToString();
    unloadItems(rollbackItems);
}

void QQuickTableViewPrivate::processLoadRequest()
{
    Q_TABLEVIEW_ASSERT(loadRequest.isActive(), "");

    while (loadRequest.hasCurrentCell()) {
        QPoint cell = loadRequest.currentCell();
        FxTableItem *fxTableItem = loadFxTableItem(cell, loadRequest.incubationMode());

        if (!fxTableItem) {
            // Requested item is not yet ready. Just leave, and wait for this
            // function to be called again when the item is ready.
            return;
        }

        loadedItems.append(fxTableItem);
        loadRequest.moveToNextCell();
    }

    qCDebug(lcTableViewDelegateLifecycle()) << "all items loaded!";

    syncLoadedTableFromLoadRequest();
    calculateEdgeSizeFromLoadRequest();
    layoutTableEdgeFromLoadRequest();

    syncLoadedTableRectFromLoadedTable();
    enforceFirstRowColumnAtOrigo();
    updateContentWidth();
    updateContentHeight();

    loadRequest.markAsDone();
    qCDebug(lcTableViewDelegateLifecycle()) << "request completed! Table:" << tableLayoutToString();
}

void QQuickTableViewPrivate::beginRebuildTable()
{
    qCDebug(lcTableViewDelegateLifecycle());
    clear();
    tableInvalid = false;
    tableRebuilding = true;
    calculateTableSize();
    loadInitialTopLeftItem();
    loadAndUnloadVisibleEdges();
}

void QQuickTableViewPrivate::endRebuildTable()
{
    tableRebuilding = false;

    if (loadedItems.isEmpty())
        return;

    // We don't calculate row/column sizes for lists.
    // Instead we we use the sizes of the items directly
    // unless for explicit row/column size assignments.
    columnWidths.clear();
    rowHeights.clear();
    if (tableSize.height() > 1)
        calculateColumnWidthsAfterRebuilding();
    if (tableSize.width() > 1)
        calculateRowHeightsAfterRebuilding();

    relayoutTable();
    qCDebug(lcTableViewDelegateLifecycle()) << tableLayoutToString();
}

void QQuickTableViewPrivate::loadInitialTopLeftItem()
{
    Q_TABLEVIEW_ASSERT(loadedItems.isEmpty(), "");

    if (tableSize.isEmpty())
        return;

    // Load top-left item. After loaded, loadItemsInsideRect() will take
    // care of filling out the rest of the table.
    loadRequest.begin(QPoint(0, 0), QQmlIncubator::AsynchronousIfNested);
    processLoadRequest();
}

void QQuickTableViewPrivate::unloadEdgesOutsideRect(const QRectF &rect)
{
    while (Qt::Edge edge = nextEdgeToUnload(rect)) {
        unloadItems(rectangleEdge(loadedTable, edge));
        loadedTable = expandedRect(loadedTable, edge, -1);
        syncLoadedTableRectFromLoadedTable();
        qCDebug(lcTableViewDelegateLifecycle) << tableLayoutToString();
    }
}

void QQuickTableViewPrivate::loadEdgesInsideRect(const QRectF &rect, QQmlIncubator::IncubationMode incubationMode)
{
    while (Qt::Edge edge = nextEdgeToLoad(rect)) {
        QLine cellsToLoad = rectangleEdge(expandedRect(loadedTable, edge, 1), edge);
        loadRequest.begin(cellsToLoad, edge, incubationMode);
        processLoadRequest();
        if (loadRequest.isActive())
            return;
    }
}

void QQuickTableViewPrivate::loadAndUnloadVisibleEdges()
{
    // Unload table edges that have been moved outside the visible part of the
    // table (including buffer area), and load new edges that has been moved inside.
    // Note: an important point is that we always keep the table rectangular
    // and without holes to reduce complexity (we never leave the table in
    // a half-loaded state, or keep track of multiple patches).
    // We load only one edge (row or column) at a time. This is especially
    // important when loading into the buffer, since we need to be able to
    // cancel the buffering quickly if the user starts to flick, and then
    // focus all further loading on the edges that are flicked into view.

    if (loadRequest.isActive()) {
        // Don't start loading more edges while we're
        // already waiting for another one to load.
        return;
    }

    if (loadedItems.isEmpty()) {
        // We need at least the top-left item to be loaded before we can
        // start loading edges around it. Not having a top-left item at
        // this point means that the model is empty (or row and and column
        // is 0) since we have no active load request.
        return;
    }

    unloadEdgesOutsideRect(hasBufferedItems ? bufferRect() : viewportRect);
    loadEdgesInsideRect(viewportRect, QQmlIncubator::AsynchronousIfNested);
}

void QQuickTableViewPrivate::loadBuffer()
{
    // Rather than making sure to stop the timer from all locations that can
    // violate the "buffering allowed" state, we just check that we're in the
    // right state here before we start buffering.
    if (cacheBuffer <= 0 || loadRequest.isActive() || loadedItems.isEmpty())
        return;

    qCDebug(lcTableViewDelegateLifecycle());
    loadEdgesInsideRect(bufferRect(), QQmlIncubator::Asynchronous);
    hasBufferedItems = true;
}

void QQuickTableViewPrivate::unloadBuffer()
{
    if (!hasBufferedItems)
        return;

    qCDebug(lcTableViewDelegateLifecycle());
    hasBufferedItems = false;
    cacheBufferDelayTimer.stop();
    if (loadRequest.isActive())
        cancelLoadRequest();
    unloadEdgesOutsideRect(viewportRect);
}

QRectF QQuickTableViewPrivate::bufferRect()
{
    return viewportRect.adjusted(-cacheBuffer, -cacheBuffer, cacheBuffer, cacheBuffer);
}

void QQuickTableViewPrivate::invalidateTable() {
    tableInvalid = true;
    if (loadRequest.isActive())
        cancelLoadRequest();
    q_func()->polish();
}

void QQuickTableViewPrivate::invalidateColumnRowPositions() {
    columnRowPositionsInvalid = true;
    q_func()->polish();
}

void QQuickTableViewPrivate::updatePolish()
{
    // Whenever something changes, e.g viewport moves, spacing is set to a
    // new value, model changes etc, this function will end up being called. Here
    // we check what needs to be done, and load/unload cells accordingly.

    if (loadRequest.isActive()) {
        // We're currently loading items async to build a new edge in the table. We see the loading
        // as an atomic operation, which means that we don't continue doing anything else until all
        // items have been received and laid out. Note that updatePolish is then called once more
        // after the loadRequest has completed to handle anything that might have occurred in-between.
        return;
    }

    if (tableInvalid) {
        beginRebuildTable();
        if (loadRequest.isActive())
            return;
    }

    if (tableRebuilding)
        endRebuildTable();

    if (loadedItems.isEmpty()) {
        qCDebug(lcTableViewDelegateLifecycle()) << "no items loaded, meaning empty model or row/column == 0";
        return;
    }

    if (columnRowPositionsInvalid)
        relayoutTable();

    if (hasBufferedItems && nextEdgeToLoad(viewportRect)) {
        // We are about to load more edges, so trim down the table as much
        // as possible to avoid loading cells that are outside the viewport.
        unloadBuffer();
    }

    loadAndUnloadVisibleEdges();

    if (loadRequest.isActive())
        return;

    if (cacheBuffer > 0) {
        // When polish hasn't been called for a while (which means that the viewport
        // rect hasn't changed), we start buffering items. We delay this operation by
        // using a timer to increase performance (by not loading hidden items) while
        // the user is flicking.
        cacheBufferDelayTimer.start(kBufferTimerInterval);
    }
}

void QQuickTableViewPrivate::createWrapperModel()
{
    Q_Q(QQuickTableView);

    auto delegateModel = new QQmlDelegateModel(qmlContext(q), q);
    if (q->isComponentComplete())
        delegateModel->componentComplete();
    model = delegateModel;
}

void QQuickTableViewPrivate::itemCreatedCallback(int modelIndex, QObject*)
{
    if (blockItemCreatedCallback)
        return;

    qCDebug(lcTableViewDelegateLifecycle) << "item done loading:"
        << cellAtModelIndex(modelIndex);

    // Since the item we waited for has finished incubating, we can
    // continue with the load request. processLoadRequest will
    // ask the model for the requested item once more, which will be
    // quick since the model has cached it.
    processLoadRequest();
    loadAndUnloadVisibleEdges();
    updatePolish();
}

void QQuickTableViewPrivate::initItemCallback(int modelIndex, QObject *object)
{
    Q_UNUSED(modelIndex);
    auto attached = getAttachedObject(object);
    if (!attached)
        return;

    // Even though row and column is injected directly into the context of a delegate item
    // from QQmlDelegateModel and its model classes, they will only return which row and
    // column an item represents in the model. This might be different from which
    // cell an item ends up in in the Table, if a different rows/columns has been set
    // on it (which is typically the case for list models). For those cases, Table.row
    // and Table.column can be helpful.
    QPoint cell = cellAtModelIndex(modelIndex);
    attached->setTableView(q_func());
    attached->setColumn(cell.x());
    attached->setRow(cell.y());
}

void QQuickTableViewPrivate::modelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    Q_UNUSED(changeSet);
    Q_UNUSED(reset);
    Q_Q(QQuickTableView);

    if (!q->isComponentComplete())
        return;

    // Rudimentary solution for now until support for
    // more fine-grained updates and transitions are implemented.
    auto modelVariant = q->model();
    q->setModel(QVariant());
    q->setModel(modelVariant);
}

QQuickTableView::QQuickTableView(QQuickItem *parent)
    : QQuickFlickable(*(new QQuickTableViewPrivate), parent)
{
}

int QQuickTableView::rows() const
{
    return d_func()->tableSize.height();
}

int QQuickTableView::columns() const
{
    return d_func()->tableSize.width();
}

qreal QQuickTableView::rowSpacing() const
{
    return d_func()->cellSpacing.height();
}

void QQuickTableView::setRowSpacing(qreal spacing)
{
    Q_D(QQuickTableView);
    if (qFuzzyCompare(d->cellSpacing.height(), spacing))
        return;

    d->cellSpacing.setHeight(spacing);
    d->invalidateColumnRowPositions();
    emit rowSpacingChanged();
}

qreal QQuickTableView::columnSpacing() const
{
    return d_func()->cellSpacing.width();
}

void QQuickTableView::setColumnSpacing(qreal spacing)
{
    Q_D(QQuickTableView);
    if (qFuzzyCompare(d->cellSpacing.width(), spacing))
        return;

    d->cellSpacing.setWidth(spacing);
    d->invalidateColumnRowPositions();
    emit columnSpacingChanged();
}

int QQuickTableView::cacheBuffer() const
{
    return d_func()->cacheBuffer;
}

void QQuickTableView::setCacheBuffer(int newBuffer)
{
    Q_D(QQuickTableView);
    if (d->cacheBuffer == newBuffer || newBuffer < 0)
        return;

    d->cacheBuffer = newBuffer;

    if (newBuffer == 0)
        d->unloadBuffer();

    emit cacheBufferChanged();
    polish();
}

QVariant QQuickTableView::model() const
{
    return d_func()->modelVariant;
}

void QQuickTableView::setModel(const QVariant &newModel)
{
    Q_D(QQuickTableView);

    d->modelVariant = newModel;
    QVariant effectiveModelVariant = d->modelVariant;
    if (effectiveModelVariant.userType() == qMetaTypeId<QJSValue>())
        effectiveModelVariant = effectiveModelVariant.value<QJSValue>().toVariant();

    if (d->model) {
        QObjectPrivate::disconnect(d->model, &QQmlInstanceModel::createdItem, d, &QQuickTableViewPrivate::itemCreatedCallback);
        QObjectPrivate::disconnect(d->model, &QQmlInstanceModel::initItem, d, &QQuickTableViewPrivate::initItemCallback);
        QObjectPrivate::disconnect(d->model, &QQmlInstanceModel::modelUpdated, d, &QQuickTableViewPrivate::modelUpdated);
    }

    const auto instanceModel = qobject_cast<QQmlInstanceModel *>(qvariant_cast<QObject*>(effectiveModelVariant));

    if (instanceModel) {
        if (d->wrapperModel())
            delete d->model;
        d->model = instanceModel;
    } else {
        if (!d->wrapperModel())
            d->createWrapperModel();
        d->wrapperModel()->setModel(effectiveModelVariant);
    }

    QObjectPrivate::connect(d->model, &QQmlInstanceModel::createdItem, d, &QQuickTableViewPrivate::itemCreatedCallback);
    QObjectPrivate::connect(d->model, &QQmlInstanceModel::initItem, d, &QQuickTableViewPrivate::initItemCallback);
    QObjectPrivate::connect(d->model, &QQmlInstanceModel::modelUpdated, d, &QQuickTableViewPrivate::modelUpdated);

    d->invalidateTable();

    emit modelChanged();
}

QQmlComponent *QQuickTableView::delegate() const
{
    if (auto wrapperModel = d_func()->wrapperModel())
        return wrapperModel->delegate();

    return nullptr;
}

void QQuickTableView::setDelegate(QQmlComponent *newDelegate)
{
    Q_D(QQuickTableView);
    if (newDelegate == delegate())
        return;

    if (!d->wrapperModel())
        d->createWrapperModel();

    d->wrapperModel()->setDelegate(newDelegate);
    d->invalidateTable();

    emit delegateChanged();
}

QQuickTableViewAttached *QQuickTableView::qmlAttachedProperties(QObject *obj)
{
    return new QQuickTableViewAttached(obj);
}

void QQuickTableView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTableView);
    QQuickFlickable::geometryChanged(newGeometry, oldGeometry);

    QRectF rect = QRectF(contentX(), contentY(), newGeometry.width(), newGeometry.height());
    if (!rect.isValid())
        return;

    d->viewportRect = rect;
    polish();
}

void QQuickTableView::viewportMoved(Qt::Orientations orientation)
{
    Q_D(QQuickTableView);
    QQuickFlickable::viewportMoved(orientation);

    d->viewportRect = QRectF(contentX(), contentY(), width(), height());
    d->updatePolish();
}

void QQuickTableView::componentComplete()
{
    Q_D(QQuickTableView);

    if (!d->model)
        setModel(QVariant());

    if (auto wrapperModel = d->wrapperModel())
        wrapperModel->componentComplete();

    QQuickFlickable::componentComplete();
}

#include "moc_qquicktableview_p.cpp"

QT_END_NAMESPACE
