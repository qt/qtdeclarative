/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QQUICKCANVAS_P_H
#define QQUICKCANVAS_P_H

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

#include "qquickitem.h"
#include "qquickcanvas.h"
#include <private/qdeclarativeguard_p.h>

#include <private/qsgcontext_p.h>
#include <private/qquickdrag_p.h>

#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <private/qwindow_p.h>
#include <private/qopengl_p.h>
#include <qopenglcontext.h>
#include <QtGui/qopenglframebufferobject.h>
#include <QtGui/qevent.h>
#include <QtGui/qinputpanel.h>

QT_BEGIN_NAMESPACE

//Make it easy to identify and customize the root item if needed
class QQuickRootItem : public QQuickItem
{
    Q_OBJECT
public:
    QQuickRootItem();
};

class QQuickCanvasPrivate;

class QTouchEvent;
class QQuickCanvasRenderLoop;
class QQuickCanvasIncubationController;

class QQuickCanvasPrivate : public QWindowPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickCanvas)

    static inline QQuickCanvasPrivate *get(QQuickCanvas *c) { return c->d_func(); }

    QQuickCanvasPrivate();
    virtual ~QQuickCanvasPrivate();

    void init(QQuickCanvas *);

    QQuickRootItem *rootItem;

    QQuickItem *activeFocusItem;
    QQuickItem *mouseGrabberItem;
    QQuickDragGrabber dragGrabber;

    // Mouse positions are saved in widget coordinates
    QPointF lastMousePosition;
    void translateTouchEvent(QTouchEvent *touchEvent);
    static void transformTouchPoints(QList<QTouchEvent::TouchPoint> &touchPoints, const QTransform &transform);
    bool deliverInitialMousePressEvent(QQuickItem *, QMouseEvent *);
    bool deliverMouseEvent(QMouseEvent *);
    bool sendFilteredMouseEvent(QQuickItem *, QQuickItem *, QMouseEvent *);
    bool deliverWheelEvent(QQuickItem *, QWheelEvent *);
    bool deliverTouchPoints(QQuickItem *, QTouchEvent *, const QList<QTouchEvent::TouchPoint> &, QSet<int> *,
            QHash<QQuickItem *, QList<QTouchEvent::TouchPoint> > *);
    bool deliverTouchEvent(QTouchEvent *);
    bool deliverHoverEvent(QQuickItem *, const QPointF &scenePos, const QPointF &lastScenePos, Qt::KeyboardModifiers modifiers, bool &accepted);
    bool sendHoverEvent(QEvent::Type, QQuickItem *, const QPointF &scenePos, const QPointF &lastScenePos,
                        Qt::KeyboardModifiers modifiers, bool accepted);
    bool clearHover();
    void deliverDragEvent(QQuickDragGrabber *, QEvent *);
    bool deliverDragEvent(QQuickDragGrabber *, QQuickItem *, QDragMoveEvent *);

    QList<QQuickItem*> hoverItems;
    enum FocusOption {
        DontChangeFocusProperty = 0x01,
    };
    Q_DECLARE_FLAGS(FocusOptions, FocusOption)

    void setFocusInScope(QQuickItem *scope, QQuickItem *item, FocusOptions = 0);
    void clearFocusInScope(QQuickItem *scope, QQuickItem *item, FocusOptions = 0);
    void notifyFocusChangesRecur(QQuickItem **item, int remaining);

    void updateInputMethodData();
    void updateFocusItemTransform();

    void dirtyItem(QQuickItem *);
    void cleanup(QSGNode *);

    void initializeSceneGraph();
    void polishItems();
    void syncSceneGraph();
    void renderSceneGraph(const QSize &size);

    QQuickItem::UpdatePaintNodeData updatePaintNodeData;

    QQuickItem *dirtyItemList;
    QList<QSGNode *> cleanupNodeList;

    QSet<QQuickItem *> itemsToPolish;

    void updateDirtyNodes();
    void cleanupNodes();
    bool updateEffectiveOpacity(QQuickItem *);
    void updateEffectiveOpacityRoot(QQuickItem *, qreal);
    void updateDirtyNode(QQuickItem *);

    QSGContext *context;

    uint vsyncAnimations : 1;

    QQuickCanvasRenderLoop *thread;
    QSize widgetSize;
    QSize viewportSize;

    QAnimationDriver *animationDriver;

    QOpenGLFramebufferObject *renderTarget;

    QHash<int, QQuickItem *> itemForTouchPointId;

    mutable QQuickCanvasIncubationController *incubationController;
};

