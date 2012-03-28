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

#include "zoomtool.h"
#include "qquickviewinspector.h"

#include <QtCore/QLineF>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QTouchEvent>
#include <QtGui/QKeyEvent>

#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>

namespace QmlJSDebugger {
namespace QtQuick2 {

ZoomTool::ZoomTool(QQuickViewInspector *inspector, QQuickView *view) :
    AbstractTool(inspector),
    m_dragStarted(false),
    m_pinchStarted(false),
    m_currentScale(1.0f),
    m_smoothScaleFactor(0.05f),
    m_minScale(0.125f),
    m_maxScale(48.0f),
    m_tapScaleCounter(0)
{
    m_rootItem = view->rootItem();
    m_originalSmooth = m_rootItem->smooth();
    if (!m_originalSmooth)
        m_rootItem->setSmooth(true);
    m_originalPosition = m_rootItem->pos();
    m_originalScale = m_rootItem->scale();
}

ZoomTool::~ZoomTool()
{
    // restoring the original states.
    if (m_rootItem) {
        m_rootItem->setScale(m_originalScale);
        m_rootItem->setPos(m_originalPosition);
        if (!m_originalSmooth)
            m_rootItem->setSmooth(m_originalSmooth);
    }
}

void ZoomTool::mousePressEvent(QMouseEvent *event)
{
    m_mousePosition = event->posF();
    if (event->buttons() & Qt::LeftButton) {
        m_dragStartPosition = event->posF();
        m_dragStarted = false;
    }
}

void ZoomTool::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pinchStarted)
        return;

    m_mousePosition = event->posF();
    if (!m_dragStarted
            && event->buttons() & Qt::LeftButton
            && ((m_dragStartPosition - event->posF()).manhattanLength()
                > Constants::DragStartDistance)) {
        m_dragStarted = true;
    }
    if (m_dragStarted) {
        m_adjustedOrigin += event->posF() - m_dragStartPosition;
        m_dragStartPosition = event->posF();
        m_rootItem->setPos(m_adjustedOrigin);
    }
}

void ZoomTool::hoverMoveEvent(QMouseEvent *event)
{
    m_mousePosition = event->posF();
}

void ZoomTool::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() != Qt::Vertical)
        return;

    Qt::KeyboardModifier smoothZoomModifier = Qt::ControlModifier;
    if (event->modifiers() & smoothZoomModifier) {
        int numDegrees = event->delta() / 8;
        qreal newScale = m_currentScale + m_smoothScaleFactor * (numDegrees / 15.0f);
        scaleView(newScale / m_currentScale, m_mousePosition, m_mousePosition);
    } else if (!event->modifiers()) {
        if (event->delta() > 0) {
            zoomIn();
        } else if (event->delta() < 0) {
            zoomOut();
        }
    }
}

void ZoomTool::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_mousePosition = event->posF();
    zoomTo100();
}

void ZoomTool::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9: {
        qreal newScale = ((event->key() - Qt::Key_0) * 1.0f);
        scaleView(newScale / m_currentScale, m_mousePosition, m_mousePosition);
        break;
    }
    default:
        break;
    }
}

void ZoomTool::touchEvent(QTouchEvent *event)
{
    QList<QTouchEvent::TouchPoint> touchPoints = event->touchPoints();

    switch (event->type()) {
    case QEvent::TouchBegin:
        // fall through..
    case QEvent::TouchUpdate: {
        if ((touchPoints.count() == 2)
                && (!(event->touchPointStates() & Qt::TouchPointReleased))) {
            // determine scale factor
            const QTouchEvent::TouchPoint &touchPoint0 = touchPoints.first();
            const QTouchEvent::TouchPoint &touchPoint1 = touchPoints.last();

            qreal touchScaleFactor =
                    QLineF(touchPoint0.pos(), touchPoint1.pos()).length()
                    / QLineF(touchPoint0.lastPos(), touchPoint1.lastPos()).length();

            QPointF oldcenter = (touchPoint0.lastPos() + touchPoint1.lastPos()) / 2;
            QPointF newcenter = (touchPoint0.pos() + touchPoint1.pos()) / 2;

            m_pinchStarted = true;
            m_tapScaleCounter = 0;
            scaleView(touchScaleFactor, newcenter, oldcenter);
        }
        break;
    }
    case QEvent::TouchEnd: {
        if (m_pinchStarted) {
            m_pinchStarted = false;
        } else if ((touchPoints.count() == 1)
                   &&(!m_dragStarted)) {
            ++m_tapScaleCounter;
            qreal factor = 1.0f + (1.0f / (m_tapScaleCounter + 1));
            scaleView(factor, touchPoints.first().pos(),
                      touchPoints.first().pos());
        }
        break;
    }
    default:
        break;
    }
}

void ZoomTool::scaleView(const qreal &factor, const QPointF &newcenter, const QPointF &oldcenter)
{
    if (((m_currentScale * factor) > m_maxScale)
            || ((m_currentScale * factor) < m_minScale)) {
        return;
    }
    //New position = new center + scalefactor * (oldposition - oldcenter)
    m_adjustedOrigin = newcenter + (factor * (m_adjustedOrigin - oldcenter));
    m_currentScale *= factor;

    m_rootItem->setScale(m_currentScale);
    m_rootItem->setPos(m_adjustedOrigin);
}

void ZoomTool::zoomIn()
{
    qreal newScale = nextZoomScale(ZoomIn);
    scaleView(newScale / m_currentScale, m_mousePosition, m_mousePosition);
}

void ZoomTool::zoomOut()
{
    qreal newScale = nextZoomScale(ZoomOut);
    scaleView(newScale / m_currentScale, m_mousePosition, m_mousePosition);
}

void ZoomTool::zoomTo100()
{
    m_currentScale = 1.0;
    m_adjustedOrigin = QPointF(0, 0);
    m_tapScaleCounter = 0;

    m_rootItem->setPos(m_adjustedOrigin);
    m_rootItem->setScale(m_currentScale);
}

qreal ZoomTool::nextZoomScale(ZoomDirection direction)
{
    static QList<qreal> zoomScales =
            QList<qreal>()
            << 0.125f
            << 1.0f / 6.0f
            << 0.25f
            << 1.0f / 3.0f
            << 0.5f
            << 2.0f / 3.0f
            << 1.0f
            << 2.0f
            << 3.0f
            << 4.0f
            << 5.0f
            << 6.0f
            << 7.0f
            << 8.0f
            << 12.0f
            << 16.0f
            << 32.0f
            << 48.0f;

    if (direction == ZoomIn) {
        for (int i = 0; i < zoomScales.length(); ++i) {
            if (zoomScales[i] > m_currentScale)
                return zoomScales[i];
        }
        return zoomScales.last();
    } else {
        for (int i = zoomScales.length() - 1; i >= 0; --i) {
            if (zoomScales[i] < m_currentScale)
                return zoomScales[i];
        }
        return zoomScales.first();
    }

    return 1.0f;
}

} // namespace QtQuick2
} // namespace QmlJSDebugger
