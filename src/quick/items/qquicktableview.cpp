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
#include <QtCore/qdir.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>
#include <QtQmlModels/private/qqmldelegatemodel_p_p.h>
#include <QtQml/private/qqmlincubator_p.h>
#include <QtQmlModels/private/qqmlchangeset_p.h>
#include <QtQml/qqmlinfo.h>

#include <QtQuick/private/qquickflickable_p_p.h>
#include <QtQuick/private/qquickitemviewfxitem_p_p.h>

/*!
    \qmltype TableView
    \inqmlmodule QtQuick
    \since 5.12
    \ingroup qtquick-views
    \inherits Flickable
    \brief Provides a table view of items to display data from a model.

    A TableView has a \l model that defines the data to be displayed, and a
    \l delegate that defines how the data should be displayed.

    TableView inherits \l Flickable. This means that while the model can have
    any number of rows and columns, only a subsection of the table is usually
    visible inside the viewport. As soon as you flick, new rows and columns
    enter the viewport, while old ones exit and are removed from the viewport.
    The rows and columns that move out are reused for building the rows and columns
    that move into the viewport. As such, the TableView support models of any
    size without affecting performance.

    A TableView displays data from models created from built-in QML types
    such as ListModel and XmlListModel, which populates the first column only
    in a TableView. To create models with multiple columns, either use
    \l TableModel or a C++ model that inherits QAbstractItemModel.

    \section1 Example Usage

    \section2 C++ Models

    The following example shows how to create a model from C++ with multiple
    columns:

    \snippet qml/tableview/cpp-tablemodel.h 0

    And then how to use it from QML:

    \snippet qml/tableview/cpp-tablemodel.qml 0

    \section2 QML Models

    For prototyping and displaying very simple data (from a web API, for
    example), \l TableModel can be used:

    \snippet qml/tableview/qml-tablemodel.qml 0

    \section1 Reusing items

    TableView recycles delegate items by default, instead of instantiating from
    the \l delegate whenever new rows and columns are flicked into view. This
    approach gives a huge performance boost, depending on the complexity of the
    delegate.

    When an item is flicked out, it moves to the \e{reuse pool}, which is an
    internal cache of unused items. When this happens, the \l TableView::pooled
    signal is emitted to inform the item about it. Likewise, when the item is
    moved back from the pool, the \l TableView::reused signal is emitted.

    Any item properties that come from the model are updated when the
    item is reused. This includes \c index, \c row, and \c column, but also
    any model roles.

    \note Avoid storing any state inside a delegate. If you do, reset it
    manually on receiving the \l TableView::reused signal.

    If an item has timers or animations, consider pausing them on receiving
    the \l TableView::pooled signal. That way you avoid using the CPU resources
    for items that are not visible. Likewise, if an item has resources that
    cannot be reused, they could be freed up.

    If you don't want to reuse items or if the \l delegate cannot support it,
    you can set the \l reuseItems property to \c false.

    \note While an item is in the pool, it might still be alive and respond
    to connected signals and bindings.

    The following example shows a delegate that animates a spinning rectangle. When
    it is pooled, the animation is temporarily paused:

    \snippet qml/tableview/reusabledelegate.qml 0

    \section1 Row heights and column widths

    When a new column is flicked into view, TableView will determine its width
    by calling the \l columnWidthProvider function. TableView does not store
    row height or column width, as it's designed to support large models
    containing any number of rows and columns. Instead, it will ask the
    application whenever it needs to know.

    TableView uses the largest \c implicitWidth among the items as the column
    width, unless the \l columnWidthProvider property is explicitly set. Once
    the column width is found, all other items in the same column are resized
    to this width, even if new items that are flicked in later have larger
    \c implicitWidth. Setting an explicit \c width on an item is ignored and
    overwritten.

    \note The calculated width of a column is discarded when it is flicked out
    of the viewport, and is recalculated if the column is flicked back in. The
    calculation is always based on the items that are visible when the column
    is flicked in. This means that column width can be different each time,
    depending on which row you're at when the column enters. You should
    therefore have the same \c implicitWidth for all items in a column, or set
    \l columnWidthProvider. The same logic applies for the row height
    calculation.

    If you change the values that a \l rowHeightProvider or a
    \l columnWidthProvider return for rows and columns inside the viewport, you
    must call \l forceLayout. This informs TableView that it needs to use the
    provider functions again to recalculate and update the layout.

    Since Qt 5.13, if you want to hide a specific column, you can return \c 0
    from the \l columnWidthProvider for that column. Likewise, you can return 0
    from the \l rowHeightProvider to hide a row. If you return a negative
    number, TableView will fall back to calculate the size based on the delegate
    items.

    \note The size of a row or column should be a whole number to avoid
    sub-pixel alignment of items.

    The following example shows how to set a simple \c columnWidthProvider
    together with a timer that modifies the values the function returns. When
    the array is modified, \l forceLayout is called to let the changes
    take effect:

    \snippet qml/tableview/tableviewwithprovider.qml 0

    \section1 Overlays and underlays

    All new items that are instantiated from the delegate are parented to the
    \l{Flickable::}{contentItem} with the \c z value, \c 1. You can add your
    own items inside the Tableview, as child items of the Flickable. By
    controlling their \c z value, you can make them be on top of or
    underneath the table items.

    Here is an example that shows how to add some text on top of the table, that
    moves together with the table as you flick:

    \snippet qml/tableview/tableviewwithheader.qml 0
*/

/*!
    \qmlproperty int QtQuick::TableView::rows
    \readonly

    This property holds the number of rows in the table.

    \note \a rows is usually equal to the number of rows in the model, but can
    temporarily differ until all pending model changes have been processed.

    This property is read only.
*/

/*!
    \qmlproperty int QtQuick::TableView::columns
    \readonly

    This property holds the number of columns in the table.

    \note \a columns is usually equal to the number of columns in the model, but
    can temporarily differ until all pending model changes have been processed.

    If the model is a list, columns will be \c 1.

    This property is read only.
*/

/*!
    \qmlproperty real QtQuick::TableView::rowSpacing

    This property holds the spacing between the rows.

    The default value is \c 0.
*/

/*!
    \qmlproperty real QtQuick::TableView::columnSpacing

    This property holds the spacing between the columns.

    The default value is \c 0.
*/

/*!
    \qmlproperty var QtQuick::TableView::rowHeightProvider

    This property can hold a function that returns the row height for each row
    in the model. It is called whenever TableView needs to know the height of
    a specific row. The function takes one argument, \c row, for which the
    TableView needs to know the height.

    Since Qt 5.13, if you want to hide a specific row, you can return \c 0
    height for that row. If you return a negative number, TableView calculates
    the height based on the delegate items.

    \sa columnWidthProvider, {Row heights and column widths}
*/

/*!
    \qmlproperty var QtQuick::TableView::columnWidthProvider

    This property can hold a function that returns the column width for each
    column in the model. It is called whenever TableView needs to know the
    width of a specific column. The function takes one argument, \c column,
    for which the TableView needs to know the width.

    Since Qt 5.13, if you want to hide a specific column, you can return \c 0
    width for that column. If you return a negative number, TableView
    calculates the width based on the delegate items.

    \sa rowHeightProvider, {Row heights and column widths}
*/

/*!
    \qmlproperty model QtQuick::TableView::model
    This property holds the model that provides data for the table.

    The model provides the set of data that is used to create the items
    in the view. Models can be created directly in QML using \l TableModel,
    \l ListModel, \l XmlListModel, or \l ObjectModel, or provided by a custom
    C++ model class. The C++ model must be a subclass of \l QAbstractItemModel
    or a simple list.

    \sa {qml-data-models}{Data Models}
*/

/*!
    \qmlproperty Component QtQuick::TableView::delegate

    The delegate provides a template defining each cell item instantiated by the
    view. The model index is exposed as an accessible \c index property. The same
    applies to \c row and \c column. Properties of the model are also available
    depending upon the type of \l {qml-data-models}{Data Model}.

    A delegate should specify its size using \l{Item::}{implicitWidth} and
    \l {Item::}{implicitHeight}. The TableView lays out the items based on that
    information. Explicit width or height settings are ignored and overwritten.

    \note Delegates are instantiated as needed and may be destroyed at any time.
    They are also reused if the \l reuseItems property is set to \c true. You
    should therefore avoid storing state information in the delegates.

    \sa {Row heights and column widths}, {Reusing items}
*/

/*!
    \qmlproperty bool QtQuick::TableView::reuseItems

    This property holds whether or not items instantiated from the \l delegate
    should be reused. If set to \c false, any currently pooled items
    are destroyed.

    \sa {Reusing items}, TableView::pooled, TableView::reused
*/

/*!
    \qmlproperty real QtQuick::TableView::contentWidth

    This property holds the table width required to accommodate the number of
    columns in the model. This is usually not the same as the \c width of the
    \l view, which means that the table's width could be larger or smaller than
    the viewport width. As a TableView cannot always know the exact width of
    the table without loading all columns in the model, the \c contentWidth is
    usually an estimate based on the initially loaded table.

    If you know what the width of the table will be, assign a value to
    \c contentWidth, to avoid unnecessary calculations and updates to the
    TableView.

    \sa contentHeight, columnWidthProvider
*/

/*!
    \qmlproperty real QtQuick::TableView::contentHeight

    This property holds the table height required to accommodate the number of
    rows in the data model. This is usually not the same as the \c height of the
    \c view, which means that the table's height could be larger or smaller than the
    viewport height. As a TableView cannot always know the exact height of the
    table without loading all rows in the model, the \c contentHeight is
    usually an estimate based on the initially loaded table.

    If you know what the height of the table will be, assign a
    value to \c contentHeight, to avoid unnecessary calculations and updates to
    the TableView.

    \sa contentWidth, rowHeightProvider
*/

/*!
    \qmlmethod QtQuick::TableView::forceLayout

    Responding to changes in the model are batched so that they are handled
    only once per frame. This means the TableView delays showing any changes
    while a script is being run. The same is also true when changing
    properties, such as \l rowSpacing or \l{Item::anchors.leftMargin}{leftMargin}.

    This method forces the TableView to immediately update the layout so
    that any recent changes take effect.

    Calling this function re-evaluates the size and position of each visible
    row and column. This is needed if the functions assigned to
    \l rowHeightProvider or \l columnWidthProvider return different values than
    what is already assigned.
*/

/*!
    \qmlattachedproperty TableView QtQuick::TableView::view

    This attached property holds the view that manages the delegate instance.
    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedsignal QtQuick::TableView::pooled

    This signal is emitted after an item has been added to the reuse
    pool. You can use it to pause ongoing timers or animations inside
    the item, or free up resources that cannot be reused.

    This signal is emitted only if the \l reuseItems property is \c true.

    \sa {Reusing items}, reuseItems, reused
*/

/*!
    \qmlattachedsignal QtQuick::TableView::reused

    This signal is emitted after an item has been reused. At this point, the
    item has been taken out of the pool and placed inside the content view,
    and the model properties such as index, row, and column have been updated.

    Other properties that are not provided by the model does not change when an
    item is reused. You should avoid storing any state inside a delegate, but if
    you do, manually reset that state on receiving this signal.

    This signal is emitted when the item is reused, and not the first time the
    item is created.

    This signal is emitted only if the \l reuseItems property is \c true.

    \sa {Reusing items}, reuseItems, pooled
*/

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTableViewDelegateLifecycle, "qt.quick.tableview.lifecycle")

#define Q_TABLEVIEW_UNREACHABLE(output) { dumpTable(); qWarning() << "output:" << output; Q_UNREACHABLE(); }
#define Q_TABLEVIEW_ASSERT(cond, output) Q_ASSERT((cond) || [&](){ dumpTable(); qWarning() << "output:" << output; return false;}())

static const Qt::Edge allTableEdges[] = { Qt::LeftEdge, Qt::RightEdge, Qt::TopEdge, Qt::BottomEdge };
static const int kEdgeIndexNotSet = -2;
static const int kEdgeIndexAtEnd = -3;

const QPoint QQuickTableViewPrivate::kLeft = QPoint(-1, 0);
const QPoint QQuickTableViewPrivate::kRight = QPoint(1, 0);
const QPoint QQuickTableViewPrivate::kUp = QPoint(0, -1);
const QPoint QQuickTableViewPrivate::kDown = QPoint(0, 1);

QQuickTableViewPrivate::EdgeRange::EdgeRange()
    : startIndex(kEdgeIndexNotSet)
    , endIndex(kEdgeIndexNotSet)
    , size(0)
{}

bool QQuickTableViewPrivate::EdgeRange::containsIndex(Qt::Edge edge, int index)
{
    if (startIndex == kEdgeIndexNotSet)
        return false;

    if (endIndex == kEdgeIndexAtEnd) {
        switch (edge) {
        case Qt::LeftEdge:
        case Qt::TopEdge:
            return index <= startIndex;
        case Qt::RightEdge:
        case Qt::BottomEdge:
            return index >= startIndex;
        }
    }

    const int s = std::min(startIndex, endIndex);
    const int e = std::max(startIndex, endIndex);
    return index >= s && index <= e;
}

