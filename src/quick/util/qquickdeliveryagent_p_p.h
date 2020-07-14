/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QQUICKDELIVERYAGENT_P_P_H
#define QQUICKDELIVERYAGENT_P_P_H

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

#include <QtQuick/private/qquickdeliveryagent_p.h>
#include <QtGui/qevent.h>
#include <QtCore/qstack.h>

#include <private/qevent_p.h>
#include <private/qpointingdevice_p.h>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQuickDragGrabber;
class QQuickItem;
class QQuickPointerHandler;
class QQuickWindow;

/*! \internal
    Extra device-specific data to be stored in QInputDevicePrivate::qqExtra
*/
struct QQuickPointingDeviceExtra {
    // used in QQuickPointerHandlerPrivate::deviceDeliveryTargets
    QVector<QObject *> deliveryTargets;
    // memory of which agent was delivering when each QEventPoint was grabbed
    // TODO maybe add QEventPointPrivate::qqExtra, or sth in QPointingDevicePrivate::EventPointData
    QFlatMap<int, QQuickDeliveryAgent*> grabbedEventPointDeliveryAgents;
};

class Q_QUICK_PRIVATE_EXPORT QQuickDeliveryAgentPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickDeliveryAgent)
public:
    QQuickDeliveryAgentPrivate(QQuickItem *root);
    ~QQuickDeliveryAgentPrivate();

    QQuickItem *rootItem = nullptr;

    QQuickItem *activeFocusItem = nullptr;

    void deliverKeyEvent(QKeyEvent *e);

    enum FocusOption {
        DontChangeFocusProperty = 0x01,
        DontChangeSubFocusItem  = 0x02
    };
    Q_DECLARE_FLAGS(FocusOptions, FocusOption)

    void setFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason, FocusOptions = { });
    void clearFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason, FocusOptions = { });
    static void notifyFocusChangesRecur(QQuickItem **item, int remaining);
    void clearFocusObject();
    void updateFocusItemTransform();

    // Keeps track of the item currently receiving mouse events
#if QT_CONFIG(quick_draganddrop)
    QQuickDragGrabber *dragGrabber = nullptr;
