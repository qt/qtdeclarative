/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QQUICKWINDOW_P_H
#define QQUICKWINDOW_P_H

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

#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qquickpaletteproviderprivatebase_p.h>
#include <QtQuick/private/qquickrendertarget_p.h>
#include <QtQuick/private/qquickgraphicsdevice_p.h>
#include <QtQuick/private/qquickgraphicsconfiguration_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>

#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qrunnable.h>

#include <QtGui/private/qwindow_p.h>
#include <QtGui/qevent.h>
#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QQuickAnimatorController;
class QQuickDragGrabber;
class QQuickItemPrivate;
class QPointingDevice;
class QQuickRenderControl;
class QQuickWindowIncubationController;
class QQuickWindowPrivate;
class QQuickWindowRenderLoop;
class QSGRenderLoop;
class QTouchEvent;
class QRhi;
class QRhiSwapChain;
class QRhiRenderBuffer;
class QRhiRenderPassDescriptor;
class QRhiTexture;

//Make it easy to identify and customize the root item if needed
class Q_QUICK_PRIVATE_EXPORT QQuickRootItem : public QQuickItem
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickRootItem();
public Q_SLOTS:
    void setWidth(int w) {QQuickItem::setWidth(qreal(w));}
    void setHeight(int h) {QQuickItem::setHeight(qreal(h));}
};

class QQuickWindowRenderTarget
{
public:
    void reset(QRhi *rhi, QSGRenderer *renderer);
    QRhiRenderTarget *renderTarget = nullptr;
    QRhiRenderPassDescriptor *rpDesc = nullptr;
    QRhiTexture *texture = nullptr;
    QRhiRenderBuffer *depthStencil = nullptr;
    bool owns = false;
};

