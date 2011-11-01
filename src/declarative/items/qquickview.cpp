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

#include "qquickview.h"
#include "qquickview_p.h"

#include "qquickcanvas_p.h"
#include "qquickitem_p.h"
#include "qquickitemchangelistener_p.h"

#include <private/qdeclarativedebugtrace_p.h>
#include <private/qdeclarativeinspectorservice_p.h>

#include <QtDeclarative/qdeclarativeengine.h>
#include <private/qdeclarativeengine_p.h>
#include <QtCore/qbasictimer.h>


// XXX todo - This whole class should probably be merged with QDeclarativeView for
// maximum seamlessness
QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(frameRateDebug, QML_SHOW_FRAMERATE)

void QQuickViewPrivate::init()
{
    Q_Q(QQuickView);

    QDeclarativeEnginePrivate::get(&engine)->sgContext = QQuickCanvasPrivate::context;

    engine.setIncubationController(q->incubationController());

    if (QDeclarativeDebugService::isDebuggingEnabled())
        QDeclarativeInspectorService::instance()->addView(q);
}

QQuickViewPrivate::QQuickViewPrivate()
    : root(0), component(0), resizeMode(QQuickView::SizeViewToRootObject), initialSize(0,0)
{
}

QQuickViewPrivate::~QQuickViewPrivate()
{
    if (QDeclarativeDebugService::isDebuggingEnabled())
        QDeclarativeInspectorService::instance()->removeView(q_func());

    delete root;
}

void QQuickViewPrivate::execute()
{
    Q_Q(QQuickView);
    if (root) {
        delete root;
        root = 0;
    }
    if (component) {
        delete component;
        component = 0;
    }
    if (!source.isEmpty()) {
        component = new QDeclarativeComponent(&engine, source, q);
        if (!component->isLoading()) {
            q->continueExecute();
        } else {
            QObject::connect(component, SIGNAL(statusChanged(QDeclarativeComponent::Status)),
                             q, SLOT(continueExecute()));
        }
    }
}

void QQuickViewPrivate::itemGeometryChanged(QQuickItem *resizeItem, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_Q(QQuickView);
    if (resizeItem == root && resizeMode == QQuickView::SizeViewToRootObject) {
        // wait for both width and height to be changed
        resizetimer.start(0,q);
    }
    QQuickItemChangeListener::itemGeometryChanged(resizeItem, newGeometry, oldGeometry);
}

QQuickView::QQuickView(QWindow *parent, Qt::WindowFlags f)
: QQuickCanvas(*(new QQuickViewPrivate), parent)
{
    setWindowFlags(f);
    d_func()->init();
}

QQuickView::QQuickView(const QUrl &source, QWindow *parent, Qt::WindowFlags f)
: QQuickCanvas(*(new QQuickViewPrivate), parent)
{
    setWindowFlags(f);
    d_func()->init();
    setSource(source);
}

QQuickView::~QQuickView()
{
}

void QQuickView::setSource(const QUrl& url)
{
    Q_D(QQuickView);
    d->source = url;
    d->execute();
}

QUrl QQuickView::source() const
{
    Q_D(const QQuickView);
    return d->source;
}

QDeclarativeEngine* QQuickView::engine() const
{
    Q_D(const QQuickView);
    return const_cast<QDeclarativeEngine *>(&d->engine);
}

QDeclarativeContext* QQuickView::rootContext() const
{
    Q_D(const QQuickView);
    return d->engine.rootContext();
}

QQuickView::Status QQuickView::status() const
{
    Q_D(const QQuickView);
    if (!d->component)
        return QQuickView::Null;

    return QQuickView::Status(d->component->status());
}

QList<QDeclarativeError> QQuickView::errors() const
{
    Q_D(const QQuickView);
    if (d->component)
        return d->component->errors();
    return QList<QDeclarativeError>();
}

void QQuickView::setResizeMode(ResizeMode mode)
{
    Q_D(QQuickView);
    if (d->resizeMode == mode)
        return;

    if (d->root) {
        if (d->resizeMode == SizeViewToRootObject) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(d->root);
            p->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
        }
    }

    d->resizeMode = mode;
    if (d->root) {
        d->initResize();
    }
}

void QQuickViewPrivate::initResize()
{
    if (root) {
        if (resizeMode == QQuickView::SizeViewToRootObject) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(root);
            p->addItemChangeListener(this, QQuickItemPrivate::Geometry);
        }
    }
    updateSize();
}

void QQuickViewPrivate::updateSize()
{
    Q_Q(QQuickView);
    if (!root)
        return;

    if (resizeMode == QQuickView::SizeViewToRootObject) {
        QSize newSize = QSize(root->width(), root->height());
        if (newSize.isValid() && newSize != q->size()) {
            q->resize(newSize);
        }
    } else if (resizeMode == QQuickView::SizeRootObjectToView) {
        if (!qFuzzyCompare(q->width(), root->width()))
            root->setWidth(q->width());
        if (!qFuzzyCompare(q->height(), root->height()))
            root->setHeight(q->height());
    }
}