#endif
    QQuickItem *lastUngrabbed = nullptr;
    QStack<QPointerEvent *> eventsInDelivery;
    QList<QPointer<QQuickItem>> hoverItems;
    QVector<QQuickItem *> hasFiltered; // during event delivery to a single receiver, the filtering parents for which childMouseEventFilter was already called
    QVector<QQuickItem *> skipDelivery; // during delivery of one event to all receivers, Items to which we know delivery is no longer necessary

    QScopedPointer<QMutableTouchEvent> delayedTouch;
    QList<const QPointingDevice *> knownPointingDevices;

    uint lastWheelEventAccepted = 0;
    bool allowChildEventFiltering = true;
    bool allowDoubleClick = true;
    bool frameSynchronousHoverEnabled = true;

    bool isSubsceneAgent = false;

    Qt::FocusReason lastFocusReason = Qt::OtherFocusReason;
    int pointerEventRecursionGuard = 0;

    int touchMouseId = -1; // only for obsolete stuff like QQuickItem::grabMouse()
    // TODO get rid of these
    const QPointingDevice *touchMouseDevice = nullptr;
    ulong touchMousePressTimestamp = 0;
    QPoint touchMousePressPos;      // in screen coordinates

    QQuickDeliveryAgent::Transform *sceneTransform = nullptr;

    bool isDeliveringTouchAsMouse() const { return touchMouseId != -1 && touchMouseDevice; }
    void cancelTouchMouseSynthesis();

    bool checkIfDoubleTapped(ulong newPressEventTimestamp, QPoint newPressPos);
    QPointingDevicePrivate::EventPointData *mousePointData();
    QPointerEvent *eventInDelivery() const;

    // Mouse positions are saved in widget coordinates
    QPointF lastMousePosition;
    bool deliverTouchAsMouse(QQuickItem *item, QTouchEvent *pointerEvent);
    void translateTouchEvent(QTouchEvent *touchEvent);
    void removeGrabber(QQuickItem *grabber, bool mouse = true, bool touch = true, bool cancel = false);
    void onGrabChanged(QObject *grabber, QPointingDevice::GrabTransition transition, const QPointerEvent *event, const QEventPoint &point);
    static QPointerEvent *clonePointerEvent(QPointerEvent *event, std::optional<QPointF> transformedLocalPos = std::nullopt);
    void deliverToPassiveGrabbers(const QVector<QPointer<QObject> > &passiveGrabbers, QPointerEvent *pointerEvent);
    bool sendFilteredMouseEvent(QEvent *event, QQuickItem *receiver, QQuickItem *filteringParent);
    bool sendFilteredPointerEvent(QPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent = nullptr);
    bool sendFilteredPointerEventImpl(QPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent);
    bool deliverSinglePointEventUntilAccepted(QPointerEvent *);

    // entry point of events to the window
    void handleTouchEvent(QTouchEvent *);
    void handleMouseEvent(QMouseEvent *);
    bool compressTouchEvent(QTouchEvent *);
    void flushFrameSynchronousEvents(QQuickWindow *win);
    void deliverDelayedTouchEvent();
    void handleWindowDeactivate(QQuickWindow *win);

    // utility functions that used to be in QQuickPointerEvent et al.
    bool allUpdatedPointsAccepted(const QPointerEvent *ev);
    static void localizePointerEvent(QPointerEvent *ev, const QQuickItem *dest);
    QList<QObject *> exclusiveGrabbers(QPointerEvent *ev);
    static bool anyPointGrabbed(const QPointerEvent *ev);
    static bool isMouseEvent(const QPointerEvent *ev);
    static bool isTouchEvent(const QPointerEvent *ev);
    static bool isTabletEvent(const QPointerEvent *ev);
    static QQuickPointingDeviceExtra *deviceExtra(const QInputDevice *device);

    // delivery of pointer events:
    void touchToMouseEvent(QEvent::Type type, const QEventPoint &p, const QTouchEvent *touchEvent, QMutableSinglePointEvent *mouseEvent);
    void ensureDeviceConnected(const QPointingDevice *dev);
    void deliverPointerEvent(QPointerEvent *);
    bool deliverTouchCancelEvent(QTouchEvent *);
    bool deliverPressOrReleaseEvent(QPointerEvent *, bool handlersOnly = false);
    void deliverUpdatedPoints(QPointerEvent *event);
    void deliverMatchingPointsToItem(QQuickItem *item, bool isGrabber, QPointerEvent *pointerEvent, bool handlersOnly = false);

    QVector<QQuickItem *> pointerTargets(QQuickItem *, const QPointerEvent *event, const QEventPoint &point,
                                         bool checkMouseButtons, bool checkAcceptsTouch) const;
    QVector<QQuickItem *> mergePointerTargets(const QVector<QQuickItem *> &list1, const QVector<QQuickItem *> &list2) const;

    // hover delivery
    bool deliverHoverEvent(QQuickItem *, const QPointF &scenePos, const QPointF &lastScenePos, Qt::KeyboardModifiers modifiers, ulong timestamp, bool &accepted);
    bool sendHoverEvent(QEvent::Type, QQuickItem *, const QPointF &scenePos, const QPointF &lastScenePos,
                        Qt::KeyboardModifiers modifiers, ulong timestamp, bool accepted);
    bool clearHover(ulong timestamp = 0);

#if QT_CONFIG(quick_draganddrop)
    void deliverDragEvent(QQuickDragGrabber *, QEvent *);
    bool deliverDragEvent(QQuickDragGrabber *, QQuickItem *, QDragMoveEvent *, QVarLengthArray<QQuickItem*, 64> *currentGrabItems = nullptr);
#endif

    static bool dragOverThreshold(qreal d, Qt::Axis axis, QMouseEvent *event, int startDragThreshold = -1);

    static bool dragOverThreshold(qreal d, Qt::Axis axis, const QEventPoint &tp, int startDragThreshold = -1);

    static bool dragOverThreshold(QVector2D delta);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickDeliveryAgentPrivate::FocusOptions)

QT_END_NAMESPACE

#endif // QQUICKDELIVERYAGENT_P_P_H
