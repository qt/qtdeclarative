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
#include <QtQml/private/qqmldelegatemodel_p_p.h>
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
    auto listCopy = loadedItems.values();
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

int QQuickTableViewPrivate::modelIndexAtCell(const QPoint &cell) const
{
    int availableRows = tableSize.height();
    int modelIndex = cell.y() + (cell.x() * availableRows);
    Q_TABLEVIEW_ASSERT(modelIndex < model->count(), modelIndex << cell);
    return modelIndex;
}

QPoint QQuickTableViewPrivate::cellAtModelIndex(int modelIndex) const
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

        // loadedTableOuterRect has already been adjusted for left margin
        currentWidth += tableMargins.right();
        estimatedWith += tableMargins.right();

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

        // loadedTableOuterRect has already been adjusted for top margin
        currentHeight += tableMargins.bottom();
        estimatedHeight += tableMargins.bottom();

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

    if (loadedTable.x() == 0 && loadedTableOuterRect.x() != tableMargins.left()) {
        // The table is at the beginning, but not at the edge of the
        // content view. So move the table to origo.
        loadedTableOuterRect.moveLeft(tableMargins.left());
        layoutNeeded = true;
    } else if (loadedTableOuterRect.x() < 0) {
        // The table is outside the beginning of the content view. Move
        // the whole table inside, and make some room for flicking.
        loadedTableOuterRect.moveLeft(tableMargins.left() + loadedTable.x() == 0 ? 0 : flickMargin);
        layoutNeeded = true;
    }

    if (loadedTable.y() == 0 && loadedTableOuterRect.y() != tableMargins.top()) {
        loadedTableOuterRect.moveTop(tableMargins.top());
        layoutNeeded = true;
    } else if (loadedTableOuterRect.y() < 0) {
        loadedTableOuterRect.moveTop(tableMargins.top() + loadedTable.y() == 0 ? 0 : flickMargin);
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
    const int modelIndex = modelIndexAtCell(cell);
    Q_TABLEVIEW_ASSERT(loadedItems.contains(modelIndex), modelIndex << cell);
    return loadedItems.value(modelIndex);
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
    const int modelIndex = modelIndexAtCell(cell);
    Q_TABLEVIEW_ASSERT(loadedItems.contains(modelIndex), modelIndex << cell);
    releaseItem(loadedItems.take(modelIndex));
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
        return loadedTableOuterRect.left() > fillRect.left() + cellSpacing.width();
    case Qt::RightEdge:
        if (loadedTable.bottomRight().x() >= tableSize.width() - 1)
            return false;
        return loadedTableOuterRect.right() < fillRect.right() - cellSpacing.width();
    case Qt::TopEdge:
        if (loadedTable.topLeft().y() == 0)
            return false;
        return loadedTableOuterRect.top() > fillRect.top() + cellSpacing.height();
    case Qt::BottomEdge:
        if (loadedTable.bottomRight().y() >= tableSize.height() - 1)
            return false;
        return loadedTableOuterRect.bottom() < fillRect.bottom() - cellSpacing.height();
    }

    return false;
}

