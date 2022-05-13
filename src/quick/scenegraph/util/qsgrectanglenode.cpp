// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrectanglenode.h"

QT_BEGIN_NAMESPACE

/*!
  \class QSGRectangleNode

  \brief The QSGRectangleNode class is a convenience class for drawing
  solid filled rectangles using scenegraph.
  \inmodule QtQuick
  \since 5.8
 */

/*!
    \fn void QSGRectangleNode::setRect(const QRectF &rect)

    Sets the rectangle of this rect node to \a rect.
 */

/*!
    \fn void QSGRectangleNode::setRect(qreal x, qreal y, qreal w, qreal h)
    \overload

    Sets the rectangle of this rect node to begin at (\a x, \a y) and have
    width \a w and height \a h.
 */

/*!
    \fn QRectF QSGRectangleNode::rect() const

    Returns the rectangle that this rect node covers.
 */

/*!
    \fn void QSGRectangleNode::setColor(const QColor &color)

    Sets the color of this rectangle to \a color. The default color will be
    white.
 */

/*!
    \fn QColor QSGRectangleNode::color() const

    Returns the color of this rectangle.
 */

QT_END_NAMESPACE