QQuickTableViewPrivate::QQuickTableViewPrivate()
    : QQuickFlickablePrivate()
{
    QObject::connect(&columnWidths, &QQuickTableSectionSizeProvider::sizeChanged,
                            [this] { this->forceLayout();});
    QObject::connect(&rowHeights, &QQuickTableSectionSizeProvider::sizeChanged,
                            [this] { this->forceLayout();});
}

QQuickTableViewPrivate::~QQuickTableViewPrivate()
{
    for (auto *fxTableItem : loadedItems) {
        if (auto item = fxTableItem->item) {
            if (fxTableItem->ownItem)
                delete item;
            else if (tableModel)
                tableModel->dispose(item);
        }
        delete fxTableItem;
    }

    if (tableModel)
        delete tableModel;
}

QString QQuickTableViewPrivate::tableLayoutToString() const
{
    if (loadedItems.isEmpty())
        return QLatin1String("table is empty!");
    return QString(QLatin1String("table cells: (%1,%2) -> (%3,%4), item count: %5, table rect: %6,%7 x %8,%9"))
            .arg(leftColumn()).arg(topRow())
            .arg(rightColumn()).arg(bottomRow())
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

    const QString filename = QStringLiteral("QQuickTableView_dumptable_capture.png");
    const QString path = QDir::current().absoluteFilePath(filename);
    if (q_func()->window() && q_func()->window()->grabWindow().save(path))
        qWarning() << "Window capture saved to:" << path;
}

QQuickTableViewAttached *QQuickTableViewPrivate::getAttachedObject(const QObject *object) const
{
    QObject *attachedObject = qmlAttachedPropertiesObject<QQuickTableView>(object);
    return static_cast<QQuickTableViewAttached *>(attachedObject);
}

int QQuickTableViewPrivate::modelIndexAtCell(const QPoint &cell) const
{
    // QQmlTableInstanceModel expects index to be in column-major
    // order. This means that if the view is transposed (with a flipped
    // width and height), we need to calculate it in row-major instead.
    if (isTransposed) {
        int availableColumns = tableSize.width();
        return (cell.y() * availableColumns) + cell.x();
    } else {
        int availableRows = tableSize.height();
        return (cell.x() * availableRows) + cell.y();
    }
}

QPoint QQuickTableViewPrivate::cellAtModelIndex(int modelIndex) const
{
    // QQmlTableInstanceModel expects index to be in column-major
    // order. This means that if the view is transposed (with a flipped
    // width and height), we need to calculate it in row-major instead.
    if (isTransposed) {
        int availableColumns = tableSize.width();
        int row = int(modelIndex / availableColumns);
        int column = modelIndex % availableColumns;
        return QPoint(column, row);
    } else {
        int availableRows = tableSize.height();
        int column = int(modelIndex / availableRows);
        int row = modelIndex % availableRows;
        return QPoint(column, row);
    }
}

int QQuickTableViewPrivate::edgeToArrayIndex(Qt::Edge edge)
{
    return int(log2(float(edge)));
}

void QQuickTableViewPrivate::clearEdgeSizeCache()
{
    cachedColumnWidth.startIndex = kEdgeIndexNotSet;
    cachedRowHeight.startIndex = kEdgeIndexNotSet;

    for (Qt::Edge edge : allTableEdges)
        cachedNextVisibleEdgeIndex[edgeToArrayIndex(edge)].startIndex = kEdgeIndexNotSet;
}

int QQuickTableViewPrivate::nextVisibleEdgeIndexAroundLoadedTable(Qt::Edge edge)
{
    // Find the next column (or row) around the loaded table that is
    // visible, and should be loaded next if the content item moves.
    int startIndex = -1;
    switch (edge) {
    case Qt::LeftEdge: startIndex = loadedColumns.firstKey() - 1; break;
    case Qt::RightEdge: startIndex = loadedColumns.lastKey() + 1; break;
    case Qt::TopEdge: startIndex = loadedRows.firstKey() - 1; break;
    case Qt::BottomEdge: startIndex = loadedRows.lastKey() + 1; break;
    }

    return nextVisibleEdgeIndex(edge, startIndex);
}

int QQuickTableViewPrivate::nextVisibleEdgeIndex(Qt::Edge edge, int startIndex)
{
    // First check if we have already searched for the first visible index
    // after the given startIndex recently, and if so, return the cached result.
    // The cached result is valid if startIndex is inside the range between the
    // startIndex and the first visible index found after it.
    auto &cachedResult = cachedNextVisibleEdgeIndex[edgeToArrayIndex(edge)];
    if (cachedResult.containsIndex(edge, startIndex))
        return cachedResult.endIndex;

    // Search for the first column (or row) in the direction of edge that is
    // visible, starting from the given column (startIndex).
    int foundIndex = kEdgeIndexNotSet;
    int testIndex = startIndex;

    switch (edge) {
    case Qt::LeftEdge: {
        forever {
            if (testIndex < 0) {
                foundIndex = kEdgeIndexAtEnd;
                break;
            }

            if (!isColumnHidden(testIndex)) {
                foundIndex = testIndex;
                break;
            }

            --testIndex;
        }
        break; }
    case Qt::RightEdge: {
        forever {
            if (testIndex > tableSize.width() - 1) {
                foundIndex = kEdgeIndexAtEnd;
                break;
            }

            if (!isColumnHidden(testIndex)) {
                foundIndex = testIndex;
                break;
            }

            ++testIndex;
        }
        break; }
    case Qt::TopEdge: {
        forever {
            if (testIndex < 0) {
                foundIndex = kEdgeIndexAtEnd;
                break;
            }

            if (!isRowHidden(testIndex)) {
                foundIndex = testIndex;
                break;
            }

            --testIndex;
        }
        break; }
    case Qt::BottomEdge: {
        forever {
            if (testIndex > tableSize.height() - 1) {
                foundIndex = kEdgeIndexAtEnd;
                break;
            }

            if (!isRowHidden(testIndex)) {
                foundIndex = testIndex;
                break;
            }

            ++testIndex;
        }
        break; }
    }

    cachedResult.startIndex = startIndex;
    cachedResult.endIndex = foundIndex;
    return foundIndex;
}

void QQuickTableViewPrivate::updateContentWidth()
{
    // Note that we actually never really know what the content size / size of the full table will
    // be. Even if e.g spacing changes, and we normally would assume that the size of the table
    // would increase accordingly, the model might also at some point have removed/hidden/resized
    // rows/columns outside the viewport. This would also affect the size, but since we don't load
    // rows or columns outside the viewport, this information is ignored. And even if we did, we
    // might also have been fast-flicked to a new location at some point, and started a new rebuild
    // there based on a new guesstimated top-left cell. So the calculated content size should always
    // be understood as a guesstimate, which sometimes can be really off (as a tradeoff for performance).
    // When this is not acceptable, the user can always set a custom content size explicitly.
    Q_Q(QQuickTableView);

    if (syncHorizontally) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        q->QQuickFlickable::setContentWidth(syncView->contentWidth());
        return;
    }

    if (explicitContentWidth.isValid()) {
        // Don't calculate contentWidth when it
        // was set explicitly by the application.
        return;
    }

    if (loadedItems.isEmpty()) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        q->QQuickFlickable::setContentWidth(0);
        return;
    }

    const int nextColumn = nextVisibleEdgeIndexAroundLoadedTable(Qt::RightEdge);
    const int columnsRemaining = nextColumn == kEdgeIndexAtEnd ? 0 : tableSize.width() - nextColumn;
    const qreal remainingColumnWidths = columnsRemaining * averageEdgeSize.width();
    const qreal remainingSpacing = columnsRemaining * cellSpacing.width();
    const qreal estimatedRemainingWidth = remainingColumnWidths + remainingSpacing;
    const qreal estimatedWidth = loadedTableOuterRect.right() + estimatedRemainingWidth;

    QBoolBlocker fixupGuard(inUpdateContentSize, true);
    q->QQuickFlickable::setContentWidth(estimatedWidth);
}

void QQuickTableViewPrivate::updateContentHeight()
{
    Q_Q(QQuickTableView);

    if (syncVertically) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        q->QQuickFlickable::setContentHeight(syncView->contentHeight());
        return;
    }

    if (explicitContentHeight.isValid()) {
        // Don't calculate contentHeight when it
        // was set explicitly by the application.
        return;
    }

    if (loadedItems.isEmpty()) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        q->QQuickFlickable::setContentHeight(0);
        return;
    }

    const int nextRow = nextVisibleEdgeIndexAroundLoadedTable(Qt::BottomEdge);
    const int rowsRemaining = nextRow == kEdgeIndexAtEnd ? 0 : tableSize.height() - nextRow;
    const qreal remainingRowHeights = rowsRemaining * averageEdgeSize.height();
    const qreal remainingSpacing = rowsRemaining * cellSpacing.height();
    const qreal estimatedRemainingHeight = remainingRowHeights + remainingSpacing;
    const qreal estimatedHeight = loadedTableOuterRect.bottom() + estimatedRemainingHeight;

    QBoolBlocker fixupGuard(inUpdateContentSize, true);
    q->QQuickFlickable::setContentHeight(estimatedHeight);
}