bool QQuickTableViewPrivate::canUnloadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const
{
    // Note: if there is only one row or column left, we cannot unload, since
    // they are needed as anchor point for further layouting.
    switch (tableEdge) {
    case Qt::LeftEdge:
        if (loadedTable.width() <= 1)
            return false;
        return loadedTableInnerRect.left() < fillRect.left();
    case Qt::RightEdge:
        if (loadedTable.width() <= 1)
            return false;
        return loadedTableInnerRect.right() > fillRect.right();
    case Qt::TopEdge:
        if (loadedTable.height() <= 1)
            return false;
        return loadedTableInnerRect.top() < fillRect.top();
    case Qt::BottomEdge:
        if (loadedTable.height() <= 1)
            return false;
        return loadedTableInnerRect.bottom() > fillRect.bottom();
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

qreal QQuickTableViewPrivate::cellWidth(const QPoint& cell)
{
    // If a delegate item has TableView.cellWidth set, then
    // we prefer that. Otherwise we fall back to use implicitWidth.
    // Using an items width directly is not an option, since we change
    // it during layout (which would also cause problems when recycling items).
    auto const cellItem = loadedTableItem(cell)->item;
    if (auto const attached = getAttachedObject(cellItem)) {
        if (!attached->m_cellWidth.isNull)
            return attached->m_cellWidth;
    }
    return cellItem->implicitWidth();
}

qreal QQuickTableViewPrivate::cellHeight(const QPoint& cell)
{
    // If a delegate item has TableView.cellHeight set, then
    // we prefer that. Otherwise we fall back to use implicitHeight.
    // Using an items height directly is not an option, since we change
    // it during layout (which would also cause problems when recycling items).
    auto const cellItem = loadedTableItem(cell)->item;
    if (auto const attached = getAttachedObject(cellItem)) {
        if (!attached->m_cellHeight.isNull)
            return attached->m_cellHeight;
    }
    return cellItem->implicitHeight();
}

void QQuickTableViewPrivate::calculateColumnWidthsAfterRebuilding()
{
    qreal prevColumnWidth = 0;
    for (int column = loadedTable.left(); column <= loadedTable.right(); ++column) {
        qreal columnWidth = 0;
        for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row)
            columnWidth = qMax(columnWidth, cellWidth(QPoint(column, row)));

        if (columnWidth <= 0)
            columnWidth = kDefaultColumnWidth;

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
        for (int column = loadedTable.left(); column <= loadedTable.right(); ++column)
            rowHeight = qMax(rowHeight, cellHeight(QPoint(column, row)));

        if (rowHeight <= 0)
            rowHeight = kDefaultRowHeight;

        if (rowHeight == prevRowHeight)
            continue;

        rowHeights.append({row, rowHeight});
        prevRowHeight = rowHeight;
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
    for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row)
        columnWidth = qMax(columnWidth, cellWidth(QPoint(column, row)));

    if (columnWidth <= 0)
        columnWidth = kDefaultColumnWidth;

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
    for (int column = loadedTable.left(); column <= loadedTable.right(); ++column)
        rowHeight = qMax(rowHeight, cellHeight(QPoint(column, row)));

    if (rowHeight <= 0)
        rowHeight = kDefaultRowHeight;

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

    if (delegateModel)
        tableSize = QSize(delegateModel->columns(), delegateModel->rows());
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
        // Find the first ColumnRowSize with a column before, or at, the given column
        auto iter = std::upper_bound(columnWidths.constBegin(), columnWidths.constEnd(),
                                     ColumnRowSize{column, -1}, ColumnRowSize::lessThan);

        if (iter == columnWidths.constEnd()) {
            // If the table is not a list, return the size
            // of the last recorded ColumnRowSize.
            if (tableSize.height() > 1)
                return columnWidths.last().size;
        } else {
            // Check if we got an explicit assignment for this column
            if (iter->index == column)
                return iter->size;

            // If the table is not a list, return the size of
            // ColumnRowSize element found before column. Since there
            // is always an element stored for column 0, this is safe.
            // Otherwise we continue, and return the size of the delegate
            // item at the given column instead.
            if (tableSize.height() > 1)
                return (iter - 1)->size;
        }
    }

    // If we have an item loaded at column, return the width of the item.
    if (column >= loadedTable.left() && column <= loadedTable.right())
        return cellWidth(QPoint(column, loadedTable.top()));

    return -1;
}

