/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEPATHVIEW_P_H
#define QDECLARATIVEPATHVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qdeclarativepathview_p.h"

#include "QtQuick1/private/qdeclarativeitem_p.h"
#include "QtQuick1/private/qdeclarativevisualitemmodel_p.h"

#include <QtDeclarative/qdeclarative.h>
#include <QtQuick1/private/qdeclarativeanimation_p_p.h>
#include <QtDeclarative/private/qdeclarativeguard_p.h>

#include <qdatetime.h>

QT_BEGIN_NAMESPACE

class QDeclarative1OpenMetaObjectType;
class QDeclarative1PathViewAttached;
class QDeclarative1PathViewPrivate : public QDeclarativeItemPrivate, public QDeclarativeItemChangeListener
{
    Q_DECLARE_PUBLIC(QDeclarative1PathView)

public:
    QDeclarative1PathViewPrivate()
      : path(0), currentIndex(0), currentItemOffset(0.0), startPc(0), lastDist(0)
        , lastElapsed(0), offset(0.0), offsetAdj(0.0), mappedRange(1.0)
        , stealMouse(false), ownModel(false), interactive(true), haveHighlightRange(true)
        , autoHighlight(true), highlightUp(false), layoutScheduled(false)
        , moving(false), flicking(false)
        , dragMargin(0), deceleration(100)
        , moveOffset(this, &QDeclarative1PathViewPrivate::setAdjustedOffset)
        , firstIndex(-1), pathItems(-1), requestedIndex(-1)
        , moveReason(Other), moveDirection(Shortest), attType(0), highlightComponent(0), highlightItem(0)
        , moveHighlight(this, &QDeclarative1PathViewPrivate::setHighlightPosition)
        , highlightPosition(0)
        , highlightRangeStart(0), highlightRangeEnd(0)
        , highlightRangeMode(QDeclarative1PathView::StrictlyEnforceRange)
        , highlightMoveDuration(300), modelCount(0)
    {
    }

    void init();

    void itemGeometryChanged(QDeclarativeItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) {
        if ((newGeometry.size() != oldGeometry.size())
            && (!highlightItem || item != highlightItem)) {
            if (QDeclarative1PathViewAttached *att = attached(item))
                att->m_percent = -1;
            scheduleLayout();
        }
    }

    void scheduleLayout() {
        Q_Q(QDeclarative1PathView);
        if (!layoutScheduled) {
            layoutScheduled = true;
            QCoreApplication::postEvent(q, new QEvent(QEvent::User), Qt::HighEventPriority);
        }
    }

    QDeclarativeItem *getItem(int modelIndex);
    void releaseItem(QDeclarativeItem *item);
    QDeclarative1PathViewAttached *attached(QDeclarativeItem *item);
    void clear();
    void updateMappedRange();
    qreal positionOfIndex(qreal index) const;
    void createHighlight();
    void updateHighlight();
    void setHighlightPosition(qreal pos);
    bool isValid() const {
        return model && model->count() > 0 && model->isValid() && path;
    }

    void handleMousePressEvent(QGraphicsSceneMouseEvent *event);
    void handleMouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void handleMouseReleaseEvent(QGraphicsSceneMouseEvent *);

    int calcCurrentIndex();
    void updateCurrent();
    static void fixOffsetCallback(void*);
    void fixOffset();
    void setOffset(qreal offset);
    void setAdjustedOffset(qreal offset);
    void regenerate();
    void updateItem(QDeclarativeItem *, qreal);
    void snapToCurrent();
    QPointF pointNear(const QPointF &point, qreal *nearPercent=0) const;

    QDeclarative1Path *path;
    int currentIndex;
    QDeclarativeGuard<QDeclarativeItem> currentItem;
    qreal currentItemOffset;
    qreal startPc;
    QPointF startPoint;
    qreal lastDist;
    int lastElapsed;
    qreal offset;
    qreal offsetAdj;
    qreal mappedRange;
    bool stealMouse : 1;
    bool ownModel : 1;
    bool interactive : 1;
    bool haveHighlightRange : 1;
    bool autoHighlight : 1;
    bool highlightUp : 1;
    bool layoutScheduled : 1;
    bool moving : 1;
    bool flicking : 1;
    QElapsedTimer lastPosTime;
    QPointF lastPos;
    qreal dragMargin;
    qreal deceleration;
    QDeclarative1TimeLine tl;
    QDeclarative1TimeLineValueProxy<QDeclarative1PathViewPrivate> moveOffset;
    int firstIndex;
    int pathItems;
    int requestedIndex;
    QList<QDeclarativeItem *> items;
    QList<QDeclarativeItem *> itemCache;
    QDeclarativeGuard<QDeclarative1VisualModel> model;
    QVariant modelVariant;
    enum MovementReason { Other, SetIndex, Mouse };
    MovementReason moveReason;
    enum MovementDirection { Shortest, Negative, Positive };
    MovementDirection moveDirection;
    QDeclarative1OpenMetaObjectType *attType;
    QDeclarativeComponent *highlightComponent;
    QDeclarativeItem *highlightItem;
    QDeclarative1TimeLineValueProxy<QDeclarative1PathViewPrivate> moveHighlight;
    qreal highlightPosition;
    qreal highlightRangeStart;
    qreal highlightRangeEnd;
    QDeclarative1PathView::HighlightRangeMode highlightRangeMode;
    int highlightMoveDuration;
    int modelCount;
};

QT_END_NAMESPACE

#endif