void QQuickTableViewPrivate::updateExtents()
{
    // When rows or columns outside the viewport are removed or added, or a rebuild
    // forces us to guesstimate a new top-left, the edges of the table might end up
    // out of sync with the edges of the content view. We detect this situation here, and
    // move the origin to ensure that there will never be gaps at the end of the table.
    // Normally we detect that the size of the whole table is not going to be equal to the
    // size of the content view already when we load the last row/column, and especially
    // before it's flicked completely inside the viewport. For those cases we simply adjust
    // the origin/endExtent, to give a smooth flicking experience.
    // But if flicking fast (e.g with a scrollbar), it can happen that the viewport ends up
    // outside the end of the table in just one viewport update. To avoid a "blink" in the
    // viewport when that happens, we "move" the loaded table into the viewport to cover it.
    Q_Q(QQuickTableView);

    bool tableMovedHorizontally = false;
    bool tableMovedVertically = false;

    const int nextLeftColumn = nextVisibleEdgeIndexAroundLoadedTable(Qt::LeftEdge);
    const int nextRightColumn = nextVisibleEdgeIndexAroundLoadedTable(Qt::RightEdge);
    const int nextTopRow = nextVisibleEdgeIndexAroundLoadedTable(Qt::TopEdge);
    const int nextBottomRow = nextVisibleEdgeIndexAroundLoadedTable(Qt::BottomEdge);

    if (syncHorizontally) {
        const auto syncView_d = syncView->d_func();
        origin.rx() = syncView_d->origin.x();
        endExtent.rwidth() = syncView_d->endExtent.width();
        hData.markExtentsDirty();
    } else if (nextLeftColumn == kEdgeIndexAtEnd) {
        // There are no more columns to load on the left side of the table.
        // In that case, we ensure that the origin match the beginning of the table.
        if (loadedTableOuterRect.left() > viewportRect.left()) {
            // We have a blank area at the left end of the viewport. In that case we don't have time to
            // wait for the viewport to move (after changing origin), since that will take an extra
            // update cycle, which will be visible as a blink. Instead, unless the blank spot is just
            // us overshooting, we brute force the loaded table inside the already existing viewport.
            if (loadedTableOuterRect.left() > origin.x()) {
                const qreal diff = loadedTableOuterRect.left() - origin.x();
                loadedTableOuterRect.moveLeft(loadedTableOuterRect.left() - diff);
                loadedTableInnerRect.moveLeft(loadedTableInnerRect.left() - diff);
                tableMovedHorizontally = true;
            }
        }
        origin.rx() = loadedTableOuterRect.left();
        hData.markExtentsDirty();
    } else if (loadedTableOuterRect.left() <= origin.x() + cellSpacing.width()) {
        // The table rect is at the origin, or outside, but we still have more
        // visible columns to the left. So we try to guesstimate how much space
        // the rest of the columns will occupy, and move the origin accordingly.
        const int columnsRemaining = nextLeftColumn + 1;
        const qreal remainingColumnWidths = columnsRemaining * averageEdgeSize.width();
        const qreal remainingSpacing = columnsRemaining * cellSpacing.width();
        const qreal estimatedRemainingWidth = remainingColumnWidths + remainingSpacing;
        origin.rx() = loadedTableOuterRect.left() - estimatedRemainingWidth;
        hData.markExtentsDirty();
    } else if (nextRightColumn == kEdgeIndexAtEnd) {
        // There are no more columns to load on the right side of the table.
        // In that case, we ensure that the end of the content view match the end of the table.
        if (loadedTableOuterRect.right() < viewportRect.right()) {
            // We have a blank area at the right end of the viewport. In that case we don't have time to
            // wait for the viewport to move (after changing endExtent), since that will take an extra
            // update cycle, which will be visible as a blink. Instead, unless the blank spot is just
            // us overshooting, we brute force the loaded table inside the already existing viewport.
            const qreal w = qMin(viewportRect.right(), q->contentWidth() + endExtent.width());
            if (loadedTableOuterRect.right() < w) {
                const qreal diff = loadedTableOuterRect.right() - w;
                loadedTableOuterRect.moveRight(loadedTableOuterRect.right() - diff);
                loadedTableInnerRect.moveRight(loadedTableInnerRect.right() - diff);
                tableMovedHorizontally = true;
            }
        }
        endExtent.rwidth() = loadedTableOuterRect.right() - q->contentWidth();
        hData.markExtentsDirty();
    } else if (loadedTableOuterRect.right() >= q->contentWidth() + endExtent.width() - cellSpacing.width()) {
        // The right-most column is outside the end of the content view, and we
        // still have more visible columns in the model. This can happen if the application
        // has set a fixed content width.
        const int columnsRemaining = tableSize.width() - nextRightColumn;
        const qreal remainingColumnWidths = columnsRemaining * averageEdgeSize.width();
        const qreal remainingSpacing = columnsRemaining * cellSpacing.width();
        const qreal estimatedRemainingWidth = remainingColumnWidths + remainingSpacing;
        const qreal pixelsOutsideContentWidth = loadedTableOuterRect.right() - q->contentWidth();
        endExtent.rwidth() = pixelsOutsideContentWidth + estimatedRemainingWidth;
        hData.markExtentsDirty();
    }

    if (syncVertically) {
        const auto syncView_d = syncView->d_func();
        origin.ry() = syncView_d->origin.y();
        endExtent.rheight() = syncView_d->endExtent.height();
        vData.markExtentsDirty();
    } else if (nextTopRow == kEdgeIndexAtEnd) {
        // There are no more rows to load on the top side of the table.
        // In that case, we ensure that the origin match the beginning of the table.
        if (loadedTableOuterRect.top() > viewportRect.top()) {
            // We have a blank area at the top of the viewport. In that case we don't have time to
            // wait for the viewport to move (after changing origin), since that will take an extra
            // update cycle, which will be visible as a blink. Instead, unless the blank spot is just
            // us overshooting, we brute force the loaded table inside the already existing viewport.
            if (loadedTableOuterRect.top() > origin.y()) {
                const qreal diff = loadedTableOuterRect.top() - origin.y();
                loadedTableOuterRect.moveTop(loadedTableOuterRect.top() - diff);
                loadedTableInnerRect.moveTop(loadedTableInnerRect.top() - diff);
                tableMovedVertically = true;
            }
        }
        origin.ry() = loadedTableOuterRect.top();
        vData.markExtentsDirty();
    } else if (loadedTableOuterRect.top() <= origin.y() + cellSpacing.height()) {
        // The table rect is at the origin, or outside, but we still have more
        // visible rows at the top. So we try to guesstimate how much space
        // the rest of the rows will occupy, and move the origin accordingly.
        const int rowsRemaining = nextTopRow + 1;
        const qreal remainingRowHeights = rowsRemaining * averageEdgeSize.height();
        const qreal remainingSpacing = rowsRemaining * cellSpacing.height();
        const qreal estimatedRemainingHeight = remainingRowHeights + remainingSpacing;
        origin.ry() = loadedTableOuterRect.top() - estimatedRemainingHeight;
        vData.markExtentsDirty();
    } else if (nextBottomRow == kEdgeIndexAtEnd) {
        // There are no more rows to load on the bottom side of the table.
        // In that case, we ensure that the end of the content view match the end of the table.
        if (loadedTableOuterRect.bottom() < viewportRect.bottom()) {
            // We have a blank area at the bottom of the viewport. In that case we don't have time to
            // wait for the viewport to move (after changing endExtent), since that will take an extra
            // update cycle, which will be visible as a blink. Instead, unless the blank spot is just
            // us overshooting, we brute force the loaded table inside the already existing viewport.
            const qreal h = qMin(viewportRect.bottom(), q->contentHeight() + endExtent.height());
            if (loadedTableOuterRect.bottom() < h) {
                const qreal diff = loadedTableOuterRect.bottom() - h;
                loadedTableOuterRect.moveBottom(loadedTableOuterRect.bottom() - diff);
                loadedTableInnerRect.moveBottom(loadedTableInnerRect.bottom() - diff);
                tableMovedVertically = true;
            }
        }
        endExtent.rheight() = loadedTableOuterRect.bottom() - q->contentHeight();
        vData.markExtentsDirty();
    } else if (loadedTableOuterRect.bottom() >= q->contentHeight() + endExtent.height() - cellSpacing.height()) {
        // The bottom-most row is outside the end of the content view, and we
        // still have more visible rows in the model. This can happen if the application
        // has set a fixed content height.
        const int rowsRemaining = tableSize.height() - nextBottomRow;
        const qreal remainingRowHeigts = rowsRemaining * averageEdgeSize.height();
        const qreal remainingSpacing = rowsRemaining * cellSpacing.height();
        const qreal estimatedRemainingHeight = remainingRowHeigts + remainingSpacing;
        const qreal pixelsOutsideContentHeight = loadedTableOuterRect.bottom() - q->contentHeight();
        endExtent.rheight() = pixelsOutsideContentHeight + estimatedRemainingHeight;
        vData.markExtentsDirty();
    }

    if (tableMovedHorizontally || tableMovedVertically) {
        qCDebug(lcTableViewDelegateLifecycle) << "move table to" << loadedTableOuterRect;

        // relayoutTableItems() will take care of moving the existing
        // delegate items into the new loadedTableOuterRect.
        relayoutTableItems();

        // Inform the sync children that they need to rebuild to stay in sync
        for (auto syncChild : qAsConst(syncChildren)) {
            auto syncChild_d = syncChild->d_func();
            syncChild_d->scheduledRebuildOptions |= RebuildOption::ViewportOnly;
            if (tableMovedHorizontally)
                syncChild_d->scheduledRebuildOptions |= RebuildOption::CalculateNewTopLeftColumn;
            if (tableMovedVertically)
                syncChild_d->scheduledRebuildOptions |= RebuildOption::CalculateNewTopLeftRow;
        }
    }

    if (hData.minExtentDirty || vData.minExtentDirty) {
        qCDebug(lcTableViewDelegateLifecycle) << "move origin and endExtent to:" << origin << endExtent;
        // updateBeginningEnd() will let the new extents take effect. This will also change the
        // visualArea of the flickable, which again will cause any attached scrollbars to adjust
        // the position of the handle. Note the latter will cause the viewport to move once more.
        updateBeginningEnd();
    }
}

void QQuickTableViewPrivate::updateAverageColumnWidth()
{
    if (explicitContentWidth.isValid()) {
        const qreal accColumnSpacing = (tableSize.width() - 1) * cellSpacing.width();
        averageEdgeSize.setWidth((explicitContentWidth - accColumnSpacing) / tableSize.width());
    } else {
        const qreal accColumnSpacing = (loadedColumns.count() - 1) * cellSpacing.width();
        averageEdgeSize.setWidth((loadedTableOuterRect.width() - accColumnSpacing) / loadedColumns.count());
    }
}

void QQuickTableViewPrivate::updateAverageRowHeight()
{
    if (explicitContentHeight.isValid()) {
        const qreal accRowSpacing = (tableSize.height() - 1) * cellSpacing.height();
        averageEdgeSize.setHeight((explicitContentHeight - accRowSpacing) / tableSize.height());
    } else {
        const qreal accRowSpacing = (loadedRows.count() - 1) * cellSpacing.height();
        averageEdgeSize.setHeight((loadedTableOuterRect.height() - accRowSpacing) / loadedRows.count());
    }
}

void QQuickTableViewPrivate::syncLoadedTableRectFromLoadedTable()
{
    const QPoint topLeft = QPoint(leftColumn(), topRow());
    const QPoint bottomRight = QPoint(rightColumn(), bottomRow());
    QRectF topLeftRect = loadedTableItem(topLeft)->geometry();
    QRectF bottomRightRect = loadedTableItem(bottomRight)->geometry();
    loadedTableOuterRect = QRectF(topLeftRect.topLeft(), bottomRightRect.bottomRight());
    loadedTableInnerRect = QRectF(topLeftRect.bottomRight(), bottomRightRect.topLeft());
}

QQuickTableViewPrivate::RebuildOptions QQuickTableViewPrivate::checkForVisibilityChanges()
{
    // Go through all columns from first to last, find the columns that used
    // to be hidden and not loaded, and check if they should become visible
    // (and vice versa). If there is a change, we need to rebuild.
    RebuildOptions rebuildOptions = RebuildOption::None;

    for (int column = leftColumn(); column <= rightColumn(); ++column) {
        const bool wasVisibleFromBefore = loadedColumns.contains(column);
        const bool isVisibleNow = !qFuzzyIsNull(getColumnWidth(column));
        if (wasVisibleFromBefore == isVisibleNow)
            continue;

        // A column changed visibility. This means that it should
        // either be loaded or unloaded. So we need a rebuild.
        qCDebug(lcTableViewDelegateLifecycle) << "Column" << column << "changed visibility to" << isVisibleNow;
        rebuildOptions.setFlag(RebuildOption::ViewportOnly);
        if (column == leftColumn()) {
            // The first loaded column should now be hidden. This means that we
            // need to calculate which column should now be first instead.
            rebuildOptions.setFlag(RebuildOption::CalculateNewTopLeftColumn);
        }
        break;
    }

    // Go through all rows from first to last, and do the same as above
    for (int row = topRow(); row <= bottomRow(); ++row) {
        const bool wasVisibleFromBefore = loadedRows.contains(row);
        const bool isVisibleNow = !qFuzzyIsNull(getRowHeight(row));
        if (wasVisibleFromBefore == isVisibleNow)
            continue;

        // A row changed visibility. This means that it should
        // either be loaded or unloaded. So we need a rebuild.
        qCDebug(lcTableViewDelegateLifecycle) << "Row" << row << "changed visibility to" << isVisibleNow;
        rebuildOptions.setFlag(RebuildOption::ViewportOnly);
        if (row == topRow())
            rebuildOptions.setFlag(RebuildOption::CalculateNewTopLeftRow);
        break;
    }

    return rebuildOptions;
}

void QQuickTableViewPrivate::forceLayout()
{
    if (loadedItems.isEmpty())
        return;

    clearEdgeSizeCache();
    RebuildOptions rebuildOptions = RebuildOption::None;

    const QSize actualTableSize = calculateTableSize();
    if (tableSize != actualTableSize) {
        // This can happen if the app is calling forceLayout while
        // the model is updated, but before we're notified about it.
        rebuildOptions = RebuildOption::All;
    } else {
        // Resizing a column (or row) can result in the table going from being
        // e.g completely inside the viewport to go outside. And in the latter
        // case, the user needs to be able to scroll the viewport, also if
        // flags such as Flickable.StopAtBounds is in use. So we need to
        // update contentWidth/Height to support that case.
        rebuildOptions = RebuildOption::LayoutOnly
                | RebuildOption::CalculateNewContentWidth
                | RebuildOption::CalculateNewContentHeight
                | checkForVisibilityChanges();
    }

    scheduleRebuildTable(rebuildOptions);

    auto rootView = rootSyncView();
    const bool updated = rootView->d_func()->updateTableRecursive();
    if (!updated) {
        qWarning() << "TableView::forceLayout(): Cannot do an immediate re-layout during an ongoing layout!";
        rootView->polish();
    }
}

void QQuickTableViewPrivate::syncLoadedTableFromLoadRequest()
{
    if (loadRequest.edge() == Qt::Edge(0)) {
        // No edge means we're loading the top-left item
        loadedColumns.insert(loadRequest.column(), 0);
        loadedRows.insert(loadRequest.row(), 0);
        return;
    }

    switch (loadRequest.edge()) {
    case Qt::LeftEdge:
    case Qt::RightEdge:
        loadedColumns.insert(loadRequest.column(), 0);
        break;
    case Qt::TopEdge:
    case Qt::BottomEdge:
        loadedRows.insert(loadRequest.row(), 0);
        break;
    }
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
    } else {
        QQuickAnchors *anchors = QQuickItemPrivate::get(item)->_anchors;
        if (anchors && anchors->activeDirections())
            qmlWarning(item) << "TableView: detected anchors on delegate with index: " << modelIndex
                             << ". Use implicitWidth and implicitHeight instead.";
    }

    if (ownItem) {
        // Parent item is normally set early on from initItemCallback (to
        // allow bindings to the parent property). But if we created the item
        // within this function, we need to set it explicit.
        item->setImplicitWidth(kDefaultColumnWidth);
        item->setImplicitHeight(kDefaultRowHeight);
        item->setParentItem(q->contentItem());
    }
    Q_TABLEVIEW_ASSERT(item->parentItem() == q->contentItem(), item->parentItem());

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

void QQuickTableViewPrivate::releaseLoadedItems(QQmlTableInstanceModel::ReusableFlag reusableFlag) {
    // Make a copy and clear the list of items first to avoid destroyed
    // items being accessed during the loop (QTBUG-61294)
    auto const tmpList = loadedItems;
    loadedItems.clear();
    for (FxTableItem *item : tmpList)
        releaseItem(item, reusableFlag);
}