class QQuickCanvasRenderLoop
{
public:
    QQuickCanvasRenderLoop()
        : d(0)
        , renderer(0)
        , gl(0)
    {
    }
    virtual ~QQuickCanvasRenderLoop()
    {
        delete gl;
    }

    friend class QQuickCanvasPrivate;

    virtual void paint() = 0;
    virtual void resize(const QSize &size) = 0;
    virtual void startRendering() = 0;
    virtual void stopRendering() = 0;
    virtual QImage grab() = 0;
    virtual void setWindowSize(const QSize &size) = 0;
    virtual void maybeUpdate() = 0;
    virtual bool isRunning() const = 0;
    virtual void animationStarted() = 0;
    virtual void animationStopped() = 0;
    virtual void moveContextToThread(QSGContext *) { }
    virtual bool *allowMainThreadProcessing() { return 0; }

protected:
    void initializeSceneGraph() { d->initializeSceneGraph(); }
    void syncSceneGraph() { d->syncSceneGraph(); }
    void renderSceneGraph(const QSize &size) { d->renderSceneGraph(size); }
    void polishItems() { d->polishItems(); }
    QAnimationDriver *animationDriver() const { return d->animationDriver; }

    inline QOpenGLContext *glContext() const { return gl; }
    void createGLContext();
    void makeCurrent() { gl->makeCurrent(renderer); }
    void doneCurrent() { gl->doneCurrent(); }
    void swapBuffers() {
        gl->swapBuffers(renderer);
        emit renderer->frameSwapped();
    }

private:
    QQuickCanvasPrivate *d;
    QQuickCanvas *renderer;

    QOpenGLContext *gl;
};

class QQuickCanvasRenderThread : public QThread, public QQuickCanvasRenderLoop
{
    Q_OBJECT
public:
    QQuickCanvasRenderThread()
        : mutex(QMutex::NonRecursive)
        , allowMainThreadProcessingFlag(true)
        , animationRunning(false)
        , isGuiBlocked(0)
        , isPaintCompleted(false)
        , isGuiBlockPending(false)
        , isRenderBlocked(false)
        , isExternalUpdatePending(false)
        , syncAlreadyHappened(false)
        , inSync(false)
        , doGrab(false)
        , shouldExit(false)
        , hasExited(false)
        , renderThreadAwakened(false)
    {}

    inline void lock() { mutex.lock(); }
    inline void unlock() { mutex.unlock(); }
    inline void wait() { condition.wait(&mutex); }
    inline void wake() { condition.wakeOne(); }

    void lockInGui();
    void unlockInGui();

    void paint();
    void resize(const QSize &size);
    void startRendering();
    void stopRendering();
    void exhaustSyncEvent();
    void sync(bool guiAlreadyLocked);
    bool isRunning() const { return QThread::isRunning(); }
    void setWindowSize(const QSize &size) { windowSize = size; }
    void maybeUpdate();
    void moveContextToThread(QSGContext *c) { c->moveToThread(this); }
    bool *allowMainThreadProcessing() { return &allowMainThreadProcessingFlag; }

    bool event(QEvent *);

    QImage grab();

public slots:
    void animationStarted();
    void animationStopped();

public:
    QMutex mutex;
    QWaitCondition condition;

    bool allowMainThreadProcessingFlag;

    QSize windowSize;
    QSize renderedSize;

    uint animationRunning: 1;
    int isGuiBlocked;
    uint isPaintCompleted : 1;
    uint isGuiBlockPending : 1;
    uint isRenderBlocked : 1;
    uint isExternalUpdatePending : 1;
    uint syncAlreadyHappened : 1;
    uint inSync : 1;
    uint doGrab : 1;
    uint shouldExit : 1;
    uint hasExited : 1;
    uint renderThreadAwakened : 1;

    QImage grabContent;

    void run();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickCanvasPrivate::FocusOptions)

QT_END_NAMESPACE

#endif // QQUICKCANVAS_P_H
