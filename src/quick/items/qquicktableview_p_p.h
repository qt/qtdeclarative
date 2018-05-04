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

#include <QtCore/qtimer.h>
#include <QtQml/private/qqmldelegatemodel_p.h>
#include <QtQml/private/qqmlincubator_p.h>
#include <QtQml/private/qqmlchangeset_p.h>
#include <QtQml/qqmlinfo.h>

#include <QtQuick/private/qquickflickable_p_p.h>
#include <QtQuick/private/qquickitemviewfxitem_p_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcTableViewDelegateLifecycle)

static const int kDefaultCacheBuffer = 300;

class FxTableItem;

class QQuickTableViewPrivate : public QQuickFlickablePrivate
{
    Q_DECLARE_PUBLIC(QQuickTableView)

public:
    class TableEdgeLoadRequest
    {
        // Whenever we need to load new rows or columns in the
        // table, we fill out a TableEdgeLoadRequest.
        // TableEdgeLoadRequest is just a struct that keeps track
        // of which cells that needs to be loaded, and which cell
        // the table is currently loading. The loading itself is
        // done by QQuickTableView.

    public:
        void begin(const QPoint &cell, QQmlIncubator::IncubationMode incubationMode)
        {
            Q_ASSERT(!active);
            active = true;
            tableEdge = Qt::Edge(0);
            tableCells = QLine(cell, cell);
            mode = incubationMode;
            cellCount = 1;
            currentIndex = 0;
            qCDebug(lcTableViewDelegateLifecycle()) << "begin top-left:" << toString();
        }

        void begin(const QLine cellsToLoad, Qt::Edge edgeToLoad, QQmlIncubator::IncubationMode incubationMode)
        {
            Q_ASSERT(!active);
            active = true;
            tableEdge = edgeToLoad;
            tableCells = cellsToLoad;
            mode = incubationMode;
            cellCount = tableCells.x2() - tableCells.x1() + tableCells.y2() - tableCells.y1() + 1;
            currentIndex = 0;
            qCDebug(lcTableViewDelegateLifecycle()) << "begin:" << toString();
        }

        inline void markAsDone() { active = false; }
        inline bool isActive() { return active; }

        inline QPoint firstCell() { return tableCells.p1(); }
        inline QPoint lastCell() { return tableCells.p2(); }
        inline QPoint currentCell() { return cellAt(currentIndex); }
        inline QPoint previousCell() { return cellAt(currentIndex - 1); }

        inline bool atBeginning() { return currentIndex == 0; }
        inline bool hasCurrentCell() { return currentIndex < cellCount; }
        inline void moveToNextCell() { ++currentIndex; }

        inline Qt::Edge edge() { return tableEdge; }
        inline QQmlIncubator::IncubationMode incubationMode() { return mode; }

        QString toString()
        {
            QString str;
            QDebug dbg(&str);
            dbg.nospace() << "TableSectionLoadRequest(" << "edge:"
                << tableEdge << " cells:" << tableCells << " incubation:";

            switch (mode) {
            case QQmlIncubator::Asynchronous:
                dbg << "Asynchronous";
                break;
            case QQmlIncubator::AsynchronousIfNested:
                dbg << "AsynchronousIfNested";
                break;
            case QQmlIncubator::Synchronous:
                dbg << "Synchronous";
                break;
            }

            return str;
        }

    private:
        Qt::Edge tableEdge = Qt::Edge(0);
        QLine tableCells;
        int currentIndex = 0;
        int cellCount = 0;
        bool active = false;
        QQmlIncubator::IncubationMode mode = QQmlIncubator::AsynchronousIfNested;

        QPoint cellAt(int index)
        {
            int x = tableCells.p1().x() + (tableCells.dx() ? index : 0);
            int y = tableCells.p1().y() + (tableCells.dy() ? index : 0);
            return QPoint(x, y);
        }
    };

    struct ColumnRowSize
    {
        // ColumnRowSize is a helper class for storing row heights
        // and column widths. We calculate the average width of a column
        // the first time it's scrolled into view based on the size of
        // the loaded items in the new column. Since we never load more items
        // that what fits inside the viewport (cachebuffer aside), this calculation
        // would be different depending on which row you were at when the column
        // was scrolling in. To avoid that a column resizes when it's scrolled
        // in and out and in again, we store its width. But to avoid storing
        // the width for all columns, we choose to only store the width if it
        // differs from the column(s) to the left. The same logic applies for row heights.
        // 'index' translates to either 'row' or 'column'.
        int index;
        qreal size;

        static bool lessThan(const ColumnRowSize& a, const ColumnRowSize& b)
        {
            return a.index < b.index;
        }
    };

public:
    QQuickTableViewPrivate();
    ~QQuickTableViewPrivate() override;

    static inline QQuickTableViewPrivate *get(QQuickTableView *q) { return q->d_func(); }

    void updatePolish() override;

public:
    QList<FxTableItem *> loadedItems;