void QQuickTableViewPrivate::releaseItem(FxTableItem *fxTableItem, QQmlTableInstanceModel::ReusableFlag reusableFlag)
{
    Q_Q(QQuickTableView);
    // Note that fxTableItem->item might already have been destroyed, in case
    // the item is owned by the QML context rather than the model (e.g ObjectModel etc).
    auto item = fxTableItem->item;

    if (fxTableItem->ownItem) {
        Q_TABLEVIEW_ASSERT(item, fxTableItem->index);
        delete item;
    } else if (item) {
        auto releaseFlag = model->release(item, reusableFlag);
        if (releaseFlag == QQmlInstanceModel::Pooled) {
            fxTableItem->setVisible(false);

            // If the item (or a descendant) has focus, remove it, so
            // that the item doesn't enter with focus when it's reused.
            if (QQuickWindow *window = item->window()) {
                const auto focusItem = qobject_cast<QQuickItem *>(window->focusObject());
                if (focusItem) {
                    const bool hasFocus = item == focusItem || item->isAncestorOf(focusItem);
                    if (hasFocus) {
                        const auto focusChild = QQuickItemPrivate::get(q)->subFocusItem;
                        QQuickWindowPrivate::get(window)->clearFocusInScope(q, focusChild, Qt::OtherFocusReason);
                    }
                }
            }
        }
    }

    delete fxTableItem;
}

void QQuickTableViewPrivate::unloadItem(const QPoint &cell)
{
    const int modelIndex = modelIndexAtCell(cell);
    Q_TABLEVIEW_ASSERT(loadedItems.contains(modelIndex), modelIndex << cell);
    releaseItem(loadedItems.take(modelIndex), reusableFlag);
}

bool QQuickTableViewPrivate::canLoadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const
{
    switch (tableEdge) {
    case Qt::LeftEdge:
        return loadedTableOuterRect.left() > fillRect.left() + cellSpacing.width();
    case Qt::RightEdge:
        return loadedTableOuterRect.right() < fillRect.right() - cellSpacing.width();
    case Qt::TopEdge:
        return loadedTableOuterRect.top() > fillRect.top() + cellSpacing.height();
    case Qt::BottomEdge:
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
        if (loadedColumns.count() <= 1)
            return false;
        return loadedTableInnerRect.left() <= fillRect.left();
    case Qt::RightEdge:
        if (loadedColumns.count() <= 1)
            return false;
        return loadedTableInnerRect.right() >= fillRect.right();
    case Qt::TopEdge:
        if (loadedRows.count() <= 1)
            return false;
        return loadedTableInnerRect.top() <= fillRect.top();
    case Qt::BottomEdge:
        if (loadedRows.count() <= 1)
            return false;
        return loadedTableInnerRect.bottom() >= fillRect.bottom();
    }
    Q_TABLEVIEW_UNREACHABLE(tableEdge);
    return false;
}

