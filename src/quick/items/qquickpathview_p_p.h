/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQUICKPATHVIEW_P_P_H
#define QQUICKPATHVIEW_P_P_H

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

#include "qquickpathview_p.h"
#include "qquickitem_p.h"
#include "qquickvisualdatamodel_p.h"

#include <QtQml/qqml.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qcoreapplication.h>

#include <private/qquickanimation_p_p.h>
#include <private/qqmlguard_p.h>
#include <private/qquicktimeline_p_p.h>

QT_BEGIN_NAMESPACE

class QQmlOpenMetaObjectType;
class QQuickPathViewAttached;
class QQuickPathViewPrivate : public QQuickItemPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickPathView)

public:
    QQuickPathViewPrivate()
      : path(0), currentIndex(0), currentItemOffset(0.0), startPc(0), lastDist(0)
        , lastElapsed(0), offset(0.0), offsetAdj(0.0), mappedRange(1.0)
        , stealMouse(false), ownModel(false), interactive(true), haveHighlightRange(true)
        , autoHighlight(true), highlightUp(false), layoutScheduled(false)
        , moving(false), flicking(false), requestedOnPath(false), inRequest(false)
        , dragMargin(0), deceleration(100)
        , moveOffset(this, &QQuickPathViewPrivate::setAdjustedOffset), flickDuration(0)
        , firstIndex(-1), pathItems(-1), requestedIndex(-1), requestedZ(0)
        , moveReason(Other), moveDirection(Shortest), attType(0), highlightComponent(0), highlightItem(0)
        , moveHighlight(this, &QQuickPathViewPrivate::setHighlightPosition)
        , highlightPosition(0)
        , highlightRangeStart(0), highlightRangeEnd(0)
        , highlightRangeMode(QQuickPathView::StrictlyEnforceRange)
        , highlightMoveDuration(300), modelCount(0)
    {
    }

    void init();

    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) {
        if ((newGeometry.size() != oldGeometry.size())
            && (!highlightItem || item != highlightItem)) {
            if (QQuickPathViewAttached *att = attached(item))
                att->m_percent = -1;
            scheduleLayout();
        }
    }

    void scheduleLayout() {
        Q_Q(QQuickPathView);
        if (!layoutScheduled) {
            layoutScheduled = true;
            q->polish();
        }
    }

    QQuickItem *getItem(int modelIndex, qreal z = 0, bool onPath=true);
    void releaseItem(QQuickItem *item);
    QQuickPathViewAttached *attached(QQuickItem *item);
    QQmlOpenMetaObjectType *attachedType();
    void clear();
    void updateMappedRange();
    qreal positionOfIndex(qreal index) const;
    void createHighlight();
    void updateHighlight();
    void setHighlightPosition(qreal pos);
    bool isValid() const {
        return model && model->count() > 0 && model->isValid() && path;
    }

    void handleMousePressEvent(QMouseEvent *event);
    void handleMouseMoveEvent(QMouseEvent *event);
    void handleMouseReleaseEvent(QMouseEvent *);

    int calcCurrentIndex();
    void createCurrentItem();
    void updateCurrent();
    static void fixOffsetCallback(void*);
    void fixOffset();
    void setOffset(qreal offset);
    void setAdjustedOffset(qreal offset);
    void regenerate();
    void updateItem(QQuickItem *, qreal);
    void snapToCurrent();
    QPointF pointNear(const QPointF &point, qreal *nearPercent=0) const;
    void addVelocitySample(qreal v);
    qreal calcVelocity() const;

    QQuickPath *path;
    int currentIndex;
    QQmlGuard<QQuickItem> currentItem;
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
    bool requestedOnPath : 1;
    bool inRequest : 1;
    QElapsedTimer lastPosTime;
    QPointF lastPos;
    qreal dragMargin;
    qreal deceleration;
    QQuickTimeLine tl;
    QQuickTimeLineValueProxy<QQuickPathViewPrivate> moveOffset;
    int flickDuration;
    int firstIndex;
    int pathItems;
    int requestedIndex;
    qreal requestedZ;
    QList<QQuickItem *> items;
    QList<QQuickItem *> itemCache;
    QQmlGuard<QQuickVisualModel> model;
    QVariant modelVariant;
    enum MovementReason { Other, SetIndex, Mouse };
    MovementReason moveReason;
    enum MovementDirection { Shortest, Negative, Positive };
    MovementDirection moveDirection;
    QQmlOpenMetaObjectType *attType;
    QQmlComponent *highlightComponent;
    QQuickItem *highlightItem;
    QQuickTimeLineValueProxy<QQuickPathViewPrivate> moveHighlight;
    qreal highlightPosition;
    qreal highlightRangeStart;
    qreal highlightRangeEnd;
    QQuickPathView::HighlightRangeMode highlightRangeMode;
    int highlightMoveDuration;
    int modelCount;
    QPODVector<qreal,10> velocityBuffer;
};

QT_END_NAMESPACE

#endif