class Q_QUICK_PRIVATE_EXPORT QQuickWindowPrivate
    : public QWindowPrivate
    , public QQuickPaletteProviderPrivateBase<QQuickWindow, QQuickWindowPrivate>
{
public:
    Q_DECLARE_PUBLIC(QQuickWindow)

    enum CustomEvents {
        FullUpdateRequest = QEvent::User + 1,
        TriggerContextCreationFailure = QEvent::User + 2
    };

    static inline QQuickWindowPrivate *get(QQuickWindow *c) { return c->d_func(); }
    static inline const QQuickWindowPrivate *get(const QQuickWindow *c) { return c->d_func(); }

    QQuickWindowPrivate();
    ~QQuickWindowPrivate() override;

    void init(QQuickWindow *, QQuickRenderControl *control = nullptr);

    QQuickRootItem *contentItem;
    QSet<QQuickItem *> parentlessItems;
    QQmlListProperty<QObject> data();

    QQuickItem *activeFocusItem;

    void deliverKeyEvent(QKeyEvent *e);

    // Keeps track of the item currently receiving mouse events
#if QT_CONFIG(cursor)
    QQuickItem *cursorItem;
    QQuickPointerHandler *cursorHandler;
#endif
#if QT_CONFIG(quick_draganddrop)
    QQuickDragGrabber *dragGrabber;
#endif
    int touchMouseId;
    const QPointingDevice *touchMouseDevice;
    bool checkIfDoubleTapped(ulong newPressEventTimestamp, QPoint newPressPos);
    ulong touchMousePressTimestamp;
    QPoint touchMousePressPos;      // in screen coordiantes
    void cancelTouchMouseSynthesis();

    // Mouse positions are saved in widget coordinates
    QPointF lastMousePosition;
    bool deliverTouchAsMouse(QQuickItem *item, QQuickPointerEvent *pointerEvent);
    bool isDeliveringTouchAsMouse() const { return touchMouseId != -1 && touchMouseDevice; }
    void translateTouchEvent(QTouchEvent *touchEvent);
    void grabTouchPoints(QObject *grabber, const QVector<int> &ids);
    void removeGrabber(QQuickItem *grabber, bool mouse = true, bool touch = true);
    void sendUngrabEvent(QQuickItem *grabber, bool touch);
    static QMouseEvent *cloneMouseEvent(QMouseEvent *event, QPointF *transformedLocalPos = nullptr);
    void deliverToPassiveGrabbers(const QVector<QPointer <QQuickPointerHandler> > &passiveGrabbers, QQuickPointerEvent *pointerEvent);
    void deliverMouseEvent(QQuickPointerMouseEvent *pointerEvent);
    bool sendFilteredMouseEvent(QEvent *event, QQuickItem *receiver, QQuickItem *filteringParent);
    bool sendFilteredPointerEvent(QQuickPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent = nullptr);
    bool sendFilteredPointerEventImpl(QQuickPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent);
    bool deliverSinglePointEventUntilAccepted(QQuickPointerEvent *);

    // entry point of events to the window
    void handleTouchEvent(QTouchEvent *);
    void handleMouseEvent(QMouseEvent *);
    bool compressTouchEvent(QTouchEvent *);
    void flushFrameSynchronousEvents();
    void deliverDelayedTouchEvent();

    // the device-specific event instances which are reused during event delivery
    mutable QVector<QQuickPointerEvent *> pointerEventInstances;
    QQuickPointerEvent *queryPointerEventInstance(const QPointingDevice *device, QEvent::Type eventType = QEvent::None) const;
    QQuickPointerEvent *pointerEventInstance(const QPointingDevice *device, QEvent::Type eventType = QEvent::None) const;

    // delivery of pointer events:
    QQuickPointerEvent *pointerEventInstance(QEvent *ev) const;
    void deliverPointerEvent(QQuickPointerEvent *);
    void deliverTouchEvent(QQuickPointerTouchEvent *);
    bool deliverTouchCancelEvent(QTouchEvent *);
    bool deliverPressOrReleaseEvent(QQuickPointerEvent *, bool handlersOnly = false);
    void deliverUpdatedTouchPoints(QQuickPointerTouchEvent *event);
    void deliverMatchingPointsToItem(QQuickItem *item, QQuickPointerEvent *pointerEvent, bool handlersOnly = false);

    QVector<QQuickItem *> pointerTargets(QQuickItem *, QQuickEventPoint *point, bool checkMouseButtons, bool checkAcceptsTouch) const;
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
#if QT_CONFIG(cursor)
    void updateCursor(const QPointF &scenePos);
    QPair<QQuickItem*, QQuickPointerHandler*> findCursorItemAndHandler(QQuickItem *item, const QPointF &scenePos) const;
#endif

    QList<QQuickItem*> hoverItems;
    enum FocusOption {
        DontChangeFocusProperty = 0x01,
        DontChangeSubFocusItem  = 0x02
    };
    Q_DECLARE_FLAGS(FocusOptions, FocusOption)

    void setFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason, FocusOptions = { });
    void clearFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason, FocusOptions = { });
    static void notifyFocusChangesRecur(QQuickItem **item, int remaining);
    void clearFocusObject() override;

    void updateFocusItemTransform();

    void dirtyItem(QQuickItem *);
    void cleanup(QSGNode *);

    void ensureCustomRenderTarget();
    void setCustomCommandBuffer(QRhiCommandBuffer *cb);

    void polishItems();
    void forcePolish();
    void syncSceneGraph();
    void renderSceneGraph(const QSize &size, const QSize &surfaceSize = QSize());

    bool isRenderable() const;

    bool emitError(QQuickWindow::SceneGraphError error, const QString &msg);

    QQuickItem::UpdatePaintNodeData updatePaintNodeData;

    QQuickItem *dirtyItemList;
    QList<QSGNode *> cleanupNodeList;

    QVector<QQuickItem *> itemsToPolish;
    QVector<QQuickItem *> hasFiltered; // during event delivery to a single receiver, the filtering parents for which childMouseEventFilter was already called
    QVector<QQuickItem *> skipDelivery; // during delivery of one event to all receivers, Items to which we know delivery is no longer necessary

    qreal devicePixelRatio;
    QMetaObject::Connection physicalDpiChangedConnection;

    void updateDirtyNodes();
    void cleanupNodes();
    void cleanupNodesOnShutdown();
    bool updateEffectiveOpacity(QQuickItem *);
    void updateEffectiveOpacityRoot(QQuickItem *, qreal);
    void updateDirtyNode(QQuickItem *);

    void fireFrameSwapped() { Q_EMIT q_func()->frameSwapped(); }
    void fireAboutToStop() { Q_EMIT q_func()->sceneGraphAboutToStop(); }

    QSGRenderContext *context;
    QSGRenderer *renderer;
    QByteArray visualizationMode; // Default renderer supports "clip", "overdraw", "changes", "batches" and blank.

    QSGRenderLoop *windowManager;
    QQuickRenderControl *renderControl;
    QScopedPointer<QQuickAnimatorController> animationController;
    QScopedPointer<QTouchEvent> delayedTouch;

    int pointerEventRecursionGuard;

    QColor clearColor;

    uint persistentGraphics : 1;
    uint persistentSceneGraph : 1;

    uint lastWheelEventAccepted : 1;
    bool componentCompleted : 1;

    bool allowChildEventFiltering : 1;
    bool allowDoubleClick : 1;

    Qt::FocusReason lastFocusReason;

    // Storage for setRenderTarget(QQuickRenderTarget).
    // Gets baked into redirect.renderTarget by ensureCustomRenderTarget() when rendering the next frame.
    QQuickRenderTarget customRenderTarget;

    struct Redirect {
        QRhiCommandBuffer *commandBuffer = nullptr;
        QQuickWindowRenderTarget rt;
        bool renderTargetDirty = false;
    } redirect;

    QQuickGraphicsDevice customDeviceObjects;

    QQuickGraphicsConfiguration graphicsConfig;

    mutable QQuickWindowIncubationController *incubationController;

    static bool defaultAlphaBuffer;
    static QQuickWindow::TextRenderType textRenderType;

    static bool dragOverThreshold(qreal d, Qt::Axis axis, QMouseEvent *event, int startDragThreshold = -1);

    static bool dragOverThreshold(qreal d, Qt::Axis axis, const QEventPoint *tp, int startDragThreshold = -1);

    static bool dragOverThreshold(QVector2D delta);

    // data property
    static void data_append(QQmlListProperty<QObject> *, QObject *);
    static int data_count(QQmlListProperty<QObject> *);
    static QObject *data_at(QQmlListProperty<QObject> *, int);
    static void data_clear(QQmlListProperty<QObject> *);
    static void data_replace(QQmlListProperty<QObject> *, int, QObject *);
    static void data_removeLast(QQmlListProperty<QObject> *);

    static void rhiCreationFailureMessage(const QString &backendName,
                                          QString *translatedMessage,
                                          QString *untranslatedMessage);

    static void emitBeforeRenderPassRecording(void *ud);
    static void emitAfterRenderPassRecording(void *ud);

    QMutex renderJobMutex;
    QList<QRunnable *> beforeSynchronizingJobs;
    QList<QRunnable *> afterSynchronizingJobs;
    QList<QRunnable *> beforeRenderingJobs;
    QList<QRunnable *> afterRenderingJobs;
    QList<QRunnable *> afterSwapJobs;

    void runAndClearJobs(QList<QRunnable *> *jobs);
    QOpenGLContext *openglContext();

    QQuickWindow::GraphicsStateInfo rhiStateInfo;
    QRhi *rhi = nullptr;
    QRhiSwapChain *swapchain = nullptr;
    QRhiRenderBuffer *depthStencilForSwapchain = nullptr;
    QRhiRenderPassDescriptor *rpDescForSwapchain = nullptr;
    uint hasActiveSwapchain : 1;
    uint hasRenderableSwapchain : 1;
    uint swapchainJustBecameRenderable : 1;

private:
    static void cleanupNodesOnShutdown(QQuickItem *);
};

class QQuickWindowQObjectCleanupJob : public QRunnable
{
public:
    QQuickWindowQObjectCleanupJob(QObject *o) : object(o) { }
    void run() override { delete object; }
    QObject *object;
    static void schedule(QQuickWindow *window, QObject *object) {
        Q_ASSERT(window);
        Q_ASSERT(object);
        window->scheduleRenderJob(new QQuickWindowQObjectCleanupJob(object), QQuickWindow::AfterSynchronizingStage);
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickWindowPrivate::FocusOptions)

QT_END_NAMESPACE

#endif // QQUICKWINDOW_P_H