Qt::Edge QQuickTableViewPrivate::nextEdgeToLoad(const QRectF rect)
{
    for (Qt::Edge edge : allTableEdges) {
        if (!canLoadTableEdge(edge, rect))
            continue;
        const int nextIndex = nextVisibleEdgeIndexAroundLoadedTable(edge);
        if (nextIndex == kEdgeIndexAtEnd)
            continue;
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
    // Using an items width directly is not an option, since we change
    // it during layout (which would also cause problems when recycling items).
    auto const cellItem = loadedTableItem(cell)->item;
    return cellItem->implicitWidth();
}

qreal QQuickTableViewPrivate::cellHeight(const QPoint& cell)
{
    // Using an items height directly is not an option, since we change
    // it during layout (which would also cause problems when recycling items).
    auto const cellItem = loadedTableItem(cell)->item;
    return cellItem->implicitHeight();
}

qreal QQuickTableViewPrivate::sizeHintForColumn(int column)
{
    // Find the widest cell in the column, and return its width
    qreal columnWidth = 0;
    for (auto r = loadedRows.cbegin(); r != loadedRows.cend(); ++r) {
        const int row = r.key();
        columnWidth = qMax(columnWidth, cellWidth(QPoint(column, row)));
    }

    return columnWidth;
}

qreal QQuickTableViewPrivate::sizeHintForRow(int row)
{
    // Find the highest cell in the row, and return its height
    qreal rowHeight = 0;
    for (auto c = loadedColumns.cbegin(); c != loadedColumns.cend(); ++c) {
        const int column = c.key();
        rowHeight = qMax(rowHeight, cellHeight(QPoint(column, row)));
    }

    return rowHeight;
}

void QQuickTableViewPrivate::updateTableSize()
{
    // tableSize is the same as row and column count, and will always
    // be the same as the number of rows and columns in the model.
    Q_Q(QQuickTableView);

    const QSize prevTableSize = tableSize;
    tableSize = calculateTableSize();

    if (prevTableSize.width() != tableSize.width())
        emit q->columnsChanged();
    if (prevTableSize.height() != tableSize.height())
        emit q->rowsChanged();
}

QSize QQuickTableViewPrivate::calculateTableSize()
{
    QSize size(0, 0);
    if (tableModel)
        size = QSize(tableModel->columns(), tableModel->rows());
    else if (model)
        size = QSize(1, model->count());

    return isTransposed ? size.transposed() : size;
}

qreal QQuickTableViewPrivate::getColumnLayoutWidth(int column)
{
    // Return the column width specified by the application, or go
    // through the loaded items and calculate it as a fallback. For
    // layouting, the width can never be zero (or negative), as this
    // can lead us to be stuck in an infinite loop trying to load and
    // fill out the empty viewport space with empty columns.
    const qreal explicitColumnWidth = getColumnWidth(column);
    if (explicitColumnWidth >= 0)
        return explicitColumnWidth;

    if (syncHorizontally) {
        if (syncView->d_func()->loadedColumns.contains(column))
            return syncView->d_func()->getColumnLayoutWidth(column);
    }

    // Iterate over the currently visible items in the column. The downside
    // of doing that, is that the column width will then only be based on the implicit
    // width of the currently loaded items (which can be different depending on which
    // row you're at when the column is flicked in). The upshot is that you don't have to
    // bother setting columnWidthProvider for small tables, or if the implicit width doesn't vary.
    qreal columnWidth = sizeHintForColumn(column);

    if (qIsNaN(columnWidth) || columnWidth <= 0) {
        if (!layoutWarningIssued) {
            layoutWarningIssued = true;
            qmlWarning(q_func()) << "the delegate's implicitWidth needs to be greater than zero";
        }
        columnWidth = kDefaultColumnWidth;
    }

    return columnWidth;
}

qreal QQuickTableViewPrivate::getRowLayoutHeight(int row)
{
    // Return the row height specified by the application, or go
    // through the loaded items and calculate it as a fallback. For
    // layouting, the height can never be zero (or negative), as this
    // can lead us to be stuck in an infinite loop trying to load and
    // fill out the empty viewport space with empty rows.
    const qreal explicitRowHeight = getRowHeight(row);
    if (explicitRowHeight >= 0)
        return explicitRowHeight;

    if (syncVertically) {
        if (syncView->d_func()->loadedRows.contains(row))
            return syncView->d_func()->getRowLayoutHeight(row);
    }

    // Iterate over the currently visible items in the row. The downside
    // of doing that, is that the row height will then only be based on the implicit
    // height of the currently loaded items (which can be different depending on which
    // column you're at when the row is flicked in). The upshot is that you don't have to
    // bother setting rowHeightProvider for small tables, or if the implicit height doesn't vary.
    qreal rowHeight = sizeHintForRow(row);

    if (qIsNaN(rowHeight) || rowHeight <= 0) {
        if (!layoutWarningIssued) {
            layoutWarningIssued = true;
            qmlWarning(q_func()) << "the delegate's implicitHeight needs to be greater than zero";
        }
        rowHeight = kDefaultRowHeight;
    }

    return rowHeight;
}

qreal QQuickTableViewPrivate::getColumnWidth(int column)
{
    // Return the width of the given column, if explicitly set. Return 0 if the column
    // is hidden, and -1 if the width is not set (which means that the width should
    // instead be calculated from the implicit size of the delegate items. This function
    // can be overridden by e.g HeaderView to provide the column widths by other means.
    const int noExplicitColumnWidth = -1;

    if (cachedColumnWidth.startIndex == column)
        return cachedColumnWidth.size;

    if (syncHorizontally)
        return syncView->d_func()->getColumnWidth(column);

    auto cw = columnWidths.size(column);
    if (cw >= 0)
        return cw;

    if (columnWidthProvider.isUndefined())
        return noExplicitColumnWidth;

    qreal columnWidth = noExplicitColumnWidth;

    if (columnWidthProvider.isCallable()) {
        auto const columnAsArgument = QJSValueList() << QJSValue(column);
        columnWidth = columnWidthProvider.call(columnAsArgument).toNumber();
        if (qIsNaN(columnWidth) || columnWidth < 0)
            columnWidth = noExplicitColumnWidth;
    } else {
        if (!layoutWarningIssued) {
            layoutWarningIssued = true;
            qmlWarning(q_func()) << "columnWidthProvider doesn't contain a function";
        }
        columnWidth = noExplicitColumnWidth;
    }

    cachedColumnWidth.startIndex = column;
    cachedColumnWidth.size = columnWidth;
    return columnWidth;
}

qreal QQuickTableViewPrivate::getRowHeight(int row)
{
    // Return the height of the given row, if explicitly set. Return 0 if the row
    // is hidden, and -1 if the height is not set (which means that the height should
    // instead be calculated from the implicit size of the delegate items. This function
    // can be overridden by e.g HeaderView to provide the row heights by other means.
    const int noExplicitRowHeight = -1;

    if (cachedRowHeight.startIndex == row)
        return cachedRowHeight.size;

    if (syncVertically)
        return syncView->d_func()->getRowHeight(row);

    auto rh = rowHeights.size(row);
    if (rh >= 0)
        return rh;

    if (rowHeightProvider.isUndefined())
        return noExplicitRowHeight;

    qreal rowHeight = noExplicitRowHeight;

    if (rowHeightProvider.isCallable()) {
        auto const rowAsArgument = QJSValueList() << QJSValue(row);
        rowHeight = rowHeightProvider.call(rowAsArgument).toNumber();
        if (qIsNaN(rowHeight) || rowHeight < 0)
            rowHeight = noExplicitRowHeight;
    } else {
        if (!layoutWarningIssued) {
            layoutWarningIssued = true;
            qmlWarning(q_func()) << "rowHeightProvider doesn't contain a function";
        }
        rowHeight = noExplicitRowHeight;
    }

    cachedRowHeight.startIndex = row;
    cachedRowHeight.size = rowHeight;
    return rowHeight;
}

bool QQuickTableViewPrivate::isColumnHidden(int column)
{
    // A column is hidden if the width is explicit set to zero (either by
    // using a columnWidthProvider, or by overriding getColumnWidth()).
    return qFuzzyIsNull(getColumnWidth(column));
}

bool QQuickTableViewPrivate::isRowHidden(int row)
{
    // A row is hidden if the height is explicit set to zero (either by
    // using a rowHeightProvider, or by overriding getRowHeight()).
    return qFuzzyIsNull(getRowHeight(row));
}

void QQuickTableViewPrivate::relayoutTableItems()
{
    qCDebug(lcTableViewDelegateLifecycle);

    qreal nextColumnX = loadedTableOuterRect.x();
    qreal nextRowY = loadedTableOuterRect.y();

    for (auto c = loadedColumns.cbegin(); c != loadedColumns.cend(); ++c) {
        const int column = c.key();
        // Adjust the geometry of all cells in the current column
        const qreal width = getColumnLayoutWidth(column);

        for (auto r = loadedRows.cbegin(); r != loadedRows.cend(); ++r) {
            const int row = r.key();
            auto item = loadedTableItem(QPoint(column, row));
            QRectF geometry = item->geometry();
            geometry.moveLeft(nextColumnX);
            geometry.setWidth(width);
            item->setGeometry(geometry);
        }

        if (width > 0)
            nextColumnX += width + cellSpacing.width();
    }

    for (auto r = loadedRows.cbegin(); r != loadedRows.cend(); ++r) {
        const int row = r.key();
        // Adjust the geometry of all cells in the current row
        const qreal height = getRowLayoutHeight(row);

        for (auto c = loadedColumns.cbegin(); c != loadedColumns.cend(); ++c) {
            const int column = c.key();
            auto item = loadedTableItem(QPoint(column, row));
            QRectF geometry = item->geometry();
            geometry.moveTop(nextRowY);
            geometry.setHeight(height);
            item->setGeometry(geometry);
        }

        if (height > 0)
            nextRowY += height + cellSpacing.height();
    }

    if (Q_UNLIKELY(lcTableViewDelegateLifecycle().isDebugEnabled())) {
        for (auto c = loadedColumns.cbegin(); c != loadedColumns.cend(); ++c) {
            const int column = c.key();
            for (auto r = loadedRows.cbegin(); r != loadedRows.cend(); ++r) {
                const int row = r.key();
                QPoint cell = QPoint(column, row);
                qCDebug(lcTableViewDelegateLifecycle()) << "relayout item:" << cell << loadedTableItem(cell)->geometry();
            }
        }
    }
}

void QQuickTableViewPrivate::layoutVerticalEdge(Qt::Edge tableEdge)
{
    int columnThatNeedsLayout;
    int neighbourColumn;
    qreal columnX;
    qreal columnWidth;

    if (tableEdge == Qt::LeftEdge) {
        columnThatNeedsLayout = leftColumn();
        neighbourColumn = loadedColumns.keys().value(1);
        columnWidth = getColumnLayoutWidth(columnThatNeedsLayout);
        const auto neighbourItem = loadedTableItem(QPoint(neighbourColumn, topRow()));
        columnX = neighbourItem->geometry().left() - cellSpacing.width() - columnWidth;
    } else {
        columnThatNeedsLayout = rightColumn();
        neighbourColumn = loadedColumns.keys().value(loadedColumns.count() - 2);
        columnWidth = getColumnLayoutWidth(columnThatNeedsLayout);
        const auto neighbourItem = loadedTableItem(QPoint(neighbourColumn, topRow()));
        columnX = neighbourItem->geometry().right() + cellSpacing.width();
    }

    for (auto r = loadedRows.cbegin(); r != loadedRows.cend(); ++r) {
        const int row = r.key();
        auto fxTableItem = loadedTableItem(QPoint(columnThatNeedsLayout, row));
        auto const neighbourItem = loadedTableItem(QPoint(neighbourColumn, row));
        const qreal rowY = neighbourItem->geometry().y();
        const qreal rowHeight = neighbourItem->geometry().height();

        fxTableItem->setGeometry(QRectF(columnX, rowY, columnWidth, rowHeight));
        fxTableItem->setVisible(true);

        qCDebug(lcTableViewDelegateLifecycle()) << "layout item:" << QPoint(columnThatNeedsLayout, row) << fxTableItem->geometry();
    }
}

void QQuickTableViewPrivate::layoutHorizontalEdge(Qt::Edge tableEdge)
{
    int rowThatNeedsLayout;
    int neighbourRow;

    if (tableEdge == Qt::TopEdge) {
        rowThatNeedsLayout = topRow();
        neighbourRow = loadedRows.keys().value(1);
    } else {
        rowThatNeedsLayout = bottomRow();
        neighbourRow = loadedRows.keys().value(loadedRows.count() - 2);
    }

    // Set the width first, since text items in QtQuick will calculate
    // implicitHeight based on the text items width.
    for (auto c = loadedColumns.cbegin(); c != loadedColumns.cend(); ++c) {
        const int column = c.key();
        auto fxTableItem = loadedTableItem(QPoint(column, rowThatNeedsLayout));
        auto const neighbourItem = loadedTableItem(QPoint(column, neighbourRow));
        const qreal columnX = neighbourItem->geometry().x();
        const qreal columnWidth = neighbourItem->geometry().width();
        fxTableItem->item->setX(columnX);
        fxTableItem->item->setWidth(columnWidth);
    }

    qreal rowY;
    qreal rowHeight;
    if (tableEdge == Qt::TopEdge) {
        rowHeight = getRowLayoutHeight(rowThatNeedsLayout);
        const auto neighbourItem = loadedTableItem(QPoint(leftColumn(), neighbourRow));
        rowY = neighbourItem->geometry().top() - cellSpacing.height() - rowHeight;
    } else {
        rowHeight = getRowLayoutHeight(rowThatNeedsLayout);
        const auto neighbourItem = loadedTableItem(QPoint(leftColumn(), neighbourRow));
        rowY = neighbourItem->geometry().bottom() + cellSpacing.height();
    }

    for (auto c = loadedColumns.cbegin(); c != loadedColumns.cend(); ++c) {
        const int column = c.key();
        auto fxTableItem = loadedTableItem(QPoint(column, rowThatNeedsLayout));
        fxTableItem->item->setY(rowY);
        fxTableItem->item->setHeight(rowHeight);
        fxTableItem->setVisible(true);

        qCDebug(lcTableViewDelegateLifecycle()) << "layout item:" << QPoint(column, rowThatNeedsLayout) << fxTableItem->geometry();
    }
}

void QQuickTableViewPrivate::layoutTopLeftItem()
{
    const QPoint cell(loadRequest.column(), loadRequest.row());
    auto topLeftItem = loadedTableItem(cell);
    auto item = topLeftItem->item;

    item->setPosition(loadRequest.startPosition());
    item->setSize(QSizeF(getColumnLayoutWidth(cell.x()), getRowLayoutHeight(cell.y())));
    topLeftItem->setVisible(true);
    qCDebug(lcTableViewDelegateLifecycle) << "geometry:" << topLeftItem->geometry();
}

void QQuickTableViewPrivate::layoutTableEdgeFromLoadRequest()
{
    if (loadRequest.edge() == Qt::Edge(0)) {
        // No edge means we're loading the top-left item
        layoutTopLeftItem();
        return;
    }

    switch (loadRequest.edge()) {
    case Qt::LeftEdge:
    case Qt::RightEdge:
        layoutVerticalEdge(loadRequest.edge());
        break;
    case Qt::TopEdge:
    case Qt::BottomEdge:
        layoutHorizontalEdge(loadRequest.edge());
        break;
    }
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
    layoutTableEdgeFromLoadRequest();
    syncLoadedTableRectFromLoadedTable();

    if (rebuildState == RebuildState::Done) {
        // Loading of this edge was not done as a part of a rebuild, but
        // instead as an incremental build after e.g a flick.
        updateExtents();
        drainReusePoolAfterLoadRequest();
    }

    loadRequest.markAsDone();

    qCDebug(lcTableViewDelegateLifecycle()) << "request completed! Table:" << tableLayoutToString();
}

void QQuickTableViewPrivate::processRebuildTable()
{
    Q_Q(QQuickTableView);

    if (rebuildState == RebuildState::Begin) {
        if (Q_UNLIKELY(lcTableViewDelegateLifecycle().isDebugEnabled())) {
            qCDebug(lcTableViewDelegateLifecycle()) << "begin rebuild:" << q;
            if (rebuildOptions & RebuildOption::All)
                qCDebug(lcTableViewDelegateLifecycle()) << "RebuildOption::All, options:" << rebuildOptions;
            else if (rebuildOptions & RebuildOption::ViewportOnly)
                qCDebug(lcTableViewDelegateLifecycle()) << "RebuildOption::ViewportOnly, options:" << rebuildOptions;
            else if (rebuildOptions & RebuildOption::LayoutOnly)
                qCDebug(lcTableViewDelegateLifecycle()) << "RebuildOption::LayoutOnly, options:" << rebuildOptions;
            else
                Q_TABLEVIEW_UNREACHABLE(rebuildOptions);
        }
    }

    moveToNextRebuildState();

    if (rebuildState == RebuildState::LoadInitalTable) {
        beginRebuildTable();
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::VerifyTable) {
        if (loadedItems.isEmpty()) {
            qCDebug(lcTableViewDelegateLifecycle()) << "no items loaded!";
            updateContentWidth();
            updateContentHeight();
            rebuildState = RebuildState::Done;
        } else if (!moveToNextRebuildState()) {
            return;
        }
    }

    if (rebuildState == RebuildState::LayoutTable) {
        layoutAfterLoadingInitialTable();
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::LoadAndUnloadAfterLayout) {
        loadAndUnloadVisibleEdges();
        if (!moveToNextRebuildState())
            return;
    }

    const bool preload = (rebuildOptions & RebuildOption::All
                          && reusableFlag == QQmlTableInstanceModel::Reusable);

    if (rebuildState == RebuildState::PreloadColumns) {
        if (preload && nextVisibleEdgeIndexAroundLoadedTable(Qt::RightEdge) != kEdgeIndexAtEnd)
            loadEdge(Qt::RightEdge, QQmlIncubator::AsynchronousIfNested);
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::PreloadRows) {
        if (preload && nextVisibleEdgeIndexAroundLoadedTable(Qt::BottomEdge) != kEdgeIndexAtEnd)
            loadEdge(Qt::BottomEdge, QQmlIncubator::AsynchronousIfNested);
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::MovePreloadedItemsToPool) {
        while (Qt::Edge edge = nextEdgeToUnload(viewportRect))
            unloadEdge(edge);
        if (!moveToNextRebuildState())
            return;
    }

    Q_TABLEVIEW_ASSERT(rebuildState == RebuildState::Done, int(rebuildState));
    qCDebug(lcTableViewDelegateLifecycle()) << "rebuild complete:" << q;
}

bool QQuickTableViewPrivate::moveToNextRebuildState()
{
    if (loadRequest.isActive()) {
        // Items are still loading async, which means
        // that the current state is not yet done.
        return false;
    }

    if (rebuildState == RebuildState::Begin
            && rebuildOptions.testFlag(RebuildOption::LayoutOnly))
        rebuildState = RebuildState::LayoutTable;
    else
        rebuildState = RebuildState(int(rebuildState) + 1);

    qCDebug(lcTableViewDelegateLifecycle()) << int(rebuildState);
    return true;
}

void QQuickTableViewPrivate::calculateTopLeft(QPoint &topLeftCell, QPointF &topLeftPos)
{
    if (tableSize.isEmpty()) {
        // There is no cell that can be top left
        topLeftCell.rx() = kEdgeIndexAtEnd;
        topLeftCell.ry() = kEdgeIndexAtEnd;
        return;
    }

    if (syncHorizontally || syncVertically) {
        const auto syncView_d = syncView->d_func();

        if (syncView_d->loadedItems.isEmpty()) {
            // The sync view contains no loaded items. This probably means
            // that it has not been rebuilt yet. Which also means that
            // we cannot rebuild anything before this happens.
            topLeftCell.rx() = kEdgeIndexNotSet;
            topLeftCell.ry() = kEdgeIndexNotSet;
            return;
        }

        // Get sync view top left, and use that as our own top left (if possible)
        const QPoint syncViewTopLeftCell(syncView_d->leftColumn(), syncView_d->topRow());
        const auto syncViewTopLeftFxItem = syncView_d->loadedTableItem(syncViewTopLeftCell);
        const QPointF syncViewTopLeftPos = syncViewTopLeftFxItem->geometry().topLeft();

        if (syncHorizontally) {
            topLeftCell.rx() = syncViewTopLeftCell.x();
            topLeftPos.rx() = syncViewTopLeftPos.x();

            if (topLeftCell.x() >= tableSize.width()) {
                // Top left is outside our own model.
                topLeftCell.rx() = kEdgeIndexAtEnd;
                topLeftPos.rx() = kEdgeIndexAtEnd;
            }
        }

        if (syncVertically) {
            topLeftCell.ry() = syncViewTopLeftCell.y();
            topLeftPos.ry() = syncViewTopLeftPos.y();

            if (topLeftCell.y() >= tableSize.height()) {
                // Top left is outside our own model.
                topLeftCell.ry() = kEdgeIndexAtEnd;
                topLeftPos.ry() = kEdgeIndexAtEnd;
            }
        }

        if (syncHorizontally && syncVertically) {
            // We have a valid top left, so we're done
            return;
        }
    }

    // Since we're not sync-ing both horizontal and vertical, calculate the missing
    // dimention(s) ourself. If we rebuild all, we find the first visible top-left
    // item starting from cell(0, 0). Otherwise, guesstimate which row or column that
    // should be the new top-left given the geometry of the viewport.

    if (!syncHorizontally) {
        if (rebuildOptions & RebuildOption::All) {
            // Find the first visible column from the beginning
            topLeftCell.rx() = nextVisibleEdgeIndex(Qt::RightEdge, 0);
            if (topLeftCell.x() == kEdgeIndexAtEnd) {
                // No visible column found
                return;
            }
        } else if (rebuildOptions & RebuildOption::CalculateNewTopLeftColumn) {
            // Guesstimate new top left
            const int newColumn = int(viewportRect.x() / (averageEdgeSize.width() + cellSpacing.width()));
            topLeftCell.rx() = qBound(0, newColumn, tableSize.width() - 1);
            topLeftPos.rx() = topLeftCell.x() * (averageEdgeSize.width() + cellSpacing.width());
        } else {
            // Keep the current top left, unless it's outside model
            topLeftCell.rx() = qBound(0, leftColumn(), tableSize.width() - 1);
            topLeftPos.rx() = loadedTableOuterRect.topLeft().x();
        }
    }

    if (!syncVertically) {
        if (rebuildOptions & RebuildOption::All) {
            // Find the first visible row from the beginning
            topLeftCell.ry() = nextVisibleEdgeIndex(Qt::BottomEdge, 0);
            if (topLeftCell.y() == kEdgeIndexAtEnd) {
                // No visible row found
                return;
            }
        } else if (rebuildOptions & RebuildOption::CalculateNewTopLeftRow) {
            // Guesstimate new top left
            const int newRow = int(viewportRect.y() / (averageEdgeSize.height() + cellSpacing.height()));
            topLeftCell.ry() = qBound(0, newRow, tableSize.height() - 1);
            topLeftPos.ry() = topLeftCell.y() * (averageEdgeSize.height() + cellSpacing.height());
        } else {
            // Keep the current top left, unless it's outside model
            topLeftCell.ry() = qBound(0, topRow(), tableSize.height() - 1);
            topLeftPos.ry() = loadedTableOuterRect.topLeft().y();
        }
    }
}

void QQuickTableViewPrivate::beginRebuildTable()
{
    updateTableSize();

    QPoint topLeft;
    QPointF topLeftPos;
    calculateTopLeft(topLeft, topLeftPos);

    if (!loadedItems.isEmpty()) {
        if (rebuildOptions & RebuildOption::All)
            releaseLoadedItems(QQmlTableInstanceModel::NotReusable);
        else if (rebuildOptions & RebuildOption::ViewportOnly)
            releaseLoadedItems(reusableFlag);
    }

    if (rebuildOptions & RebuildOption::All) {
        origin = QPointF(0, 0);
        endExtent = QSizeF(0, 0);
        hData.markExtentsDirty();
        vData.markExtentsDirty();
        updateBeginningEnd();
    }

    loadedColumns.clear();
    loadedRows.clear();
    loadedTableOuterRect = QRect();
    loadedTableInnerRect = QRect();
    clearEdgeSizeCache();

    if (syncHorizontally) {
        setLocalViewportX(syncView->contentX());
        viewportRect.moveLeft(syncView->d_func()->viewportRect.left());
    }

    if (syncVertically) {
        setLocalViewportY(syncView->contentY());
        viewportRect.moveTop(syncView->d_func()->viewportRect.top());
    }

    syncViewportRect();

    if (!model) {
        qCDebug(lcTableViewDelegateLifecycle()) << "no model found, leaving table empty";
        return;
    }

    if (model->count() == 0) {
        qCDebug(lcTableViewDelegateLifecycle()) << "empty model found, leaving table empty";
        return;
    }

    if (tableModel && !tableModel->delegate()) {
        qCDebug(lcTableViewDelegateLifecycle()) << "no delegate found, leaving table empty";
        return;
    }

    if (topLeft.x() == kEdgeIndexAtEnd || topLeft.y() == kEdgeIndexAtEnd) {
        qCDebug(lcTableViewDelegateLifecycle()) << "no visible row or column found, leaving table empty";
        return;
    }

    if (topLeft.x() == kEdgeIndexNotSet || topLeft.y() == kEdgeIndexNotSet) {
        qCDebug(lcTableViewDelegateLifecycle()) << "could not resolve top-left item, leaving table empty";
        return;
    }

    // Load top-left item. After loaded, loadItemsInsideRect() will take
    // care of filling out the rest of the table.
    loadRequest.begin(topLeft, topLeftPos, QQmlIncubator::AsynchronousIfNested);
    processLoadRequest();
    loadAndUnloadVisibleEdges();
}

void QQuickTableViewPrivate::layoutAfterLoadingInitialTable()
{
    clearEdgeSizeCache();
    relayoutTableItems();
    syncLoadedTableRectFromLoadedTable();

    if (rebuildOptions.testFlag(RebuildOption::CalculateNewContentWidth)) {
        updateAverageColumnWidth();
        updateContentWidth();
    }

    if (rebuildOptions.testFlag(RebuildOption::CalculateNewContentHeight)) {
        updateAverageRowHeight();
        updateContentHeight();
    }

    updateExtents();
}

void QQuickTableViewPrivate::unloadEdge(Qt::Edge edge)
{
    qCDebug(lcTableViewDelegateLifecycle) << edge;

    switch (edge) {
    case Qt::LeftEdge:
    case Qt::RightEdge: {
        const int column = edge == Qt::LeftEdge ? leftColumn() : rightColumn();
        for (auto r = loadedRows.cbegin(); r != loadedRows.cend(); ++r)
            unloadItem(QPoint(column, r.key()));
        loadedColumns.remove(column);
        syncLoadedTableRectFromLoadedTable();
        break; }
    case Qt::TopEdge:
    case Qt::BottomEdge: {
        const int row = edge == Qt::TopEdge ? topRow() : bottomRow();
        for (auto c = loadedColumns.cbegin(); c != loadedColumns.cend(); ++c)
            unloadItem(QPoint(c.key(), row));
        loadedRows.remove(row);
        syncLoadedTableRectFromLoadedTable();
        break; }
    }

    qCDebug(lcTableViewDelegateLifecycle) << tableLayoutToString();
}

void QQuickTableViewPrivate::loadEdge(Qt::Edge edge, QQmlIncubator::IncubationMode incubationMode)
{
    const int edgeIndex = nextVisibleEdgeIndexAroundLoadedTable(edge);
    qCDebug(lcTableViewDelegateLifecycle) << edge << edgeIndex;

    const QList<int> visibleCells = edge & (Qt::LeftEdge | Qt::RightEdge)
            ? loadedRows.keys() : loadedColumns.keys();
    loadRequest.begin(edge, edgeIndex, visibleCells, incubationMode);
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

    bool tableModified;

    do {
        tableModified = false;

        if (Qt::Edge edge = nextEdgeToUnload(viewportRect)) {
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

void QQuickTableViewPrivate::drainReusePoolAfterLoadRequest()
{
    Q_Q(QQuickTableView);

    if (reusableFlag == QQmlTableInstanceModel::NotReusable || !tableModel)
        return;

    if (!qFuzzyIsNull(q->verticalOvershoot()) || !qFuzzyIsNull(q->horizontalOvershoot())) {
        // Don't drain while we're overshooting, since this will fill up the
        // pool, but we expect to reuse them all once the content item moves back.
        return;
    }

    // When loading edges, we don't want to drain the reuse pool too aggressively. Normally,
    // all the items in the pool are reused rapidly as the content view is flicked around
    // anyway. Even if the table is temporarily flicked to a section that contains fewer
    // cells than what used to be (e.g if the flicked-in rows are taller than average), it
    // still makes sense to keep all the items in circulation; Chances are, that soon enough,
    // thinner rows are flicked back in again (meaning that we can fit more items into the
    // view). But at the same time, if a delegate chooser is in use, the pool might contain
    // items created from different delegates. And some of those delegates might be used only
    // occasionally. So to avoid situations where an item ends up in the pool for too long, we
    // call drain after each load request, but with a sufficiently large pool time. (If an item
    // in the pool has a large pool time, it means that it hasn't been reused for an equal
    // amount of load cycles, and should be released).
    //
    // We calculate an appropriate pool time by figuring out what the minimum time must be to
    // not disturb frequently reused items. Since the number of items in a row might be higher
    // than in a column (or vice versa), the minimum pool time should take into account that
    // you might be flicking out a single row (filling up the pool), before you continue
    // flicking in several new columns (taking them out again, but now in smaller chunks). This
    // will increase the number of load cycles items are kept in the pool (poolTime), but still,
    // we shouldn't release them, as they are still being reused frequently.
    // To get a flexible maxValue (that e.g tolerates rows and columns being flicked
    // in with varying sizes, causing some items not to be resued immediately), we multiply the
    // value by 2. Note that we also add an extra +1 to the column count, because the number of
    // visible columns will fluctuate between +1/-1 while flicking.
    const int w = loadedColumns.count();
    const int h = loadedRows.count();
    const int minTime = int(std::ceil(w > h ? qreal(w + 1) / h : qreal(h + 1) / w));
    const int maxTime = minTime * 2;
    tableModel->drainReusableItemsPool(maxTime);
}

void QQuickTableViewPrivate::scheduleRebuildTable(RebuildOptions options) {
    if (!q_func()->isComponentComplete()) {
        // We'll rebuild the table once complete anyway
        return;
    }

    scheduledRebuildOptions |= options;
    q_func()->polish();
}

QQuickTableView *QQuickTableViewPrivate::rootSyncView() const
{
    QQuickTableView *root = const_cast<QQuickTableView *>(q_func());
    while (QQuickTableView *view = root->d_func()->syncView)
        root = view;
    return root;
}

void QQuickTableViewPrivate::updatePolish()
{
    // We always start updating from the top of the syncView tree, since
    // the layout of a syncView child will depend on the layout of the syncView.
    //  E.g when a new column is flicked in, the syncView should load and layout
    // the column first, before any syncChildren gets a chance to do the same.
    Q_TABLEVIEW_ASSERT(!polishing, "recursive updatePolish() calls are not allowed!");
    rootSyncView()->d_func()->updateTableRecursive();
}

bool QQuickTableViewPrivate::updateTableRecursive()
{
    if (polishing) {
        // We're already updating the Table in this view, so
        // we cannot continue. Signal this back by returning false.
        // The caller can then choose to call "polish()" instead, to
        // do the update later.
        return false;
    }

    const bool updateComplete = updateTable();
    if (!updateComplete)
        return false;

    for (auto syncChild : qAsConst(syncChildren)) {
        auto syncChild_d = syncChild->d_func();
        syncChild_d->scheduledRebuildOptions |= rebuildOptions;

        const bool descendantUpdateComplete = syncChild_d->updateTableRecursive();
        if (!descendantUpdateComplete)
            return false;
    }

    rebuildOptions = RebuildOption::None;

    return true;
}

bool QQuickTableViewPrivate::updateTable()
{
    // Whenever something changes, e.g viewport moves, spacing is set to a
    // new value, model changes etc, this function will end up being called. Here
    // we check what needs to be done, and load/unload cells accordingly.
    // If we cannot complete the update (because we need to wait for an item
    // to load async), we return false.

    Q_TABLEVIEW_ASSERT(!polishing, "recursive updatePolish() calls are not allowed!");
    QBoolBlocker polishGuard(polishing, true);

    if (loadRequest.isActive()) {
        // We're currently loading items async to build a new edge in the table. We see the loading
        // as an atomic operation, which means that we don't continue doing anything else until all
        // items have been received and laid out. Note that updatePolish is then called once more
        // after the loadRequest has completed to handle anything that might have occurred in-between.
        return false;
    }

    if (rebuildState != RebuildState::Done) {
        processRebuildTable();
        return rebuildState == RebuildState::Done;
    }

    syncWithPendingChanges();

    if (rebuildState == RebuildState::Begin) {
        processRebuildTable();
        return rebuildState == RebuildState::Done;
    }

    if (loadedItems.isEmpty())
        return !loadRequest.isActive();

    loadAndUnloadVisibleEdges();

    return !loadRequest.isActive();
}

void QQuickTableViewPrivate::fixup(QQuickFlickablePrivate::AxisData &data, qreal minExtent, qreal maxExtent)
{
    if (inUpdateContentSize) {
        // We update the content size dynamically as we load and unload edges.
        // Unfortunately, this also triggers a call to this function. The base
        // implementation will do things like start a momentum animation or move
        // the content view somewhere else, which causes glitches. This can
        // especially happen if flicking on one of the syncView children, which triggers
        // an update to our content size. In that case, the base implementation don't know
        // that the view is being indirectly dragged, and will therefore do strange things as
        // it tries to 'fixup' the geometry. So we use a guard to prevent this from happening.
        return;
    }

    QQuickFlickablePrivate::fixup(data, minExtent, maxExtent);
}

int QQuickTableViewPrivate::resolveImportVersion()
{
    const auto data = QQmlData::get(q_func());
    if (!data || !data->propertyCache)
        return 0;

    const auto cppMetaObject = data->propertyCache->firstCppMetaObject();
    const auto qmlTypeView = QQmlMetaType::qmlType(cppMetaObject);
    return qmlTypeView.minorVersion();
}

void QQuickTableViewPrivate::createWrapperModel()
{
    Q_Q(QQuickTableView);
    // When the assigned model is not an instance model, we create a wrapper
    // model (QQmlTableInstanceModel) that keeps a pointer to both the
    // assigned model and the assigned delegate. This model will give us a
    // common interface to any kind of model (js arrays, QAIM, number etc), and
    // help us create delegate instances.
    tableModel = new QQmlTableInstanceModel(qmlContext(q));
    tableModel->useImportVersion(resolveImportVersion());
    model = tableModel;
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
    Q_Q(QQuickTableView);

    if (auto item = qmlobject_cast<QQuickItem*>(object)) {
        item->setParentItem(q->contentItem());
        item->setZ(1);
    }

    if (auto attached = getAttachedObject(object))
        attached->setView(q);
}

void QQuickTableViewPrivate::itemPooledCallback(int modelIndex, QObject *object)
{
    Q_UNUSED(modelIndex);

    if (auto attached = getAttachedObject(object))
        emit attached->pooled();
}

void QQuickTableViewPrivate::itemReusedCallback(int modelIndex, QObject *object)
{
    Q_UNUSED(modelIndex);

    if (auto attached = getAttachedObject(object))
        emit attached->reused();
}

void QQuickTableViewPrivate::syncWithPendingChanges()
{
    // The application can change properties like the model or the delegate while
    // we're e.g in the middle of e.g loading a new row. Since this will lead to
    // unpredicted behavior, and possibly a crash, we need to postpone taking
    // such assignments into effect until we're in a state that allows it.

    syncViewportRect();
    syncModel();
    syncDelegate();
    syncSyncView();

    syncRebuildOptions();
}

void QQuickTableViewPrivate::syncRebuildOptions()
{
    if (!scheduledRebuildOptions)
        return;

    rebuildState = RebuildState::Begin;
    rebuildOptions = scheduledRebuildOptions;
    scheduledRebuildOptions = RebuildOption::None;

    if (loadedItems.isEmpty())
        rebuildOptions.setFlag(RebuildOption::All);

    // Some options are exclusive:
    if (rebuildOptions.testFlag(RebuildOption::All)) {
        rebuildOptions.setFlag(RebuildOption::ViewportOnly, false);
        rebuildOptions.setFlag(RebuildOption::LayoutOnly, false);
        rebuildOptions.setFlag(RebuildOption::CalculateNewContentWidth);
        rebuildOptions.setFlag(RebuildOption::CalculateNewContentHeight);
    } else if (rebuildOptions.testFlag(RebuildOption::ViewportOnly)) {
        rebuildOptions.setFlag(RebuildOption::LayoutOnly, false);
    }
}

void QQuickTableViewPrivate::syncDelegate()
{
    if (!tableModel) {
        // Only the tableModel uses the delegate assigned to a
        // TableView. DelegateModel has it's own delegate, and
        // ObjectModel etc. doesn't use one.
        return;
    }

    if (assignedDelegate != tableModel->delegate())
        tableModel->setDelegate(assignedDelegate);
}

QVariant QQuickTableViewPrivate::modelImpl() const
{
    return assignedModel;
}

void QQuickTableViewPrivate::setModelImpl(const QVariant &newModel)
{
    if (newModel == assignedModel)
        return;

    assignedModel = newModel;
    scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::All);
    emit q_func()->modelChanged();
}

void QQuickTableViewPrivate::syncModel()
{
    if (modelVariant == assignedModel)
        return;

    if (model) {
        disconnectFromModel();
        releaseLoadedItems(QQmlTableInstanceModel::NotReusable);
    }

    modelVariant = assignedModel;
    QVariant effectiveModelVariant = modelVariant;
    if (effectiveModelVariant.userType() == qMetaTypeId<QJSValue>())
        effectiveModelVariant = effectiveModelVariant.value<QJSValue>().toVariant();

    const auto instanceModel = qobject_cast<QQmlInstanceModel *>(qvariant_cast<QObject*>(effectiveModelVariant));

    if (instanceModel) {
        if (tableModel) {
            delete tableModel;
            tableModel = nullptr;
        }
        model = instanceModel;
    } else {
        if (!tableModel)
            createWrapperModel();
        tableModel->setModel(effectiveModelVariant);
    }

    connectToModel();
}

void QQuickTableViewPrivate::syncSyncView()
{
    Q_Q(QQuickTableView);

    if (assignedSyncView != syncView) {
        if (syncView)
            syncView->d_func()->syncChildren.removeOne(q);

        if (assignedSyncView) {
            QQuickTableView *view = assignedSyncView;

            while (view) {
                if (view == q) {
                    if (!layoutWarningIssued) {
                        layoutWarningIssued = true;
                        qmlWarning(q) << "TableView: recursive syncView connection detected!";
                    }
                    syncView = nullptr;
                    return;
                }
                view = view->d_func()->syncView;
            }

            assignedSyncView->d_func()->syncChildren.append(q);
            scheduledRebuildOptions |= RebuildOption::ViewportOnly;
        }

        syncView = assignedSyncView;
    }

    syncHorizontally = syncView && assignedSyncDirection & Qt::Horizontal;
    syncVertically = syncView && assignedSyncDirection & Qt::Vertical;

    if (syncHorizontally) {
        q->setColumnSpacing(syncView->columnSpacing());
        updateContentWidth();
    }

    if (syncVertically) {
        q->setRowSpacing(syncView->rowSpacing());
        updateContentHeight();
    }

    if (syncView && loadedItems.isEmpty() && !tableSize.isEmpty()) {
        // When we have a syncView, we can sometimes temporarily end up with no loaded items.
        // This can happen if the syncView has a model with more rows or columns than us, in
        // which case the viewport can end up in a place where we have no rows or columns to
        // show. In that case, check now if the viewport has been flicked back again, and
        // that we can rebuild the table with a visible top-left cell.
        const auto syncView_d = syncView->d_func();
        if (!syncView_d->loadedItems.isEmpty()) {
            if (syncHorizontally && syncView_d->leftColumn() <= tableSize.width() - 1)
                scheduledRebuildOptions |= QQuickTableViewPrivate::RebuildOption::ViewportOnly;
            else if (syncVertically && syncView_d->topRow() <= tableSize.height() - 1)
                scheduledRebuildOptions |= QQuickTableViewPrivate::RebuildOption::ViewportOnly;
        }
    }
}

void QQuickTableViewPrivate::connectToModel()
{
    Q_Q(QQuickTableView);
    Q_TABLEVIEW_ASSERT(model, "");

    QObjectPrivate::connect(model, &QQmlInstanceModel::createdItem, this, &QQuickTableViewPrivate::itemCreatedCallback);
    QObjectPrivate::connect(model, &QQmlInstanceModel::initItem, this, &QQuickTableViewPrivate::initItemCallback);
    QObjectPrivate::connect(model, &QQmlTableInstanceModel::itemPooled, this, &QQuickTableViewPrivate::itemPooledCallback);
    QObjectPrivate::connect(model, &QQmlTableInstanceModel::itemReused, this, &QQuickTableViewPrivate::itemReusedCallback);

    // Connect atYEndChanged to a function that fetches data if more is available
    QObjectPrivate::connect(q, &QQuickTableView::atYEndChanged, this, &QQuickTableViewPrivate::fetchMoreData);

    if (auto const aim = model->abstractItemModel()) {
        // When the model exposes a QAIM, we connect to it directly. This means that if the current model is
        // a QQmlDelegateModel, we just ignore all the change sets it emits. In most cases, the model will instead
        // be our own QQmlTableInstanceModel, which doesn't bother creating change sets at all. For models that are
        // not based on QAIM (like QQmlObjectModel, QQmlListModel, javascript arrays etc), there is currently no way
        // to modify the model at runtime without also re-setting the model on the view.
        connect(aim, &QAbstractItemModel::rowsMoved, this, &QQuickTableViewPrivate::rowsMovedCallback);
        connect(aim, &QAbstractItemModel::columnsMoved, this, &QQuickTableViewPrivate::columnsMovedCallback);
        connect(aim, &QAbstractItemModel::rowsInserted, this, &QQuickTableViewPrivate::rowsInsertedCallback);
        connect(aim, &QAbstractItemModel::rowsRemoved, this, &QQuickTableViewPrivate::rowsRemovedCallback);
        connect(aim, &QAbstractItemModel::columnsInserted, this, &QQuickTableViewPrivate::columnsInsertedCallback);
        connect(aim, &QAbstractItemModel::columnsRemoved, this, &QQuickTableViewPrivate::columnsRemovedCallback);
        connect(aim, &QAbstractItemModel::modelReset, this, &QQuickTableViewPrivate::modelResetCallback);
        connect(aim, &QAbstractItemModel::layoutChanged, this, &QQuickTableViewPrivate::layoutChangedCallback);
    } else {
        QObjectPrivate::connect(model, &QQmlInstanceModel::modelUpdated, this, &QQuickTableViewPrivate::modelUpdated);
    }
}

void QQuickTableViewPrivate::disconnectFromModel()
{
    Q_Q(QQuickTableView);
    Q_TABLEVIEW_ASSERT(model, "");

    QObjectPrivate::disconnect(model, &QQmlInstanceModel::createdItem, this, &QQuickTableViewPrivate::itemCreatedCallback);
    QObjectPrivate::disconnect(model, &QQmlInstanceModel::initItem, this, &QQuickTableViewPrivate::initItemCallback);
    QObjectPrivate::disconnect(model, &QQmlTableInstanceModel::itemPooled, this, &QQuickTableViewPrivate::itemPooledCallback);
    QObjectPrivate::disconnect(model, &QQmlTableInstanceModel::itemReused, this, &QQuickTableViewPrivate::itemReusedCallback);

    QObjectPrivate::disconnect(q, &QQuickTableView::atYEndChanged, this, &QQuickTableViewPrivate::fetchMoreData);

    if (auto const aim = model->abstractItemModel()) {
        disconnect(aim, &QAbstractItemModel::rowsMoved, this, &QQuickTableViewPrivate::rowsMovedCallback);
        disconnect(aim, &QAbstractItemModel::columnsMoved, this, &QQuickTableViewPrivate::columnsMovedCallback);
        disconnect(aim, &QAbstractItemModel::rowsInserted, this, &QQuickTableViewPrivate::rowsInsertedCallback);
        disconnect(aim, &QAbstractItemModel::rowsRemoved, this, &QQuickTableViewPrivate::rowsRemovedCallback);
        disconnect(aim, &QAbstractItemModel::columnsInserted, this, &QQuickTableViewPrivate::columnsInsertedCallback);
        disconnect(aim, &QAbstractItemModel::columnsRemoved, this, &QQuickTableViewPrivate::columnsRemovedCallback);
        disconnect(aim, &QAbstractItemModel::modelReset, this, &QQuickTableViewPrivate::modelResetCallback);
        disconnect(aim, &QAbstractItemModel::layoutChanged, this, &QQuickTableViewPrivate::layoutChangedCallback);
    } else {
        QObjectPrivate::disconnect(model, &QQmlInstanceModel::modelUpdated, this, &QQuickTableViewPrivate::modelUpdated);
    }
}

void QQuickTableViewPrivate::modelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    Q_UNUSED(changeSet);
    Q_UNUSED(reset);

    Q_TABLEVIEW_ASSERT(!model->abstractItemModel(), "");
    scheduleRebuildTable(RebuildOption::ViewportOnly
                         | RebuildOption::CalculateNewContentWidth
                         | RebuildOption::CalculateNewContentHeight);
}

void QQuickTableViewPrivate::rowsMovedCallback(const QModelIndex &parent, int, int, const QModelIndex &, int )
{
    if (parent != QModelIndex())
        return;

    scheduleRebuildTable(RebuildOption::ViewportOnly);
}

void QQuickTableViewPrivate::columnsMovedCallback(const QModelIndex &parent, int, int, const QModelIndex &, int)
{
    if (parent != QModelIndex())
        return;

    scheduleRebuildTable(RebuildOption::ViewportOnly);
}

void QQuickTableViewPrivate::rowsInsertedCallback(const QModelIndex &parent, int, int)
{
    if (parent != QModelIndex())
        return;

    scheduleRebuildTable(RebuildOption::ViewportOnly | RebuildOption::CalculateNewContentHeight);
}

void QQuickTableViewPrivate::rowsRemovedCallback(const QModelIndex &parent, int, int)
{
    if (parent != QModelIndex())
        return;

    scheduleRebuildTable(RebuildOption::ViewportOnly | RebuildOption::CalculateNewContentHeight);
}

void QQuickTableViewPrivate::columnsInsertedCallback(const QModelIndex &parent, int, int)
{
    if (parent != QModelIndex())
        return;

    // Adding a column (or row) can result in the table going from being
    // e.g completely inside the viewport to go outside. And in the latter
    // case, the user needs to be able to scroll the viewport, also if
    // flags such as Flickable.StopAtBounds is in use. So we need to
    // update contentWidth to support that case.
    scheduleRebuildTable(RebuildOption::ViewportOnly | RebuildOption::CalculateNewContentWidth);
}

void QQuickTableViewPrivate::columnsRemovedCallback(const QModelIndex &parent, int, int)
{
    if (parent != QModelIndex())
        return;

    scheduleRebuildTable(RebuildOption::ViewportOnly | RebuildOption::CalculateNewContentWidth);
}

void QQuickTableViewPrivate::layoutChangedCallback(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_UNUSED(parents);
    Q_UNUSED(hint);

    scheduleRebuildTable(RebuildOption::ViewportOnly);
}

void QQuickTableViewPrivate::fetchMoreData()
{
    if (tableModel && tableModel->canFetchMore()) {
        tableModel->fetchMore();
        scheduleRebuildTable(RebuildOption::ViewportOnly);
    }
}

void QQuickTableViewPrivate::modelResetCallback()
{
    scheduleRebuildTable(RebuildOption::All);
}

void QQuickTableViewPrivate::scheduleRebuildIfFastFlick()
{
    Q_Q(QQuickTableView);
    // If the viewport has moved more than one page vertically or horizontally, we switch
    // strategy from refilling edges around the current table to instead rebuild the table
    // from scratch inside the new viewport. This will greatly improve performance when flicking
    // a long distance in one go, which can easily happen when dragging on scrollbars.
    // Note that we don't want to update the content size in this case, since first of all, the
    // content size should logically not change as a result of flicking. But more importantly, updating
    // the content size in combination with fast-flicking has a tendency to cause flicker in the viewport.

    // Check the viewport moved more than one page vertically
    if (!viewportRect.intersects(QRectF(viewportRect.x(), q->contentY(), 1, q->height()))) {
        scheduledRebuildOptions |= RebuildOption::CalculateNewTopLeftRow;
        scheduledRebuildOptions |= RebuildOption::ViewportOnly;
    }

    // Check the viewport moved more than one page horizontally
    if (!viewportRect.intersects(QRectF(q->contentX(), viewportRect.y(), q->width(), 1))) {
        scheduledRebuildOptions |= RebuildOption::CalculateNewTopLeftColumn;
        scheduledRebuildOptions |= RebuildOption::ViewportOnly;
    }
}

void QQuickTableViewPrivate::setLocalViewportX(qreal contentX)
{
    // Set the new viewport position if changed, but don't trigger any
    // rebuilds or updates. We use this function internally to distinguish
    // external flicking from internal sync-ing of the content view.
    Q_Q(QQuickTableView);
    QBoolBlocker blocker(inSetLocalViewportPos, true);

    if (qFuzzyCompare(contentX, q->contentX()))
        return;

    q->setContentX(contentX);
}

void QQuickTableViewPrivate::setLocalViewportY(qreal contentY)
{
    // Set the new viewport position if changed, but don't trigger any
    // rebuilds or updates. We use this function internally to distinguish
    // external flicking from internal sync-ing of the content view.
    Q_Q(QQuickTableView);
    QBoolBlocker blocker(inSetLocalViewportPos, true);

    if (qFuzzyCompare(contentY, q->contentY()))
        return;

    q->setContentY(contentY);
}

void QQuickTableViewPrivate::syncViewportRect()
{
    // Sync viewportRect so that it contains the actual geometry of the viewport
    Q_Q(QQuickTableView);
    viewportRect = QRectF(q->contentX(), q->contentY(), q->width(), q->height());
    qCDebug(lcTableViewDelegateLifecycle) << viewportRect;
}

void QQuickTableViewPrivate::syncViewportPosRecursive()
{
    Q_Q(QQuickTableView);
    QBoolBlocker recursionGuard(inSyncViewportPosRecursive, true);

    if (syncView) {
        auto syncView_d = syncView->d_func();
        if (!syncView_d->inSyncViewportPosRecursive) {
            if (syncHorizontally)
                syncView_d->setLocalViewportX(q->contentX());
            if (syncVertically)
                syncView_d->setLocalViewportY(q->contentY());
            syncView_d->syncViewportPosRecursive();
        }
    }

    for (auto syncChild : qAsConst(syncChildren)) {
        auto syncChild_d = syncChild->d_func();
        if (!syncChild_d->inSyncViewportPosRecursive) {
            if (syncChild_d->syncHorizontally)
                syncChild_d->setLocalViewportX(q->contentX());
            if (syncChild_d->syncVertically)
                syncChild_d->setLocalViewportY(q->contentY());
            syncChild_d->syncViewportPosRecursive();
        }
    }
}

QQuickTableView::QQuickTableView(QQuickItem *parent)
    : QQuickFlickable(*(new QQuickTableViewPrivate), parent)
{
    setFlag(QQuickItem::ItemIsFocusScope);
}

QQuickTableView::~QQuickTableView()
{
}

QQuickTableView::QQuickTableView(QQuickTableViewPrivate &dd, QQuickItem *parent)
    : QQuickFlickable(dd, parent)
{
    setFlag(QQuickItem::ItemIsFocusScope);
}

qreal QQuickTableView::minXExtent() const
{
    return QQuickFlickable::minXExtent() - d_func()->origin.x();
}

qreal QQuickTableView::maxXExtent() const
{
    return QQuickFlickable::maxXExtent() - d_func()->endExtent.width();
}

qreal QQuickTableView::minYExtent() const
{
    return QQuickFlickable::minYExtent() - d_func()->origin.y();
}

qreal QQuickTableView::maxYExtent() const
{
    return QQuickFlickable::maxYExtent() - d_func()->endExtent.height();
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
    if (qt_is_nan(spacing) || !qt_is_finite(spacing))
        return;
    if (qFuzzyCompare(d->cellSpacing.height(), spacing))
        return;

    d->cellSpacing.setHeight(spacing);
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::LayoutOnly
                            | QQuickTableViewPrivate::RebuildOption::CalculateNewContentHeight);
    emit rowSpacingChanged();
}

qreal QQuickTableView::columnSpacing() const
{
    return d_func()->cellSpacing.width();
}

void QQuickTableView::setColumnSpacing(qreal spacing)
{
    Q_D(QQuickTableView);
    if (qt_is_nan(spacing) || !qt_is_finite(spacing))
        return;
    if (qFuzzyCompare(d->cellSpacing.width(), spacing))
        return;

    d->cellSpacing.setWidth(spacing);
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::LayoutOnly
                            | QQuickTableViewPrivate::RebuildOption::CalculateNewContentWidth);
    emit columnSpacingChanged();
}

