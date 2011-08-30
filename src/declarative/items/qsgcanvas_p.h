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

#ifndef QSGCANVAS_P_H
#define QSGCANVAS_P_H

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

#include "qsgitem.h"
#include "qsgcanvas.h"
#include "qsgevent.h"
#include <private/qdeclarativeguard_p.h>

#include <private/qsgcontext_p.h>

#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <private/qwindow_p.h>
#include <private/qopengl_p.h>
#include <qopenglcontext.h>
#include <QtGui/qopenglframebufferobject.h>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

//Make it easy to identify and customize the root item if needed
class QSGRootItem : public QSGItem
{
    Q_OBJECT
public:
    QSGRootItem();
};

class QSGCanvasPrivate;

class QTouchEvent;
class QSGCanvasRenderLoop;

class QSGCanvasPrivate : public QWindowPrivate
{
public:
    Q_DECLARE_PUBLIC(QSGCanvas)

    static inline QSGCanvasPrivate *get(QSGCanvas *c) { return c->d_func(); }

    QSGCanvasPrivate();
    virtual ~QSGCanvasPrivate();

    void init(QSGCanvas *);

    QSGRootItem *rootItem;

    QSGItem *activeFocusItem;
    QSGItem *mouseGrabberItem;

    // Mouse positions are saved in widget coordinates
    QPointF lastMousePosition;
    void translateTouchEvent(QTouchEvent *touchEvent);
    static void transformTouchPoints(QList<QTouchEvent::TouchPoint> &touchPoints, const QTransform &transform);
    bool deliverInitialMousePressEvent(QSGItem *, QMouseEvent *);
    bool deliverMouseEvent(QMouseEvent *);
    bool sendFilteredMouseEvent(QSGItem *, QSGItem *, QMouseEvent *);
    bool deliverWheelEvent(QSGItem *, QWheelEvent *);
    bool deliverTouchPoints(QSGItem *, QTouchEvent *, const QList<QTouchEvent::TouchPoint> &, QSet<int> *,
            QHash<QSGItem *, QList<QTouchEvent::TouchPoint> > *);
    bool deliverTouchEvent(QTouchEvent *);
    bool deliverHoverEvent(QSGItem *, const QPointF &scenePos, const QPointF &lastScenePos, Qt::KeyboardModifiers modifiers, bool &accepted);
    bool sendHoverEvent(QEvent::Type, QSGItem *, const QPointF &scenePos, const QPointF &lastScenePos,
                        Qt::KeyboardModifiers modifiers, bool accepted);
    bool clearHover();
    void deliverDragEvent(QSGDragEvent *);
    bool deliverDragEvent(QSGItem *item, QSGDragEvent *);

    QList<QSGItem*> hoverItems;
    enum FocusOption {
        DontChangeFocusProperty = 0x01,
    };
    Q_DECLARE_FLAGS(FocusOptions, FocusOption)

    void setFocusInScope(QSGItem *scope, QSGItem *item, FocusOptions = 0);
    void clearFocusInScope(QSGItem *scope, QSGItem *item, FocusOptions = 0);
    void notifyFocusChangesRecur(QSGItem **item, int remaining);

    void updateInputMethodData();

    void dirtyItem(QSGItem *);
    void cleanup(QSGNode *);

    void initializeSceneGraph();
    void polishItems();
    void syncSceneGraph();
    void renderSceneGraph(const QSize &size);

    void updateInputContext();
    void resetInputContext();

    QSGItem::UpdatePaintNodeData updatePaintNodeData;

    QSGItem *dirtyItemList;
    QList<QSGNode *> cleanupNodeList;

    QSet<QSGItem *> itemsToPolish;

    void updateDirtyNodes();
    void cleanupNodes();
    bool updateEffectiveOpacity(QSGItem *);
    void updateEffectiveOpacityRoot(QSGItem *, qreal);
    void updateDirtyNode(QSGItem *);

    QSGContext *context;

    uint vsyncAnimations : 1;

    QSGCanvasRenderLoop *thread;
    QSize widgetSize;
    QSize viewportSize;

    QAnimationDriver *animationDriver;

    QOpenGLFramebufferObject *renderTarget;

    QHash<int, QSGItem *> itemForTouchPointId;
};

class QSGCanvasRenderLoop
{
public:
    QSGCanvasRenderLoop()
        : d(0)
        , renderer(0)
        , gl(0)
    {
    }
    virtual ~QSGCanvasRenderLoop()
    {
        delete gl;
    }

    friend class QSGCanvasPrivate;

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
    QSGCanvasPrivate *d;
    QSGCanvas *renderer;

    QOpenGLContext *gl;
};

class QSGCanvasRenderThread : public QThread, public QSGCanvasRenderLoop
{
    Q_OBJECT
public:
    QSGCanvasRenderThread()
        : mutex(QMutex::NonRecursive)
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

    bool event(QEvent *);

    QImage grab();

public slots:
    void animationStarted();
    void animationStopped();

public:
    QMutex mutex;
    QWaitCondition condition;

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


Q_DECLARE_OPERATORS_FOR_FLAGS(QSGCanvasPrivate::FocusOptions)

QT_END_NAMESPACE

#endif // QSGCANVAS_P_H
