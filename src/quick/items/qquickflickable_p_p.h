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

#ifndef QQUICKFLICKABLE_P_P_H
#define QQUICKFLICKABLE_P_P_H

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

#include "qquickflickable_p.h"
#include "qquickitem_p.h"
#include "qquickitemchangelistener_p.h"

#include <QtQml/qqml.h>
#include <QtCore/qdatetime.h>
#include "qplatformdefs.h"

#include <private/qquicktimeline_p_p.h>
#include <private/qquickanimation_p_p.h>

QT_BEGIN_NAMESPACE

// Really slow flicks can be annoying.
const qreal MinimumFlickVelocity = 75.0;

class QQuickFlickableVisibleArea;
class Q_AUTOTEST_EXPORT QQuickFlickablePrivate : public QQuickItemPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickFlickable)

public:
    static inline QQuickFlickablePrivate *get(QQuickFlickable *o) { return o->d_func(); }

    QQuickFlickablePrivate();
    void init();

    struct Velocity : public QQuickTimeLineValue
    {
        Velocity(QQuickFlickablePrivate *p)
            : parent(p) {}
        virtual void setValue(qreal v) {
            if (v != value()) {
                QQuickTimeLineValue::setValue(v);
                parent->updateVelocity();
            }
        }
        QQuickFlickablePrivate *parent;
    };

    struct AxisData {
        AxisData(QQuickFlickablePrivate *fp, void (QQuickFlickablePrivate::*func)(qreal))
            : move(fp, func), viewSize(-1), startMargin(0), endMargin(0)
            , continuousFlickVelocity(0)
            , smoothVelocity(fp), atEnd(false), atBeginning(true)
            , fixingUp(false), inOvershoot(false), moving(false), flicking(false)
            , dragging(false), extentsChanged(false)
            , explicitValue(false), minExtentDirty(true), maxExtentDirty(true)
        {}

        void reset() {
            velocityBuffer.clear();
            dragStartOffset = 0;
            fixingUp = false;
            inOvershoot = false;
        }

        void markExtentsDirty() {
            minExtentDirty = true;
            maxExtentDirty = true;
            extentsChanged = true;
        }

        void addVelocitySample(qreal v, qreal maxVelocity);
        void updateVelocity();

        QQuickTimeLineValueProxy<QQuickFlickablePrivate> move;
        qreal viewSize;
        qreal pressPos;
        qreal dragStartOffset;
        qreal dragMinBound;
        qreal dragMaxBound;
        qreal velocity;
        qreal flickTarget;
        qreal startMargin;
        qreal endMargin;
        qreal continuousFlickVelocity;
        QQuickFlickablePrivate::Velocity smoothVelocity;
        QPODVector<qreal,10> velocityBuffer;
        bool atEnd : 1;
        bool atBeginning : 1;
        bool fixingUp : 1;
        bool inOvershoot : 1;
        bool moving : 1;
        bool flicking : 1;
        bool dragging : 1;
        bool extentsChanged : 1;
        bool explicitValue : 1;
        mutable bool minExtentDirty : 1;
        mutable bool maxExtentDirty : 1;
    };

    bool flickX(qreal velocity);
    bool flickY(qreal velocity);
    virtual bool flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                        QQuickTimeLineCallback::Callback fixupCallback, qreal velocity);
    void flickingStarted(bool flickingH, bool flickingV);

    void fixupX();
    void fixupY();
    virtual void fixup(AxisData &data, qreal minExtent, qreal maxExtent);

    void updateBeginningEnd();

    bool isOutermostPressDelay() const;
    void captureDelayedPress(QMouseEvent *event);
    void clearDelayedPress();

    void setViewportX(qreal x);
    void setViewportY(qreal y);

    qreal overShootDistance(qreal size);

    void itemGeometryChanged(QQuickItem *, const QRectF &, const QRectF &);

    void draggingStarting();
    void draggingEnding();

public:
    QQuickItem *contentItem;

    AxisData hData;
    AxisData vData;

    QQuickTimeLine timeline;
    bool hMoved : 1;
    bool vMoved : 1;
    bool stealMouse : 1;
    bool pressed : 1;
    bool interactive : 1;
    bool calcVelocity : 1;
    bool pixelAligned : 1;
    QElapsedTimer timer;
    qint64 lastPosTime;
    qint64 lastPressTime;
    QPointF lastPos;
    QPointF pressPos;
    qreal deceleration;
    qreal maxVelocity;
    QElapsedTimer velocityTime;
    QPointF lastFlickablePosition;
    qreal reportedVelocitySmoothing;
    QMouseEvent *delayedPressEvent;
    QQuickItem *delayedPressTarget;
    QBasicTimer delayedPressTimer;
    int pressDelay;
    int fixupDuration;
    qreal flickBoost;

    enum FixupMode { Normal, Immediate, ExtentChanged };
    FixupMode fixupMode;

    static void fixupY_callback(void *);
    static void fixupX_callback(void *);

    void updateVelocity();
    int vTime;
    QQuickTimeLine velocityTimeline;
    QQuickFlickableVisibleArea *visibleArea;
    QQuickFlickable::FlickableDirection flickableDirection;
    QQuickFlickable::BoundsBehavior boundsBehavior;

    void handleMousePressEvent(QMouseEvent *);
    void handleMouseMoveEvent(QMouseEvent *);
    void handleMouseReleaseEvent(QMouseEvent *);

    qint64 computeCurrentTime(QInputEvent *event);

    // flickableData property
    static void data_append(QQmlListProperty<QObject> *, QObject *);
    static int data_count(QQmlListProperty<QObject> *);
    static QObject *data_at(QQmlListProperty<QObject> *, int);
    static void data_clear(QQmlListProperty<QObject> *);
};

class QQuickFlickableVisibleArea : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal xPosition READ xPosition NOTIFY xPositionChanged)
    Q_PROPERTY(qreal yPosition READ yPosition NOTIFY yPositionChanged)
    Q_PROPERTY(qreal widthRatio READ widthRatio NOTIFY widthRatioChanged)
    Q_PROPERTY(qreal heightRatio READ heightRatio NOTIFY heightRatioChanged)

public:
    QQuickFlickableVisibleArea(QQuickFlickable *parent=0);

    qreal xPosition() const;
    qreal widthRatio() const;
    qreal yPosition() const;
    qreal heightRatio() const;

    void updateVisible();

signals:
    void xPositionChanged(qreal xPosition);
    void yPositionChanged(qreal yPosition);
    void widthRatioChanged(qreal widthRatio);
    void heightRatioChanged(qreal heightRatio);

private:
    QQuickFlickable *flickable;
    qreal m_xPosition;
    qreal m_widthRatio;
    qreal m_yPosition;
    qreal m_heightRatio;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFlickableVisibleArea)

#endif // QQUICKFLICKABLE_P_P_H