QJSValue QQuickTableView::rowHeightProvider() const
{
    return d_func()->rowHeightProvider;
}

void QQuickTableView::setRowHeightProvider(const QJSValue &provider)
{
    Q_D(QQuickTableView);
    if (provider.strictlyEquals(d->rowHeightProvider))
        return;

    d->rowHeightProvider = provider;
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly
                            | QQuickTableViewPrivate::RebuildOption::CalculateNewContentHeight);
    emit rowHeightProviderChanged();
}

QJSValue QQuickTableView::columnWidthProvider() const
{
    return d_func()->columnWidthProvider;
}

void QQuickTableView::setColumnWidthProvider(const QJSValue &provider)
{
    Q_D(QQuickTableView);
    if (provider.strictlyEquals(d->columnWidthProvider))
        return;

    d->columnWidthProvider = provider;
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly
                            | QQuickTableViewPrivate::RebuildOption::CalculateNewContentWidth);
    emit columnWidthProviderChanged();
}

QVariant QQuickTableView::model() const
{
    return d_func()->modelImpl();
}

void QQuickTableView::setModel(const QVariant &newModel)
{
    return d_func()->setModelImpl(newModel);
}

QQmlComponent *QQuickTableView::delegate() const
{
    return d_func()->assignedDelegate;
}

