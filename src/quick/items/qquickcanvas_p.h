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

#include <QtQuick/private/qsgcontext_p.h>
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

class QQuickWindowManager;

class QQuickRootItem : public QQuickItem
{
    Q_OBJECT
public:
    QQuickRootItem();
public Q_SLOTS:
    void setWidth(int w) {QQuickItem::setWidth(qreal(w));}
    void setHeight(int h) {QQuickItem::setHeight(qreal(h));}
};

class QQuickItemPrivate;
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
    void initRootItem();//Currently only used if items added in QML

    QQuickRootItem *rootItem;
    QSet<QQuickItem *> parentlessItems;
    QDeclarativeListProperty<QObject> data();

    QQuickItem *activeFocusItem;
    QQuickItem *mouseGrabberItem;
    QQuickDragGrabber dragGrabber;

    // Mouse positions are saved in widget coordinates
    QPointF lastMousePosition;
    void translateTouchEvent(QTouchEvent *touchEvent);
    static void transformTouchPoints(QList<QTouchEvent::TouchPoint> &touchPoints, const QTransform &transform);
    bool deliverInitialMousePressEvent(QQuickItem *, QMouseEvent *);
    bool deliverMouseEvent(QMouseEvent *);
    bool sendFilteredMouseEvent(QQuickItem *, QQuickItem *, QEvent *);
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

    void polishItems();
    void syncSceneGraph();
    void renderSceneGraph(const QSize &size);

    QQuickItem::UpdatePaintNodeData updatePaintNodeData;

    QQuickItem *dirtyItemList;
    QList<QSGNode *> cleanupNodeList;

    QSet<QQuickItem *> itemsToPolish;

    void updateDirtyNodes();
    void cleanupNodes();
    void cleanupNodesOnShutdown();
    bool updateEffectiveOpacity(QQuickItem *);
    void updateEffectiveOpacityRoot(QQuickItem *, qreal);
    void updateDirtyNode(QQuickItem *);

    void fireFrameSwapped() { emit q_func()->frameSwapped(); }

    QSGEngine *engine;
    QSGContext *context;
    QSGRenderer *renderer;

    QQuickWindowManager *windowManager;

    QColor clearColor;

    uint clearBeforeRendering : 1;

    QOpenGLFramebufferObject *renderTarget;

    QHash<int, QQuickItem *> itemForTouchPointId;

    mutable QQuickCanvasIncubationController *incubationController;

private:
    static void cleanupNodesOnShutdown(QQuickItem *);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickCanvasPrivate::FocusOptions)

QT_END_NAMESPACE

#endif // QQUICKCANVAS_P_H
