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

#ifndef QSGITEMVIEW_P_P_H
#define QSGITEMVIEW_P_P_H

#include "qsgitemview_p.h"
#include "qsgflickable_p_p.h"
#include "qsgvisualitemmodel_p.h"
#include <private/qdeclarativechangeset_p.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class FxViewItem
{
public:
    FxViewItem(QSGItem *, bool own);
    ~FxViewItem();

    // these are positions and sizes along the current direction of scrolling/flicking
    virtual qreal position() const = 0;
    virtual qreal endPosition() const = 0;
    virtual qreal size() const = 0;
    virtual qreal sectionSize() const = 0;

    virtual bool contains(qreal x, qreal y) const = 0;

    QSGItem *item;
    bool ownItem;
    int index;
    QSGItemViewAttached *attached;
};

class QSGItemViewChangeSet
{
public:
    QSGItemViewChangeSet();

    bool hasPendingChanges() const;
    void prepare(int currentIndex, int count);
    void reset();

    void doInsert(int index, int count);
    void doRemove(int index, int count);
    void doMove(int from, int to, int count);
    void doChange(int index, int count);

    int itemCount;
    int newCurrentIndex;
    QDeclarativeChangeSet pendingChanges;
    QHash<QDeclarativeChangeSet::MoveKey, FxViewItem *> removedItems;

    bool active : 1;
    bool currentChanged : 1;
    bool currentRemoved : 1;
};

class QSGItemViewPrivate : public QSGFlickablePrivate
{
    Q_DECLARE_PUBLIC(QSGItemView)
public:
    QSGItemViewPrivate();

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
    void refill(qreal from, qreal to, bool doBuffer = false);
    void scheduleLayout();
    void mirrorChange();

    FxViewItem *createItem(int modelIndex);
    virtual void releaseItem(FxViewItem *item);

    QSGItem *createHighlightItem();
    QSGItem *createComponentItem(QDeclarativeComponent *component, bool receiveItemGeometryChanges, bool createDefault = false);

    void updateCurrent(int modelIndex);
    void updateTrackedItem();
    void updateUnrequestedIndexes();
    void updateUnrequestedPositions();
    void updateVisibleIndex();
    void positionViewAtIndex(int index, int mode);
    void applyPendingChanges();
    bool applyModelChanges();

    void checkVisible() const;

    QDeclarativeGuard<QSGVisualModel> model;
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
    QHash<QSGItem*,int> unrequestedItems;
    int requestedIndex;
    QSGItemViewChangeSet currentChanges;

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
    bool lazyRelease : 1;
    bool deferredRelease : 1;
    bool layoutScheduled : 1;
    bool inApplyModelChanges : 1;
    bool inViewportMoved : 1;
    bool forceLayout : 1;
    bool currentIndexCleared : 1;
    bool haveHighlightRange : 1;
    bool autoHighlight : 1;
    bool highlightRangeStartValid : 1;
    bool highlightRangeEndValid : 1;
    mutable bool minExtentDirty : 1;
    mutable bool maxExtentDirty : 1;

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

    virtual bool addVisibleItems(int fillFrom, int fillTo, bool doBuffer) = 0;
    virtual bool removeNonVisibleItems(int bufferFrom, int bufferTo) = 0;
    virtual void visibleItemsChanged() = 0;

    virtual FxViewItem *newViewItem(int index, QSGItem *item) = 0;
    virtual void repositionPackageItemAt(QSGItem *item, int index) = 0;
    virtual void resetItemPosition(FxViewItem *item, FxViewItem *toItem) = 0;
    virtual void resetFirstItemPosition() = 0;
    virtual void moveItemBy(FxViewItem *item, const QList<FxViewItem *> &, const QList<FxViewItem *> &) = 0;

    virtual void layoutVisibleItems() = 0;
    virtual void changedVisibleIndex(int newIndex) = 0;
    virtual bool applyInsertionChange(const QDeclarativeChangeSet::Insert &, QList<FxViewItem *> *, QList<FxViewItem *> *, FxViewItem *) = 0;

    virtual void initializeViewItem(FxViewItem *) {}
    virtual void initializeCurrentItem() {}
    virtual void updateSections() {}

    virtual void itemGeometryChanged(QSGItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
};


QT_END_NAMESPACE

QT_END_HEADER

#endif // QSGITEMVIEW_P_P_H