void QQuickTableView::setDelegate(QQmlComponent *newDelegate)
{
    Q_D(QQuickTableView);
    if (newDelegate == d->assignedDelegate)
        return;

    d->assignedDelegate = newDelegate;
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::All);

    emit delegateChanged();
}

bool QQuickTableView::reuseItems() const
{
    return bool(d_func()->reusableFlag == QQmlTableInstanceModel::Reusable);
}

void QQuickTableView::setReuseItems(bool reuse)
{
    Q_D(QQuickTableView);
    if (reuseItems() == reuse)
        return;

    d->reusableFlag = reuse ? QQmlTableInstanceModel::Reusable : QQmlTableInstanceModel::NotReusable;

    if (!reuse && d->tableModel) {
        // When we're told to not reuse items, we
        // immediately, as documented, drain the pool.
        d->tableModel->drainReusableItemsPool(0);
    }

    emit reuseItemsChanged();
}

void QQuickTableView::setContentWidth(qreal width)
{
    Q_D(QQuickTableView);
    d->explicitContentWidth = width;
    QQuickFlickable::setContentWidth(width);
}

void QQuickTableView::setContentHeight(qreal height)
{
    Q_D(QQuickTableView);
    d->explicitContentHeight = height;
    QQuickFlickable::setContentHeight(height);
}

