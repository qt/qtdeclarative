/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtQuick/private/qquickdeliveryagent_p_p.h>
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
#include <QtCore/qstack.h>

#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qpointingdevice_p.h>
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

    void updateChildrenPalettes(const QPalette &parentPalette) override;

    void init(QQuickWindow *, QQuickRenderControl *control = nullptr);

    QQuickRootItem *contentItem;
    QSet<QQuickItem *> parentlessItems;
    QQmlListProperty<QObject> data();

    // primary delivery agent for the whole scene, used by default for events that arrive in this window;
    // but any subscene root can have a QQuickItemPrivate::ExtraData::subsceneDeliveryAgent
    QQuickDeliveryAgent *deliveryAgent = nullptr;
    QQuickDeliveryAgentPrivate *deliveryAgentPrivate() const
    { return static_cast<QQuickDeliveryAgentPrivate *>(QQuickDeliveryAgentPrivate::get(deliveryAgent)); }

    // TODO remove these: they were moved to QQuickDeliveryAgentPrivate
    static bool isMouseEvent(const QPointerEvent *ev) { return QQuickDeliveryAgentPrivate::isMouseEvent(ev); }
    static bool isTouchEvent(const QPointerEvent *ev) { return QQuickDeliveryAgentPrivate::isTouchEvent(ev); }
    static bool isTabletEvent(const QPointerEvent *ev) { return QQuickDeliveryAgentPrivate::isTabletEvent(ev); }
    void flushFrameSynchronousEvents();

#if QT_CONFIG(cursor)
    QQuickItem *cursorItem = nullptr;
    QQuickPointerHandler *cursorHandler = nullptr;
    void updateCursor(const QPointF &scenePos, QQuickItem *rootItem = nullptr);
    QPair<QQuickItem*, QQuickPointerHandler*> findCursorItemAndHandler(QQuickItem *item, const QPointF &scenePos) const;
#endif

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

    enum TextureFromNativeTextureFlag {
        NativeTextureIsExternalOES = 0x01
    };
    Q_DECLARE_FLAGS(TextureFromNativeTextureFlags, TextureFromNativeTextureFlag)

    QSGTexture *createTextureFromNativeTexture(quint64 nativeObjectHandle,
                                               int nativeLayout,
                                               const QSize &size,
                                               QQuickWindow::CreateTextureOptions options,
                                               TextureFromNativeTextureFlags flags = {}) const;

    QQuickItem::UpdatePaintNodeData updatePaintNodeData;

    QQuickItem *dirtyItemList;
    QList<QSGNode *> cleanupNodeList;

    QVector<QQuickItem *> itemsToPolish;

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

    QColor clearColor;

    uint persistentGraphics : 1;
    uint persistentSceneGraph : 1;
    uint componentCompleted : 1;
    uint inDestructor : 1;

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

    // vvv currently in use in Controls 2; TODO remove
    static bool dragOverThreshold(qreal d, Qt::Axis axis, const QEventPoint *tp, int startDragThreshold = -1)
    { return QQuickDeliveryAgentPrivate::dragOverThreshold(d, axis, *tp, startDragThreshold); }
    static bool dragOverThreshold(qreal d, Qt::Axis axis, QMouseEvent *event, int startDragThreshold = -1)
    { return QQuickDeliveryAgentPrivate::dragOverThreshold(d, axis, event, startDragThreshold); }
    void clearFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason)
    { deliveryAgentPrivate()->clearFocusInScope(scope, item, reason); }
    void handleTouchEvent(QTouchEvent *e)
    { deliveryAgentPrivate()->handleTouchEvent(e); }
    void handleMouseEvent(QMouseEvent *e)
    { deliveryAgentPrivate()->handleMouseEvent(e); }
    // ^^^ currently in use in Controls 2; TODO remove

    // data property
    static void data_append(QQmlListProperty<QObject> *, QObject *);
    static qsizetype data_count(QQmlListProperty<QObject> *);
    static QObject *data_at(QQmlListProperty<QObject> *, qsizetype);
    static void data_clear(QQmlListProperty<QObject> *);
    static void data_replace(QQmlListProperty<QObject> *, qsizetype, QObject *);
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

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickWindowPrivate::TextureFromNativeTextureFlags)

QT_END_NAMESPACE

#endif // QQUICKWINDOW_P_H