qreal QQuickTableViewPrivate::rowHeight(int row)
{
    if (!rowHeights.isEmpty()) {
        // Find the ColumnRowSize assignment before, or at, row
        auto iter = std::lower_bound(rowHeights.constBegin(), rowHeights.constEnd(),
                                     ColumnRowSize{row, -1}, ColumnRowSize::lessThan);

        if (iter == rowHeights.constEnd()) {
            // If the table is not a list, return the size
            // of the last recorded ColumnRowSize.
            if (tableSize.width() > 1)
                return rowHeights.last().size;
        } else {
            // Check if we got an explicit assignment for this row
            if (iter->index == row)
                return iter->size;

            // If the table is not a list, return the size of
            // ColumnRowSize element found before row. Since there
            // is always an element stored for row 0, this is safe.
            // Otherwise we continue, and return the size of the delegate
            // item at the given row instead.
            if (tableSize.width() > 1)
                return (iter - 1)->size;
        }
    }

    // If we have an item loaded at row, return the height of the item.
    if (row >= loadedTable.top() && row <= loadedTable.bottom())
        return cellHeight(QPoint(loadedTable.left(), row));

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
        if (width <= 0)
            width = kDefaultColumnWidth;

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
        if (height <= 0)
            height = kDefaultRowHeight;

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

void QQuickTableViewPrivate::layoutVerticalEdge(Qt::Edge tableEdge)
{
    int column = (tableEdge == Qt::LeftEdge) ? loadedTable.left() : loadedTable.right();
    QPoint neighbourDirection = (tableEdge == Qt::LeftEdge) ? kRight : kLeft;
    qreal left = -1;

    qreal width = columnWidth(column);
    if (width <= 0)
        width = kDefaultColumnWidth;

    for (int row = loadedTable.top(); row <= loadedTable.bottom(); ++row) {
        auto fxTableItem = loadedTableItem(QPoint(column, row));
        auto const neighbourItem = itemNextTo(fxTableItem, neighbourDirection);

        QRectF geometry = fxTableItem->geometry();
        geometry.setWidth(width);
        geometry.setHeight(neighbourItem->geometry().height());

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

        qCDebug(lcTableViewDelegateLifecycle()) << "layout item:" << QPoint(column, row) << fxTableItem->geometry();
    }
}

void QQuickTableViewPrivate::layoutHorizontalEdge(Qt::Edge tableEdge)
{
    int row = (tableEdge == Qt::TopEdge) ? loadedTable.top() : loadedTable.bottom();
    QPoint neighbourDirection = (tableEdge == Qt::TopEdge) ? kDown : kUp;
    qreal top = -1;

    qreal height = rowHeight(row);
    if (height <= 0)
        height = kDefaultRowHeight;

    for (int column = loadedTable.left(); column <= loadedTable.right(); ++column) {
        auto fxTableItem = loadedTableItem(QPoint(column, row));
        auto const neighbourItem = itemNextTo(fxTableItem, neighbourDirection);

        QRectF geometry = fxTableItem->geometry();
        geometry.setWidth(neighbourItem->geometry().width());
        geometry.setHeight(height);

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

        qCDebug(lcTableViewDelegateLifecycle()) << "layout item:" << QPoint(column, row) << fxTableItem->geometry();
    }
}

void QQuickTableViewPrivate::layoutTopLeftItem()
{
    // ###todo: support starting with other top-left items than 0,0
    const QPoint cell = loadRequest.firstCell();
    Q_TABLEVIEW_ASSERT(cell == QPoint(0, 0), loadRequest.toString());
    auto topLeftItem = loadedTableItem(cell);
    auto item = topLeftItem->item;

    qreal width = cellWidth(cell);
    qreal height = cellHeight(cell);
    if (width <= 0)
        width = kDefaultColumnWidth;
    if (height <= 0)
        height = kDefaultRowHeight;

    item->setPosition(QPoint(tableMargins.left(), tableMargins.top()));
    item->setSize(QSizeF(width, height));
    topLeftItem->setVisible(true);
    qCDebug(lcTableViewDelegateLifecycle) << "geometry:" << topLeftItem->geometry();
}

void QQuickTableViewPrivate::layoutTableEdgeFromLoadRequest()
{
    switch (loadRequest.edge()) {
    case Qt::LeftEdge:
    case Qt::RightEdge:
        layoutVerticalEdge(loadRequest.edge());
        break;
    case Qt::TopEdge:
    case Qt::BottomEdge:
        layoutHorizontalEdge(loadRequest.edge());
        break;
    default:
        layoutTopLeftItem();
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

        loadedItems.insert(modelIndexAtCell(cell), fxTableItem);
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

    if (model->count() == 0)
        return;

    // Load top-left item. After loaded, loadItemsInsideRect() will take
    // care of filling out the rest of the table.
    loadRequest.begin(QPoint(0, 0), QQmlIncubator::AsynchronousIfNested);
    processLoadRequest();
}

void QQuickTableViewPrivate::unloadEdge(Qt::Edge edge)
{
    unloadItems(rectangleEdge(loadedTable, edge));
    loadedTable = expandedRect(loadedTable, edge, -1);
    syncLoadedTableRectFromLoadedTable();
    qCDebug(lcTableViewDelegateLifecycle) << tableLayoutToString();
}

void QQuickTableViewPrivate::loadEdge(Qt::Edge edge, QQmlIncubator::IncubationMode incubationMode)
{
    QLine cellsToLoad = rectangleEdge(expandedRect(loadedTable, edge, 1), edge);
    loadRequest.begin(cellsToLoad, edge, incubationMode);
    processLoadRequest();
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
        // this point means that the model is empty (or no delegate).
        return;
    }

    const QRectF unloadRect = hasBufferedItems ? bufferRect() : viewportRect;
    bool tableModified;

    do {
        tableModified = false;

        if (Qt::Edge edge = nextEdgeToUnload(unloadRect)) {
            tableModified = true;
            unloadEdge(edge);
        }

        if (Qt::Edge edge = nextEdgeToLoad(viewportRect)) {
            tableModified = true;
            loadEdge(edge, QQmlIncubator::AsynchronousIfNested);
            if (loadRequest.isActive())
                return;
        }
    } while (tableModified);

}

void QQuickTableViewPrivate::loadBuffer()
{
    // Rather than making sure to stop the timer from all locations that can
    // violate the "buffering allowed" state, we just check that we're in the
    // right state here before we start buffering.
    if (cacheBuffer <= 0 || loadRequest.isActive() || loadedItems.isEmpty())
        return;

    qCDebug(lcTableViewDelegateLifecycle());
    const QRectF loadRect = bufferRect();
    while (Qt::Edge edge = nextEdgeToLoad(loadRect)) {
        loadEdge(edge, QQmlIncubator::Asynchronous);
        if (loadRequest.isActive())
            break;
    }

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
    while (Qt::Edge edge = nextEdgeToUnload(viewportRect))
        unloadEdge(edge);
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
    Q_Q(QQuickTableView);

    if (loadRequest.isActive()) {
        // We're currently loading items async to build a new edge in the table. We see the loading
        // as an atomic operation, which means that we don't continue doing anything else until all
        // items have been received and laid out. Note that updatePolish is then called once more
        // after the loadRequest has completed to handle anything that might have occurred in-between.
        return;
    }

    // viewportRect describes the part of the content view that is actually visible. Since a
    // negative width/height can happen (e.g during start-up), we check for this to avoid rebuilding
    // the table (and e.g calculate initial row/column sizes) based on a premature viewport rect.
    viewportRect = QRectF(q->contentX(), q->contentY(), q->width(), q->height());
    if (!viewportRect.isValid())
        return;

    if (tableInvalid) {
        beginRebuildTable();
        if (loadRequest.isActive())
            return;
    }

    if (tableRebuilding)
        endRebuildTable();

    if (loadedItems.isEmpty()) {
        qCDebug(lcTableViewDelegateLifecycle()) << "no items loaded, meaning empty model or no delegate";
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

    delegateModel = new QQmlDelegateModel(qmlContext(q), q);
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

    // TODO: implement fine-grained support for model changes
    invalidateTable();
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

qreal QQuickTableView::topMargin() const
{
    return d_func()->tableMargins.top();
}

void QQuickTableView::setTopMargin(qreal margin)
{
    Q_D(QQuickTableView);
    if (qt_is_nan(margin))
        return;
    if (qFuzzyCompare(d->tableMargins.top(), margin))
        return;

    d->tableMargins.setTop(margin);
    d->invalidateColumnRowPositions();
    emit topMarginChanged();
}

qreal QQuickTableView::bottomMargin() const
{
    return d_func()->tableMargins.bottom();
}

void QQuickTableView::setBottomMargin(qreal margin)
{
    Q_D(QQuickTableView);
    if (qt_is_nan(margin))
        return;
    if (qFuzzyCompare(d->tableMargins.bottom(), margin))
        return;

    d->tableMargins.setBottom(margin);
    d->invalidateColumnRowPositions();
    emit bottomMarginChanged();
}

qreal QQuickTableView::leftMargin() const
{
    return d_func()->tableMargins.left();
}

void QQuickTableView::setLeftMargin(qreal margin)
{
    Q_D(QQuickTableView);
    if (qt_is_nan(margin))
        return;
    if (qFuzzyCompare(d->tableMargins.left(), margin))
        return;

    d->tableMargins.setLeft(margin);
    d->invalidateColumnRowPositions();
    emit leftMarginChanged();
}

qreal QQuickTableView::rightMargin() const
{
    return d_func()->tableMargins.right();
}

void QQuickTableView::setRightMargin(qreal margin)
{
    Q_D(QQuickTableView);
    if (qt_is_nan(margin))
        return;
    if (qFuzzyCompare(d->tableMargins.right(), margin))
        return;

    d->tableMargins.setRight(margin);
    d->invalidateColumnRowPositions();
    emit rightMarginChanged();
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
        if (d->delegateModel)
            delete d->delegateModel;
        d->model = instanceModel;
        d->delegateModel = qmlobject_cast<QQmlDelegateModel *>(instanceModel);
    } else {
        if (!d->delegateModel)
            d->createWrapperModel();
        QQmlDelegateModelPrivate::get(d->delegateModel)->m_useFirstColumnOnly = false;
        d->delegateModel->setModel(effectiveModelVariant);
    }

    Q_ASSERT(d->model);
    QObjectPrivate::connect(d->model, &QQmlInstanceModel::createdItem, d, &QQuickTableViewPrivate::itemCreatedCallback);
    QObjectPrivate::connect(d->model, &QQmlInstanceModel::initItem, d, &QQuickTableViewPrivate::initItemCallback);
    QObjectPrivate::connect(d->model, &QQmlInstanceModel::modelUpdated, d, &QQuickTableViewPrivate::modelUpdated);

    d->invalidateTable();

    emit modelChanged();
}

QQmlComponent *QQuickTableView::delegate() const
{
    Q_D(const QQuickTableView);
    if (d->delegateModel)
        return d->delegateModel->delegate();

    return nullptr;
}

void QQuickTableView::setDelegate(QQmlComponent *newDelegate)
{
    Q_D(QQuickTableView);
    if (newDelegate == delegate())
        return;

    if (!d->delegateModel)
        d->createWrapperModel();

    d->delegateModel->setDelegate(newDelegate);
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
    // We update the viewport rect from within updatePolish to
    // ensure that we update when we're ready to update, and not
    // while we're in the middle of loading/unloading edges.
    d->updatePolish();
}

void QQuickTableView::viewportMoved(Qt::Orientations orientation)
{
    Q_D(QQuickTableView);
    QQuickFlickable::viewportMoved(orientation);
    // We update the viewport rect from within updatePolish to
    // ensure that we update when we're ready to update, and not
    // while we're in the middle of loading/unloading edges.
    d->updatePolish();
}

void QQuickTableView::componentComplete()
{
    Q_D(QQuickTableView);

    if (!d->model)
        setModel(QVariant());

    if (d->delegateModel)
        d->delegateModel->componentComplete();

    QQuickFlickable::componentComplete();
}

#include "moc_qquicktableview_p.cpp"

QT_END_NAMESPACE