/*!
    \qmlproperty TableView QtQuick::TableView::syncView

    If this property of a TableView is set to another TableView, both the
    tables will synchronize with regard to flicking, column widths/row heights,
    and spacing according to \l syncDirection.

    If \l syncDirection contains \l Qt.Horizontal, current tableView's column
    widths, column spacing, and horizontal flicking movement synchronizes with
    syncView's.

    If \l syncDirection contains \l Qt.Vertical, current tableView's row
    heights, row spacing, and vertical flicking movement synchronizes with
    syncView's.

    \sa syncDirection
*/
QQuickTableView *QQuickTableView::syncView() const
{
   return d_func()->assignedSyncView;
}

void QQuickTableView::setSyncView(QQuickTableView *view)
{
    Q_D(QQuickTableView);
    if (d->assignedSyncView == view)
        return;

    d->assignedSyncView = view;
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly);

    emit syncViewChanged();
}

/*!
    \qmlproperty Qt::Orientations QtQuick::TableView::syncDirection

    If the \l syncView is set on a TableView, this property controls
    synchronization of flicking direction(s) for both tables. The default is \c
    {Qt.Horizontal | Qt.Vertical}, which means that if you flick either table
    in either direction, the other table is flicked the same amount in the
    same direction.

    This property and \l syncView can be used to make two tableViews
    synchronize with each other smoothly in flicking regardless of the different
    overshoot/undershoot, velocity, acceleration/deceleration or rebound
    animation, and so on.

    A typical use case is to make several headers flick along with the table.

    \sa syncView, headerView
*/
Qt::Orientations QQuickTableView::syncDirection() const
{
   return d_func()->assignedSyncDirection;
}

void QQuickTableView::setSyncDirection(Qt::Orientations direction)
{
    Q_D(QQuickTableView);
    if (d->assignedSyncDirection == direction)
        return;

    d->assignedSyncDirection = direction;
    if (d->assignedSyncView)
        d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly);

    emit syncDirectionChanged();
}

void QQuickTableView::forceLayout()
{
    d_func()->forceLayout();
}

QQuickTableViewAttached *QQuickTableView::qmlAttachedProperties(QObject *obj)
{
    return new QQuickTableViewAttached(obj);
}

void QQuickTableView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTableView);
    QQuickFlickable::geometryChanged(newGeometry, oldGeometry);

    if (d->tableModel) {
        // When the view changes size, we force the pool to
        // shrink by releasing all pooled items.
        d->tableModel->drainReusableItemsPool(0);
    }

    polish();
}

void QQuickTableView::viewportMoved(Qt::Orientations orientation)
{
    Q_D(QQuickTableView);

    // If the new viewport position was set from the setLocalViewportXY()
    // functions, we just update the position silently and return. Otherwise, if
    // the viewport was flicked by the user, or some other control, we
    // recursively sync all the views in the hierarchy to the same position.
    QQuickFlickable::viewportMoved(orientation);
    if (d->inSetLocalViewportPos)
        return;

    // Move all views in the syncView hierarchy to the same contentX/Y.
    // We need to start from this view (and not the root syncView) to
    // ensure that we respect all the individual syncDirection flags
    // between the individual views in the hierarchy.
    d->syncViewportPosRecursive();

    auto rootView = d->rootSyncView();
    auto rootView_d = rootView->d_func();

    rootView_d->scheduleRebuildIfFastFlick();

    if (!rootView_d->polishScheduled) {
        if (rootView_d->scheduledRebuildOptions) {
            // When we need to rebuild, collecting several viewport
            // moves and do a single polish gives a quicker UI.
            rootView->polish();
        } else {
            // Updating the table right away when flicking
            // slowly gives a smoother experience.
            const bool updated = rootView->d_func()->updateTableRecursive();
            if (!updated) {
                // One, or more, of the views are already in an
                // update, so we need to wait a cycle.
                rootView->polish();
            }
        }
    }
}

void QQuickTableViewPrivate::_q_componentFinalized()
{
    // Now that all bindings are evaluated, and we know
    // our final geometery, we can build the table.
    qCDebug(lcTableViewDelegateLifecycle);
    updatePolish();
}

void QQuickTableViewPrivate::registerCallbackWhenBindingsAreEvaluated()
{
    // componentComplete() is called on us after all static values have been assigned, but
    // before bindings to any anchestors has been evaluated. Especially this means that
    // if our size is bound to the parents size, it will still be empty at that point.
    // And we cannot build the table without knowing our own size. We could wait until we
    // got the first updatePolish() callback, but at that time, any asynchronous loaders that we
    // might be inside have already finished loading, which means that we would load all
    // the delegate items synchronously instead of asynchronously. We therefore add a componentFinalized
    // function that gets called after all the bindings we rely on has been evaluated.
    // When receiving this call, we load the delegate items (and build the table).
    Q_Q(QQuickTableView);
    QQmlEnginePrivate *engPriv = QQmlEnginePrivate::get(qmlEngine(q));
    static int finalizedIdx = -1;
    if (finalizedIdx < 0)
        finalizedIdx = q->metaObject()->indexOfSlot("_q_componentFinalized()");
    engPriv->registerFinalizeCallback(q, finalizedIdx);
}

void QQuickTableView::componentComplete()
{
    QQuickFlickable::componentComplete();
    d_func()->registerCallbackWhenBindingsAreEvaluated();
}

class QObjectPrivate;
class QQuickTableSectionSizeProviderPrivate : public QObjectPrivate {
public:
    QQuickTableSectionSizeProviderPrivate();
    ~QQuickTableSectionSizeProviderPrivate();
    QHash<int, qreal> hash;
};

QQuickTableSectionSizeProvider::QQuickTableSectionSizeProvider(QObject *parent)
    : QObject (*(new QQuickTableSectionSizeProviderPrivate), parent)
{
}

void QQuickTableSectionSizeProvider::setSize(int section, qreal size)
{
    Q_D(QQuickTableSectionSizeProvider);
    if (section < 0 || size < 0) {
        qmlWarning(this) << "setSize: section or size less than zero";
        return;
    }
    if (qFuzzyCompare(QQuickTableSectionSizeProvider::size(section), size))
        return;
    d->hash.insert(section, size);
    emit sizeChanged();
}

// return -1.0 if no valid explicit size retrieved
qreal QQuickTableSectionSizeProvider::size(int section)
{
    Q_D(QQuickTableSectionSizeProvider);
    auto it = d->hash.find(section);
    if (it != d->hash.end())
        return *it;
    return -1.0;
}

// return true if section is valid
bool QQuickTableSectionSizeProvider::resetSize(int section)
{
    Q_D(QQuickTableSectionSizeProvider);
    if (d->hash.empty())
        return false;

    auto ret = d->hash.remove(section);
    if (ret)
        emit sizeChanged();
    return ret;
}

void QQuickTableSectionSizeProvider::resetAll()
{
    Q_D(QQuickTableSectionSizeProvider);
    d->hash.clear();
    emit sizeChanged();
}

QQuickTableSectionSizeProviderPrivate::QQuickTableSectionSizeProviderPrivate()
    : QObjectPrivate()
{
}

QQuickTableSectionSizeProviderPrivate::~QQuickTableSectionSizeProviderPrivate()
{

}
#include "moc_qquicktableview_p.cpp"

QT_END_NAMESPACE
