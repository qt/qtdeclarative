/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qsgsimplerectnode.h"
#include "qsgflatcolormaterial.h"

QT_BEGIN_NAMESPACE

/*!
  \class QSGSimpleRectNode

  \brief The QSGSimpleRectNode class is a convenience class for drawing
  solid filled rectangles using scenegraph.
  \inmodule QtQuick

  \warning This utility class is only functional when running with the default
  or software backends of the Qt Quick scenegraph. As an alternative, prefer
  using QSGRectangleNode via QQuickWindow::createRectangleNode() or
  QSGEngine::createRectangleNode().

  \deprecated
 */



/*!
    Constructs a QSGSimpleRectNode instance which is spanning \a rect with
    the color \a color.
 */
QSGSimpleRectNode::QSGSimpleRectNode(const QRectF &rect, const QColor &color)
    : m_geometry(QSGGeometry::defaultAttributes_Point2D(), 4)
{
    Q_UNUSED(reserved);
    QSGGeometry::updateRectGeometry(&m_geometry, rect);
    m_material.setColor(color);
    setMaterial(&m_material);
    setGeometry(&m_geometry);
}



/*!
    Constructs a QSGSimpleRectNode instance with an empty rectangle and
    white color.
 */
QSGSimpleRectNode::QSGSimpleRectNode()
    : m_geometry(QSGGeometry::defaultAttributes_Point2D(), 4)
{
    QSGGeometry::updateRectGeometry(&m_geometry, QRectF());
    setMaterial(&m_material);
    setGeometry(&m_geometry);
}



/*!
    Sets the rectangle of this rect node to \a rect.
 */
void QSGSimpleRectNode::setRect(const QRectF &rect)
{
    QSGGeometry::updateRectGeometry(&m_geometry, rect);
    markDirty(QSGNode::DirtyGeometry);
}


/*!
    \fn void QSGSimpleRectNode::setRect(qreal x, qreal y, qreal w, qreal h)
    \overload

    Sets the rectangle of this rect node to begin at (\a x, \a y) and have
    width \a w and height \a h.
 */

/*!
    Returns the rectangle that this rect node covers.
 */
QRectF QSGSimpleRectNode::rect() const
{
    const QSGGeometry::Point2D *pts = m_geometry.vertexDataAsPoint2D();
    return QRectF(pts[0].x,
                  pts[0].y,
                  pts[3].x - pts[0].x,
                  pts[3].y - pts[0].y);
}


/*!
    Sets the color of this rectangle to \a color. The default
    color will be white.
 */
void QSGSimpleRectNode::setColor(const QColor &color)
{
    if (color != m_material.color()) {
        m_material.setColor(color);
        markDirty(QSGNode::DirtyMaterial);
    }
}



/*!
    Returns the color of this rectangle.
 */
QColor QSGSimpleRectNode::color() const
{
    return m_material.color();
}

QT_END_NAMESPACE