    QQmlInstanceModel* model = nullptr;
    QVariant modelVariant;

    // loadedTable describes the table cells that are currently loaded (from top left
    // row/column to bottom right row/column). loadedTableOuterRect describes the actual
    // pixels that those cells cover, and is matched agains the viewport to determine when
    // we need to fill up with more rows/columns. loadedTableInnerRect describes the pixels
    // that the loaded table covers if you remove one row/column on each side of the table, and
    // is used to determine rows/columns that are no longer visible and can be unloaded.
    QRect loadedTable;
    QRectF loadedTableOuterRect;
    QRectF loadedTableInnerRect;

    QRectF viewportRect = QRectF(0, 0, -1, -1);

    QSize tableSize;

    TableEdgeLoadRequest loadRequest;

    QPoint contentSizeBenchMarkPoint = QPoint(-1, -1);
    QSizeF cellSpacing;

    int cacheBuffer = kDefaultCacheBuffer;
    QTimer cacheBufferDelayTimer;
    bool hasBufferedItems = false;

    bool blockItemCreatedCallback = false;
    bool tableInvalid = false;
    bool tableRebuilding = false;
    bool columnRowPositionsInvalid = false;

    QVector<ColumnRowSize> columnWidths;
    QVector<ColumnRowSize> rowHeights;

    const static QPoint kLeft;
    const static QPoint kRight;
    const static QPoint kUp;
    const static QPoint kDown;

#ifdef QT_DEBUG
    QString forcedIncubationMode = qEnvironmentVariable("QT_TABLEVIEW_INCUBATION_MODE");
#endif

public:
    inline QQmlDelegateModel *wrapperModel() const { return qobject_cast<QQmlDelegateModel*>(model); }
    QQuickTableViewAttached *getAttachedObject(const QObject *object) const;

    int modelIndexAtCell(const QPoint &cell);
    QPoint cellAtModelIndex(int modelIndex);

    void calculateColumnWidthsAfterRebuilding();
    void calculateRowHeightsAfterRebuilding();
    void calculateColumnWidth(int column);
    void calculateRowHeight(int row);
    void calculateEdgeSizeFromLoadRequest();
    void calculateTableSize();

    qreal columnWidth(int column);
    qreal rowHeight(int row);

    void relayoutTable();
    void relayoutTableItems();

    void layoutVerticalEdge(Qt::Edge tableEdge, bool adjustSize);
    void layoutHorizontalEdge(Qt::Edge tableEdge, bool adjustSize);
    void layoutTableEdgeFromLoadRequest();

    void updateContentWidth();
    void updateContentHeight();

    void enforceFirstRowColumnAtOrigo();
    void syncLoadedTableRectFromLoadedTable();
    void syncLoadedTableFromLoadRequest();

    bool canLoadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const;
    bool canUnloadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const;
    Qt::Edge nextEdgeToLoad(const QRectF rect);
    Qt::Edge nextEdgeToUnload(const QRectF rect);

    FxTableItem *loadedTableItem(const QPoint &cell) const;
    FxTableItem *itemNextTo(const FxTableItem *fxTableItem, const QPoint &direction) const;
    FxTableItem *createFxTableItem(const QPoint &cell, QQmlIncubator::IncubationMode incubationMode);
    FxTableItem *loadFxTableItem(const QPoint &cell, QQmlIncubator::IncubationMode incubationMode);

    void releaseItem(FxTableItem *fxTableItem);
    void releaseLoadedItems();
    void clear();

    void unloadItem(const QPoint &cell);
    void unloadItems(const QLine &items);

    void loadInitialTopLeftItem();
    void loadEdgesInsideRect(const QRectF &rect, QQmlIncubator::IncubationMode incubationMode);
    void unloadEdgesOutsideRect(const QRectF &rect);
    void loadAndUnloadVisibleEdges();
    void cancelLoadRequest();
    void processLoadRequest();
    void beginRebuildTable();
    void endRebuildTable();

    void loadBuffer();
    void unloadBuffer();
    QRectF bufferRect();

    void invalidateTable();
    void invalidateColumnRowPositions();

    void createWrapperModel();

    void initItemCallback(int modelIndex, QObject *item);
    void itemCreatedCallback(int modelIndex, QObject *object);
    void modelUpdated(const QQmlChangeSet &changeSet, bool reset);

    inline QString tableLayoutToString() const;
    void dumpTable() const;
};

class FxTableItem : public QQuickItemViewFxItem
{
public:
    FxTableItem(QQuickItem *item, QQuickTableView *table, bool own)
        : QQuickItemViewFxItem(item, own, QQuickTableViewPrivate::get(table))
    {
    }

    qreal position() const override { return 0; }
    qreal endPosition() const override { return 0; }
    qreal size() const override { return 0; }
    qreal sectionSize() const override { return 0; }
    bool contains(qreal, qreal) const override { return false; }

    QPoint cell;
};

Q_DECLARE_TYPEINFO(QQuickTableViewPrivate::ColumnRowSize, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE
