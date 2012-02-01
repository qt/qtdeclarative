/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKITEMVIEW_P_P_H
#define QQUICKITEMVIEW_P_P_H

#include "qquickitemview_p.h"
#include "qquickflickable_p_p.h"
#include "qquickvisualdatamodel_p.h"
#include <private/qdeclarativechangeset_p.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class FxViewItem
{
public:
    FxViewItem(QQuickItem *, bool own);
    ~FxViewItem();

    // these are positions and sizes along the current direction of scrolling/flicking
    virtual qreal position() const = 0;
    virtual qreal endPosition() const = 0;
    virtual qreal size() const = 0;
    virtual qreal sectionSize() const = 0;

    virtual bool contains(qreal x, qreal y) const = 0;

    QQuickItem *item;
    bool ownItem;
    int index;
    QQuickItemViewAttached *attached;
};

class QQuickItemViewChangeSet
{
public:
    QQuickItemViewChangeSet();

    bool hasPendingChanges() const;
    void prepare(int currentIndex, int count);
    void reset();

    void applyChanges(const QDeclarativeChangeSet &changeSet);

    int itemCount;
    int newCurrentIndex;
    QDeclarativeChangeSet pendingChanges;
    QHash<QDeclarativeChangeSet::MoveKey, FxViewItem *> removedItems;

    bool active : 1;
    bool currentChanged : 1;
    bool currentRemoved : 1;
};

class QQuickItemViewPrivate : public QQuickFlickablePrivate
{
    Q_DECLARE_PUBLIC(QQuickItemView)
public:
    QQuickItemViewPrivate();

    struct ChangeResult {
        QDeclarativeNullableValue<qreal> visiblePos;
        qreal sizeChangesBeforeVisiblePos;
        qreal sizeChangesAfterVisiblePos;
        bool changedFirstItem;
        int changeBeforeVisible;

        ChangeResult(const QDeclarativeNullableValue<qreal> &p)
            : visiblePos(p), sizeChangesBeforeVisiblePos(0), sizeChangesAfterVisiblePos(0),
            changedFirstItem(false), changeBeforeVisible(0) {}

        void reset() {
            sizeChangesBeforeVisiblePos = 0.0;
            sizeChangesAfterVisiblePos = 0.0;
            changedFirstItem = false;
            changeBeforeVisible = 0;
        }
    };

    enum BufferMode { NoBuffer = 0x00, BufferBefore = 0x01, BufferAfter = 0x02 };
    enum MovementReason { Other, SetIndex, Mouse };

    bool isValid() const;
    qreal position() const;
    qreal size() const;
    qreal startPosition() const;
    qreal endPosition() const;
    qreal contentStartPosition() const;
    int findLastVisibleIndex(int defaultValue = -1) const;
    FxViewItem *visibleItem(int modelIndex) const;
    FxViewItem *firstVisibleItem() const;
    int mapFromModel(int modelIndex) const;

    virtual void init();
    virtual void clear();
    virtual void updateViewport();

    void regenerate();
    void layout();
    void refill();
    void refill(qreal from, qreal to);
    void mirrorChange();

    FxViewItem *createItem(int modelIndex, bool asynchronous = false);
    virtual void releaseItem(FxViewItem *item);

    QQuickItem *createHighlightItem();
    QQuickItem *createComponentItem(QDeclarativeComponent *component, bool receiveItemGeometryChanges, bool createDefault = false);

    void updateCurrent(int modelIndex);
    void updateTrackedItem();
    void updateUnrequestedIndexes();
    void updateUnrequestedPositions();
    void updateVisibleIndex();
    void positionViewAtIndex(int index, int mode);
    void applyPendingChanges();
    bool applyModelChanges();
    bool applyRemovalChange(const QDeclarativeChangeSet::Remove &removal, ChangeResult *changeResult, int *removedCount);
    void repositionFirstItem(FxViewItem *prevVisibleItemsFirst, qreal prevVisibleItemsFirstPos,
            FxViewItem *prevFirstVisible, ChangeResult *insertionResult, ChangeResult *removalResult);