QSize QQuickViewPrivate::rootObjectSize() const
{
    QSize rootObjectSize(0,0);
    int widthCandidate = -1;
    int heightCandidate = -1;
    if (root) {
        widthCandidate = root->width();
        heightCandidate = root->height();
    }
    if (widthCandidate > 0) {
        rootObjectSize.setWidth(widthCandidate);
    }
    if (heightCandidate > 0) {
        rootObjectSize.setHeight(heightCandidate);
    }
    return rootObjectSize;
}

QQuickView::ResizeMode QQuickView::resizeMode() const
{
    Q_D(const QQuickView);
    return d->resizeMode;
}

/*!
  \internal
 */
void QQuickView::continueExecute()
{
    Q_D(QQuickView);
    disconnect(d->component, SIGNAL(statusChanged(QDeclarativeComponent::Status)), this, SLOT(continueExecute()));

    if (d->component->isError()) {
        QList<QDeclarativeError> errorList = d->component->errors();
        foreach (const QDeclarativeError &error, errorList) {
            qWarning() << error;
        }
        emit statusChanged(status());
        return;
    }

    QObject *obj = d->component->create();

    if (d->component->isError()) {
        QList<QDeclarativeError> errorList = d->component->errors();
        foreach (const QDeclarativeError &error, errorList) {
            qWarning() << error;
        }
        emit statusChanged(status());
        return;
    }

    d->setRootObject(obj);
    emit statusChanged(status());
}


/*!
  \internal
*/
void QQuickViewPrivate::setRootObject(QObject *obj)
{
    Q_Q(QQuickView);
    if (root == obj)
        return;
    if (QQuickItem *sgItem = qobject_cast<QQuickItem *>(obj)) {
        root = sgItem;
        sgItem->setParentItem(q->QQuickCanvas::rootItem());
    } else {
        qWarning() << "QQuickView only supports loading of root objects that derive from QQuickItem." << endl
                   << endl
                   << "If your example is using QML 2, (such as qmlscene) and the .qml file you" << endl
                   << "loaded has 'import QtQuick 1.0' or 'import Qt 4.7', this error will occur." << endl
                   << endl
                   << "To load files with 'import QtQuick 1.0' with QML 2, specify:" << endl
                   << "  QMLSCENE_IMPORT_NAME=quick1" << endl
                   << "on as an environment variable prior to launching the application." << endl
                   << endl
                   << "To load files with 'import Qt 4.7' with QML 2, specify:" << endl
                   << "  QMLSCENE_IMPORT_NAME=qt" << endl
                   << "on as an environment variable prior to launching the application." << endl;
        delete obj;
        root = 0;
    }
    if (root) {
        initialSize = rootObjectSize();
        if ((resizeMode == QQuickView::SizeViewToRootObject || !q->width() || !q->height())
                && initialSize != q->size()) {
            q->resize(initialSize);
        }
        initResize();
    }
}

/*!
  \internal
  If the \l {QTimerEvent} {timer event} \a e is this
  view's resize timer, sceneResized() is emitted.
 */
void QQuickView::timerEvent(QTimerEvent* e)
{
    Q_D(QQuickView);
    if (!e || e->timerId() == d->resizetimer.timerId()) {
        d->updateSize();
        d->resizetimer.stop();
    }
}

/*!
    \internal
    Preferred size follows the root object geometry.
*/
QSize QQuickView::sizeHint() const
{
    Q_D(const QQuickView);
    QSize rootObjectSize = d->rootObjectSize();
    if (rootObjectSize.isEmpty()) {
        return size();
    } else {
        return rootObjectSize;
    }
}

QSize QQuickView::initialSize() const
{
    Q_D(const QQuickView);
    return d->initialSize;
}

QQuickItem *QQuickView::rootObject() const
{
    Q_D(const QQuickView);
    return d->root;
}

/*!
  \internal
  This function handles the \l {QResizeEvent} {resize event}
  \a e.
 */
void QQuickView::resizeEvent(QResizeEvent *e)
{
    Q_D(QQuickView);
    if (d->resizeMode == SizeRootObjectToView)
        d->updateSize();

    QQuickCanvas::resizeEvent(e);
}

void QQuickView::keyPressEvent(QKeyEvent *e)
{
    QDeclarativeDebugTrace::addEvent(QDeclarativeDebugTrace::Key);

    QQuickCanvas::keyPressEvent(e);
}

void QQuickView::keyReleaseEvent(QKeyEvent *e)
{
    QDeclarativeDebugTrace::addEvent(QDeclarativeDebugTrace::Key);

    QQuickCanvas::keyReleaseEvent(e);
}

void QQuickView::mouseMoveEvent(QMouseEvent *e)
{
    QDeclarativeDebugTrace::addEvent(QDeclarativeDebugTrace::Mouse);

    QQuickCanvas::mouseMoveEvent(e);
}

void QQuickView::mousePressEvent(QMouseEvent *e)
{
    QDeclarativeDebugTrace::addEvent(QDeclarativeDebugTrace::Mouse);

    QQuickCanvas::mousePressEvent(e);
}

void QQuickView::mouseReleaseEvent(QMouseEvent *e)
{
    QDeclarativeDebugTrace::addEvent(QDeclarativeDebugTrace::Mouse);

    QQuickCanvas::mouseReleaseEvent(e);
}


QT_END_NAMESPACE