    void checkVisible() const;

    void markExtentsDirty() {
        if (layoutOrientation() == Qt::Vertical)
            vData.markExtentsDirty();
        else
            hData.markExtentsDirty();
    }

    QDeclarativeGuard<QQuickVisualModel> model;
    QVariant modelVariant;
    int itemCount;
    int buffer;
    int bufferMode;
    Qt::LayoutDirection layoutDirection;

    MovementReason moveReason;

    QList<FxViewItem *> visibleItems;
    int visibleIndex;
    int currentIndex;
    FxViewItem *currentItem;
    FxViewItem *trackedItem;
    QHash<QQuickItem*,int> unrequestedItems;
    int requestedIndex;
    FxViewItem *requestedItem;
    QQuickItemViewChangeSet currentChanges;

    // XXX split into struct
    QDeclarativeComponent *highlightComponent;
    FxViewItem *highlight;
    int highlightRange;     // enum value
    qreal highlightRangeStart;
    qreal highlightRangeEnd;
    int highlightMoveDuration;

    QDeclarativeComponent *headerComponent;
    FxViewItem *header;
    QDeclarativeComponent *footerComponent;
    FxViewItem *footer;

    mutable qreal minExtent;
    mutable qreal maxExtent;

    bool ownModel : 1;
    bool wrap : 1;
    bool inApplyModelChanges : 1;
    bool inViewportMoved : 1;
    bool forceLayout : 1;
    bool currentIndexCleared : 1;
    bool haveHighlightRange : 1;
    bool autoHighlight : 1;
    bool highlightRangeStartValid : 1;
    bool highlightRangeEndValid : 1;
    bool fillCacheBuffer : 1;
    bool inRequest : 1;
    bool requestedAsync : 1;

protected:
    virtual Qt::Orientation layoutOrientation() const = 0;
    virtual bool isContentFlowReversed() const = 0;

    virtual qreal positionAt(int index) const = 0;
    virtual qreal endPositionAt(int index) const = 0;
    virtual qreal originPosition() const = 0;
    virtual qreal lastPosition() const = 0;

    virtual qreal headerSize() const = 0;
    virtual qreal footerSize() const = 0;
    virtual bool showHeaderForIndex(int index) const = 0;
    virtual bool showFooterForIndex(int index) const = 0;
    virtual void updateHeader() = 0;
    virtual void updateFooter() = 0;

    virtual void createHighlight() = 0;
    virtual void updateHighlight() = 0;
    virtual void resetHighlightPosition() = 0;

    virtual void setPosition(qreal pos) = 0;
    virtual void fixupPosition() = 0;

    virtual bool addVisibleItems(qreal fillFrom, qreal fillTo, bool doBuffer) = 0;
    virtual bool removeNonVisibleItems(qreal bufferFrom, qreal bufferTo) = 0;
    virtual void visibleItemsChanged() {}

    virtual FxViewItem *newViewItem(int index, QQuickItem *item) = 0;
    virtual void repositionPackageItemAt(QQuickItem *item, int index) = 0;
    virtual void resetFirstItemPosition(qreal pos = 0.0) = 0;
    virtual void adjustFirstItem(qreal forwards, qreal backwards, int changeBeforeVisible) = 0;

    virtual void layoutVisibleItems(int fromModelIndex = 0) = 0;
    virtual void changedVisibleIndex(int newIndex) = 0;
    virtual bool applyInsertionChange(const QDeclarativeChangeSet::Insert &insert, ChangeResult *changeResult, QList<FxViewItem *> *newItems) = 0;

    virtual bool needsRefillForAddedOrRemovedIndex(int) const { return false; }

    virtual void initializeViewItem(FxViewItem *) {}
    virtual void initializeCurrentItem() {}
    virtual void updateSections() {}

    virtual void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
};


QT_END_NAMESPACE

QT_END_HEADER

#endif // QQUICKITEMVIEW_P_P_H
